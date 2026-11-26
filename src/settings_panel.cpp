#include "settings_panel.h"

#include "build_id.h"
#include "luxvelum_switch.h"
#include "zone_halo.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>

namespace
{
constexpr float kMultiplierSliderMax = 4.0f;
constexpr float kLayerSliderMax = 2.0f;
constexpr float kLocalLightSliderMax = 8.0f;
constexpr float kPanelWidthInLines = 30.0f;
constexpr float kPanelHeightInLines = 42.0f;
constexpr float kMinPanelWidthInLines = 20.0f;
constexpr float kMinPanelHeightInLines = 16.0f;
constexpr float kPanelMarginInLines = 2.0f;
constexpr float kLabelWidthInLines = 11.0f;
constexpr float kLabelShareOfWidth = 0.45f;
constexpr float kFirstFooterLines = 5.0f;
constexpr float kNoTextWrap = -1.0f;
constexpr size_t kStatusTextSize = 512;
constexpr size_t kWindowTitleSize = 96;
const ImVec4 kDrawnColour = {0.45f, 0.85f, 0.45f, 1.0f};
const ImVec4 kSkippedColour = {0.95f, 0.75f, 0.35f, 1.0f};
const ImVec4 kErrorColour = {1.0f, 0.4f, 0.4f, 1.0f};

const char* const kQualityNames[] = {"Low: quarter resolution, 16 steps", "Medium: half resolution, 24 steps",
                                     "High: half resolution, 32 steps"};
const char* const kDebugViewNames[] = {"Off", "Fog radiance", "Transmittance", "Linear depth",
                                       "Full-resolution march"};
const char* const kLogLevelNames[] = {"Errors", "Info", "Debug"};
const char* const kWaterQualityNames[] = {"Low: 128 waves, sky reflections only",
                                          "Medium: 256 waves, scene reflections",
                                          "High: 256 waves, finer scene reflections"};
const char* const kWaterDebugViewNames[] = {"Off",        "Normals",      "Foam",   "Transmittance",
                                           "Reflection", "Liquid class", "Ripples"};

template <int N>
bool Choice(const char* label, int& value, int first, const char* const (&names)[N], const char* help)
{
    int index = value - first;
    const bool changed = ImGui::Combo(label, &index, names, N);
    ImGui::SetItemTooltip("%s", help);
    if (changed)
        value = index + first;
    return changed;
}

class Section
{
public:
    explicit Section(const char* title, ImGuiTreeNodeFlags flags = 0)
        : m_title(title), m_open(ImGui::CollapsingHeader(title, flags))
    {
        if (m_open)
            ImGui::PushID(m_title);
    }
    ~Section()
    {
        if (m_open)
            ImGui::PopID();
    }
    Section(const Section&) = delete;
    Section& operator=(const Section&) = delete;
    explicit operator bool() const { return m_open; }

private:
    const char* m_title;
    bool m_open;
};

bool Slider(const char* label, float& value, float lo, float hi, const char* format, const char* help,
            ImGuiSliderFlags flags = ImGuiSliderFlags_None)
{
    const bool changed = ImGui::SliderFloat(label, &value, lo, hi, format, flags);
    ImGui::SetItemTooltip("%s", help);
    return changed;
}

bool Multiplier(const char* label, float& value, const char* help)
{
    return Slider(label, value, 0.0f, kMultiplierSliderMax, "%.2f", help);
}

bool LayerMultiplier(const char* label, float& value, const char* help)
{
    return Slider(label, value, 0.0f, kLayerSliderMax, "%.3f", help);
}

bool Toggle(const char* label, bool& value, const char* help)
{
    const bool changed = ImGui::Checkbox(label, &value);
    ImGui::SetItemTooltip("%s", help);
    return changed;
}

bool Toggle(const char* label, int& value, const char* help)
{
    bool on = value != 0;
    const bool changed = Toggle(label, on, help);
    if (changed)
        value = on ? 1 : 0;
    return changed;
}

void DrawStatus(const FogFrameStatus& status)
{
    if (status.drawn)
    {
        ImGui::TextColored(kDrawnColour, "Fog: Drawing");
        if (status.reason && *status.reason)
            ImGui::TextWrapped("%s", status.reason);
    }
    else
        ImGui::TextColored(kSkippedColour, "Fog: Not drawing (%s)", status.reason);
}

void StatusLine(const ImVec4& colour, const char* format, ...)
{
    char text[kStatusTextSize];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    ImGui::PushTextWrapPos(kNoTextWrap);
    ImGui::TextColored(colour, "%s", text);
    ImGui::PopTextWrapPos();
    ImGui::SetItemTooltip("%s", text);
}

void DrawWaterStatus(const WaterFrameStatus& status)
{
    const bool named = status.reason && *status.reason;
    if (status.drawn && named)
        StatusLine(kDrawnColour, "Water: Drawing (%s)", status.reason);
    else if (status.drawn)
        StatusLine(kDrawnColour, "Water: Drawing");
    else
        StatusLine(kSkippedColour, "Water: Not drawing (%s)", named ? status.reason : "no reason given");
}

void DrawSwitchRow(const Config& cfg)
{
    const std::string key = HotkeyName(cfg.toggleKey);
    char help[kStatusTextSize];
    std::snprintf(help, sizeof(help),
                  "Off: the game draws its own fog, water and glow, as without Luxvelum, until Luxvelum is switched "
                  "on again; a client restart switches it on. ToggleKey in Luxvelum.ini does the same with a key "
                  "(now %s)",
                  key.c_str());
    bool on = !LuxvelumSwitchFlippedOff();
    if (Toggle("Luxvelum on", on, help))
        FlipLuxvelumSwitch();
    ImGui::SameLine();
    ImGui::TextDisabled("ToggleKey: %s", key.c_str());
}

void DrawMultisamplingStatus(const MultisamplingStatus& status)
{
    if (status.method && *status.method)
        StatusLine(kDrawnColour, "Antialiasing: multisampling %dx kept (depth copied by %s)", status.samples,
                   status.method);
    else
        StatusLine(ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled), "Antialiasing: multisampling off (%s)",
                   status.off);
}

bool DrawQuality(Config& c)
{
    const Section section("Quality", ImGuiTreeNodeFlags_DefaultOpen);
    if (!section)
        return false;
    bool changed = Choice("Quality", c.quality, 1, kQualityNames, "Resolution and ray-march steps of the fog pass");
    changed |= Slider("Temporal filter", c.temporal, 0.0f, 0.97f, "%.2f",
                      "History weight of the temporal filter, 0 = off");
    return changed;
}

bool DrawDensity(Config& c)
{
    const Section section("Density", ImGuiTreeNodeFlags_DefaultOpen);
    if (!section)
        return false;
    bool changed = Multiplier("Density", c.density,
                              "Fog thickness affects visibility and scattered light. Use Sun scatter for the halo.");
    changed |= LayerMultiplier("Haze", c.haze, "Distance haze; with Classic data its first layer, the haze");
    changed |= LayerMultiplier(
        "Ground fog", c.groundFog,
        "Low ground mist; with Classic data its third layer, the ground fog (0.6 keeps it as authored)");
    changed |= LayerMultiplier(
        "Distance fog", c.farFog,
        "The distance fog that replaces the stock fog; with Classic data also its far wall layer");
    changed |= Toggle("Replace the stock fog", c.stockFog,
                      "On: the volumetric distance fog replaces the client's linear fog. Off: keep the stock fog");
    changed |= Toggle("Classic fog layers", c.dataMode,
                      "Use the Classic client's fog layers where its lights cover the map. Off: derived layers "
                      "everywhere");
    changed |= Slider("Fog range", c.maxDistance, 200.0f, 5000.0f, "%.0f yd",
                      "How far sky rays are integrated and the scale of the Classic distance curves");
    changed |= Slider("Density variation", c.noiseAmount, 0.0f, 1.0f, "%.2f",
                      "Spatial variation in haze and ground fog; 0 keeps the authored density smooth");
    changed |= Slider("Variation scale", c.noiseScale, 0.001f, 1.0f, "%.3f",
                      "Larger values make smaller mist patches", ImGuiSliderFlags_Logarithmic);
    changed |= Slider("Mist drift", c.noiseWindSpeed, 0.0f, 10.0f, "%.2f yd/s",
                      "Speed of drifting mist; 0 keeps it stationary");
    changed |= Toggle("Classic fog noise", c.classicNoise,
                      "Drifting fog banks where the Classic layers carry the modern client's noise, mostly in storms; "
                      "it thins those layers to about half their density on average. Off: the density variation "
                      "above applies to them instead");
    return changed;
}

bool SkyColour(const char* label, float& value, const char* region)
{
    char help[kStatusTextSize];
    std::snprintf(help, sizeof(help),
                  "How much the fog in %s takes the hue of the client's current fog colour, which blends the land "
                  "into the sky and follows the weather, at the fog's own brightness: 1 = that hue, 0 = the colours "
                  "in fogdata.bin",
                  region);
    return Slider(label, value, 0.0f, 1.0f, "%.2f", help);
}

bool RegionLocalLights(const char* label, float& value, const char* region)
{
    char help[kStatusTextSize];
    std::snprintf(help, sizeof(help),
                  "Local light intensity in %s, on top of Local light intensity; the region's density does not "
                  "change the lamps' glow",
                  region);
    return Slider(label, value, 0.0f, kRegionLocalLightsSliderMax, "%.2f", help, ImGuiSliderFlags_Logarithmic);
}

bool DrawRegions(Config& c)
{
    const Section section("Regions", ImGuiTreeNodeFlags_DefaultOpen);
    if (!section)
        return false;
    bool changed = Toggle("Outland fog data", c.outlandClassicFog,
                          "On: Outland's fog layers come from its own client lights, converted into fogdata.bin with "
                          "the layers of the most alike Classic light. Off: derived layers, with the client's glow and "
                          "no colour grading. The Burning Crusade starting zones keep their converted lights");
    changed |= Toggle("Northrend fog data", c.northrendClassicFog,
                      "On: Northrend's fog layers come from its own client lights, converted into fogdata.bin with "
                      "the layers of the most alike Classic light. Off: derived layers, with the client's glow and no "
                      "colour grading");
    changed |= Multiplier("Azeroth density", c.azerothDensity,
                          "Fog thickness on the old continents, in their dungeons and on every other map, on top "
                          "of Density");
    changed |= SkyColour("Azeroth sky colour", c.azerothSkyColor, "Azeroth and on every other map");
    changed |= RegionLocalLights("Azeroth local lights", c.azerothLocalLights, "Azeroth and on every other map");
    changed |= Multiplier("Outland density", c.outlandDensity, "Fog thickness in Outland, on top of Density");
    changed |= SkyColour("Outland sky colour", c.outlandSkyColor, "Outland");
    changed |= RegionLocalLights("Outland local lights", c.outlandLocalLights, "Outland");
    changed |= Multiplier("Northrend density", c.northrendDensity, "Fog thickness in Northrend, on top of Density");
    changed |= SkyColour("Northrend sky colour", c.northrendSkyColor, "Northrend");
    changed |= RegionLocalLights("Northrend local lights", c.northrendLocalLights, "Northrend");
    changed |= Multiplier("TBC start density", c.tbcStartDensity,
                          "Fog thickness in the Burning Crusade starting zones (Eversong Woods, the Ghostlands, "
                          "Silvermoon, Quel'Danas, the Azuremyst and Bloodmyst isles, the Exodar), on top of Density");
    changed |= SkyColour("TBC start sky colour", c.tbcStartSkyColor, "the Burning Crusade starting zones");
    changed |= RegionLocalLights("TBC start local lights", c.tbcStartLocalLights,
                                 "the Burning Crusade starting zones");
    return changed;
}

const char* ZoneHaloSourceName(ZoneHaloSource source)
{
    switch (source)
    {
    case ZoneHaloSource::Own:
        return "your own";
    case ZoneHaloSource::Shipped:
        return "shipped";
    case ZoneHaloSource::Unlisted:
        break;
    }
    return "not listed";
}

void DrawZoneHalo()
{
    const Section section("This zone", ImGuiTreeNodeFlags_DefaultOpen);
    if (!section)
        return;
    ZoneHaloBook& book = GlobalZoneHalo();
    const ZoneHaloView view = book.Current();
    if (view.zoneId <= 0)
    {
        ImGui::TextDisabled("No zone yet");
        return;
    }
    ImGui::Text("%s (zone %d), %s", view.zoneName[0] ? view.zoneName : "Unnamed zone", view.zoneId,
                ZoneHaloSourceName(view.source));
    ZoneHaloFactors factors = view.factors;
    bool changed = Slider("Sun halo here", factors.sun, 0.0f, kZoneHaloMax, "%.2f",
                          "The sun's halo in this zone, on top of Sun scatter; saved as your own to "
                          "WTF\\Luxvelum-zones.ini when you let go. 1 = as Sun scatter alone");
    bool released = ImGui::IsItemDeactivatedAfterEdit();
    changed |= Slider("Moon halo here", factors.moon, 0.0f, kZoneHaloMax, "%.2f",
                      "The moon's halo in this zone, on top of Moon scatter; saved as your own to "
                      "WTF\\Luxvelum-zones.ini when you let go. 1 = as Moon scatter alone");
    released |= ImGui::IsItemDeactivatedAfterEdit();
    changed |= Slider("Fog density here", factors.density, 0.0f, kZoneHaloMax, "%.2f",
                      "The fog's density in this zone, on top of Density and the region's density; saved as your "
                      "own to WTF\\Luxvelum-zones.ini when you let go. 1 = as those alone, 0 = no fog here");
    released |= ImGui::IsItemDeactivatedAfterEdit();
    if (changed)
        book.Edit(factors);
    if (released)
        book.SaveCurrent();
    ImGui::BeginDisabled(view.source != ZoneHaloSource::Own);
    if (ImGui::Button("Reset this zone"))
        book.ForgetCurrent();
    ImGui::SetItemTooltip("Remove your own values for this zone from WTF\\Luxvelum-zones.ini, back to the shipped "
                          "values of Data\\Luxvelum\\zones.ini, or 1 where it lists none");
    ImGui::EndDisabled();
}

bool DrawLight(Config& c)
{
    const Section section("Light", ImGuiTreeNodeFlags_DefaultOpen);
    if (!section)
        return false;
    bool changed = Multiplier("Sun scatter", c.sunScatter, "In-scattered sun light by day: the halo around it");
    changed |= Multiplier("Moon scatter", c.moonScatter,
                          "In-scattered moon light at night, on its own: Sun scatter does not change it. 1 = as "
                          "bright as the sun's halo");
    changed |= Slider("Sun shadow", c.sunShadow, 0.0f, 1.0f, "%.2f",
                      "How much walls, hills and buildings between the fog and the sun or moon darken that halo");
    changed |= Multiplier("Ambient", c.ambient, "Ambient fog brightness");
    changed |= Multiplier("Exposure", c.exposure, "Brightness of the layers used where no Classic data exists");
    changed |= Multiplier("Classic exposure", c.classicExposure, "Brightness of the Classic layers, 1 = default");
    changed |= Toggle("Energy-normalised Classic phase", c.classicPhase,
                      "Scatter the sun and moon into the Classic layers with an energy-normalised phase, as their "
                      "authored intensities suggest the modern client does: dimmer horizon bands, a larger and "
                      "brighter halo around the sun. Off: the phase peaks at 1 toward the light");
    changed |= Toggle("Linear light", c.colorSpace,
                      "Scatter and blend in linear light like the modern client, with a soft highlight roll-off. "
                      "Off: gamma");
    changed |= Toggle("Local lights", c.localLights, "Scatter point lights in view from the world into the fog");
    changed |= Slider("Local light intensity", c.localLightIntensity, 0.0f, kLocalLightSliderMax, "%.2f",
                      "Brightness of the point lights in the fog");
    changed |= Slider("Local light phase", c.localLightPhase, -0.9f, 0.9f, "%.2f",
                      "How every point light scatters in the fog: 0 evenly in all directions; toward 0.9 mostly in "
                      "the fog between you and the light; below 0 the fog behind the light, steeply brighter toward "
                      "-0.9. The modern client uses one such value for all its lights; 0.3 is a calibration");
    changed |= Slider("God rays", c.godRays, 0.0f, 4.0f, "%.2f",
                      "Shafts of light through gaps in trees, buildings and terrain in front of the sun, 0 = off. An "
                      "open sky, its clouds included, gets none; the glow around the sun is Sun scatter's");
    changed |= Toggle("Glow compensation", c.glowCompensation,
                      "Pre-compensate the fog for the client's full-screen glow, which otherwise bleaches bright fog "
                      "around the sun to white. The fog itself only with linear light, while it blends over a copy "
                      "of the scene; otherwise only the god rays are compensated");
    return changed;
}

bool DrawGlowAndGrading(Config& c)
{
    const Section section("Glow and colour grading");
    if (!section)
        return false;
    bool changed = Toggle("Forever glow", c.foreverGlow,
                          "Use the modern client's glow amount where Classic lights cover the camera: 0 on most "
                          "continent lights, so the full-screen glow mostly disappears there. It replaces the client's "
                          "own amount, Ascension's ambientGlow included, and fades back to it at the edge of Classic "
                          "coverage. Off: the client's glow");
    changed |= Slider("Colour grading", c.colorGrading, 0.0f, 1.0f, "%.2f",
                      "Strength of the modern client's colour curve for the lights around the camera: brighter "
                      "midtones, and the brightest highlights clipped to white. It grades the 3D view after the glow, "
                      "names drawn in the world included; the interface, the ghost view and the view under water are "
                      "not graded. 0 = off");
    return changed;
}

bool DrawViewDistance(Config& c)
{
    bool lifted = c.farClipMax > kFarClipMaxKeepsClientCap;
    bool changed = Toggle("Lift the continent view distance", lifted,
                          "Ascension caps the continents at 791 yd; the engine allows 1583. Costs about four times "
                          "the loaded terrain. Turning it on when it was off at start-up needs a client restart");
    if (changed)
        c.farClipMax = lifted ? kEngineFarClipMax : kFarClipMaxKeepsClientCap;
    if (lifted)
        changed |= Slider("View distance", c.farClipMax, std::ceil(kEngineFarClipMin), kEngineFarClipMax, "%.0f yd",
                          "Continent view distance, within the client's own farclip setting",
                          ImGuiSliderFlags_AlwaysClamp);
    ImGui::TextDisabled("Applies at the next farclip change, map load or zone change.");
    return changed;
}

bool DrawWorld(Config& c)
{
    const Section section("World");
    if (!section)
        return false;
    bool changed = Toggle("Fog under water", c.underwater, "Keep the effect while the camera is under water");
    changed |= Toggle("Fog effects at their own distance", c.transparentFog,
                      "On by default. Particles, spell effects and other see-through models are fogged by their own "
                      "distance, as the stock client does, with a linear fog fitted to the volumetric fog within "
                      "100 yd, or in thin fog over a longer range up to where the fog halves the light, never past "
                      "the far clip; the fog is drawn before them, after the water. Beyond the fitted range the line "
                      "is extended, so distant effects and fading models can be fogged more or less than the scene "
                      "around them, and past its end distant glows vanish. Off, the fallback if distant effects look "
                      "wrongly fogged: the fog is drawn once after the world, before the names, and they take the "
                      "fog of the scene behind them");
    changed |= Toggle("Water writes depth", c.liquidDepth,
                      "Let water surfaces write depth so water is fogged by its own distance; always on while Modern "
                      "water is drawn");
    changed |= Toggle("Interior-aware fog", c.interiorAware,
                      "Fade outdoor fog and direct sunlight as the camera enters a building or cave");
    changed |= Slider("Interior density", c.interiorDensity, 0.0f, 1.0f, "%.2f",
                      "Outdoor fog density retained indoors");
    changed |= Slider("Interior far fog", c.interiorFarFog, 0.0f, 1.0f, "%.2f",
                      "Native distance-fog density retained indoors; outdoor distance fog is unchanged");
    changed |= DrawViewDistance(c);
    return changed;
}

bool DrawAntialiasing(Config& c)
{
    const Section section("Antialiasing");
    if (!section)
        return false;
    bool changed = Toggle("Keep the game's multisampling", c.multisampling,
                          "Keep the game's Multisampling video option (smooth edges) when the graphics driver can "
                          "copy its depth for the fog and water: NVIDIA through NVAPI, AMD and Intel through RESZ. "
                          "Off: multisampling stays off. The game lists its Multisampling choices once per start, so "
                          "turning this on takes effect after restarting the game; turning it off applies the next "
                          "time the game resets its display, for example after changing Multisampling or the "
                          "resolution in its Video options");
    changed |= Toggle("Report AMD to DXVK", c.dxvkAmdVendor,
                      "For an NVIDIA card behind DXVK: DXVK offers the RESZ depth copy only to AMD and Intel cards, so "
                      "Luxvelum asks it to report AMD (d3d9.customVendorId = 1002) and multisampling becomes "
                      "available, as that line in dxvk.conf would do. The game and DXVK then see an AMD card. "
                      "Takes effect after restarting the game");
    return changed;
}

bool DrawWater(Config& c)
{
    const Section section("Water");
    if (!section)
        return false;
    bool changed = Toggle("Modern water", c.water,
                          "Shade lakes, rivers, the sea and indoor pools like the modern client; water surfaces write "
                          "depth while it is drawn. Off: the client's own water");
    changed |= Choice("Water quality", c.waterQuality, 1, kWaterQualityNames,
                      "Size of the wave simulation and detail of the reflections");
    changed |= Slider("Waves", c.waterWaves, 0.0f, 2.0f, "%.2f", "Wave height, 0 = flat water");
    changed |= Slider("Wind", c.waterWind, 0.5f, 10.0f, "%.2f",
                      "Wind that drives the waves; stronger wind makes longer, rougher waves");
    changed |= Slider("Foam", c.waterFoam, 0.0f, 2.0f, "%.2f", "Foam on wave crests and along shores, 0 = none");
    changed |= Slider("Reflections", c.waterReflections, 0.0f, 2.0f, "%.2f",
                      "Reflections of the sky and the scene, 0 = none");
    changed |= Slider("Reflection waviness", c.waterReflectionWaviness, 0.0f, 1.0f, "%.2f",
                      "How much the waves bend the reflections, without changing the waves themselves: lower is "
                      "calmer and clearer, 0 = a mirror even on rough water, 1 = every wave bends them");
    changed |= Slider("Sun highlight", c.waterSpecular, 0.0f, 4.0f, "%.2f",
                      "Highlight of the sun or moon on the water, 0 = none");
    changed |= Slider("Moon highlight", c.waterMoonGlint, 0.0f, 10.0f, "%.2f",
                      "The moon's highlight on the water at night, on top of Sun highlight: the night's light is "
                      "dim, while the moon's disc is bright. 1 = as dim as the night's light");
    changed |= Slider("Clarity", c.waterClarity, 0.25f, 4.0f, "%.2f",
                      "How far you see into the water; larger is clearer", ImGuiSliderFlags_Logarithmic);
    changed |= Slider("Zone colours", c.waterZoneColors, 0.0f, 1.0f, "%.2f",
                      "How much the zone's own water colours from the client's lights tint the water, 0 = the modern "
                      "colours only");
    changed |= Slider("Ripples", c.waterRipples, 0.0f, 2.0f, "%.2f",
                      "Wakes of players, creatures, pets and mounts in the water: a thin V behind a unit that runs or "
                      "swims, one ring as it starts or stops, a calm surface while it stands. 0.5 = default, 0 = none");
    changed |= Toggle("Client splashes", c.waterClientSplashes,
                      "Keep the client's own flat splash and wake sprites while the ripples run; off hides them for "
                      "every unit, also those beyond the ripples' reach");
    changed |= Choice("Water view", c.waterDebugView, 0, kWaterDebugViewNames,
                      "Show one input of the water shading instead of the scene");
    return changed;
}

bool DrawDebug(Config& c)
{
    const Section section("Debug");
    if (!section)
        return false;
    bool changed = Choice("View", c.debugView, 0, kDebugViewNames, "Show one input of the fog instead of the scene");
    changed |= Toggle("Sun marker", c.sunMarker, "Red marker where the sun direction projects on screen");
    changed |= Choice("Log level", c.logLevel, 0, kLogLevelNames, "Detail written to Luxvelum.log");
    return changed;
}

const char* WindowTitle()
{
    static char title[kWindowTitleSize] = {};
    if (!title[0])
        std::snprintf(title, sizeof(title), "Luxvelum %s (%s)###Luxvelum", kLuxvelumVersion, kLuxvelumBuildId);
    return title;
}
}

void SettingsPanel::PlaceWindow()
{
    const float line = ImGui::GetFontSize();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float margin = kPanelMarginInLines * line;
    const ImVec2 smallest(kMinPanelWidthInLines * line, kMinPanelHeightInLines * line);
    const ImVec2 largest(std::max(display.x - margin, smallest.x), std::max(display.y - margin, smallest.y));
    const ImVec2 firstSize(kPanelWidthInLines * line,
                           std::max(smallest.y, std::min(kPanelHeightInLines * line, display.y - 2.0f * margin)));
    const PanelPlacement& p = m_placement;
    ImGui::SetNextWindowPos(p.known ? ImVec2(p.position[0], p.position[1]) : ImVec2(margin, margin),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(p.known ? ImVec2(p.size[0], p.size[1]) : firstSize, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(smallest, largest);
}

void SettingsPanel::RememberPlacement()
{
    const ImVec2 position = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    m_placement = {true, {position.x, position.y}, {size.x, size.y}};
}

bool SettingsPanel::DrawSettings(Config& edited, const FogFrameStatus& fogStatus)
{
    const float footer =
        m_footerHeight > 0.0f ? m_footerHeight : kFirstFooterLines * ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("Settings", ImVec2(0.0f, -footer), ImGuiChildFlags_None, ImGuiWindowFlags_NoNavInputs);
    DrawStatus(fogStatus);
    const float labels =
        std::min(kLabelWidthInLines * ImGui::GetFontSize(), kLabelShareOfWidth * ImGui::GetContentRegionAvail().x);
    ImGui::PushItemWidth(-labels);
    bool changed = DrawQuality(edited);
    changed |= DrawDensity(edited);
    changed |= DrawRegions(edited);
    changed |= DrawLight(edited);
    DrawZoneHalo();
    changed |= DrawGlowAndGrading(edited);
    changed |= DrawWorld(edited);
    changed |= DrawAntialiasing(edited);
    changed |= DrawWater(edited);
    changed |= DrawDebug(edited);
    ImGui::PopItemWidth();
    ImGui::EndChild();
    return changed;
}

void SettingsPanel::DrawFooter(ConfigStore& store, const WaterFrameStatus& waterStatus,
                               const MultisamplingStatus& multisampling)
{
    const float top = ImGui::GetCursorPosY();
    ImGui::Separator();
    DrawSwitchRow(store.Get());
    DrawWaterStatus(waterStatus);
    DrawMultisamplingStatus(multisampling);
    DrawSaveRow(store);
    m_footerHeight = ImGui::GetCursorPosY() - top;
}

void SettingsPanel::Draw(ConfigStore& store, const FogFrameStatus& fogStatus, const WaterFrameStatus& waterStatus,
                         const MultisamplingStatus& multisampling, bool& open)
{
    PlaceWindow();
    const bool expanded =
        ImGui::Begin(WindowTitle(), &open, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNavInputs);
    if (!expanded)
    {
        ImGui::End();
        return;
    }
    RememberPlacement();
    ImGui::PushTextWrapPos(0.0f);
    Config edited = store.Get();
    if (DrawSettings(edited, fogStatus))
        store.Apply(edited);
    if (!ImGui::IsAnyItemActive())
        store.LogSettledEdits();
    DrawFooter(store, waterStatus, multisampling);
    ImGui::PopTextWrapPos();
    ImGui::End();
}

void SettingsPanel::DrawSaveRow(ConfigStore& store)
{
    const bool unsaved = store.HasUnsavedChanges();
    ImGui::BeginDisabled(!unsaved);
    if (ImGui::Button("Save"))
        m_saveFailed = !store.Save();
    ImGui::SetItemTooltip("Write the changed settings to Luxvelum.ini");
    ImGui::SameLine();
    if (ImGui::Button("Revert"))
    {
        store.Revert();
        m_saveFailed = false;
    }
    ImGui::SetItemTooltip("Go back to the settings in Luxvelum.ini");
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (m_saveFailed)
        ImGui::TextColored(kErrorColour, "Could not write Luxvelum.ini");
    else
        ImGui::TextDisabled("%s", unsaved ? "Unsaved changes" : "Matches Luxvelum.ini");
}
