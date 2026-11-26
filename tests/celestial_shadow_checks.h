#pragma once

namespace celestial_shadow
{
constexpr double kStart = 100.0;
constexpr int kMap = 0;
constexpr int kOtherMap = 1;
constexpr float kRidgeDistance = 300.0f;
constexpr float kNearOccluder = 40.0f;
constexpr float kBeyondEverySurface = 5000.0f;
constexpr float kMaxDifference = 1.0e-4f;
constexpr float kMinHaloRadiance = 0.01f;
constexpr UINT kCentre = march_layers::kSize / 2 - 1;
constexpr UINT kSkyRow = 0;
constexpr UINT kFirstSurfaceRow = 3;
constexpr int kHaloShadowRegister = 79;
constexpr int kHaloEdgePhaseRegister = 80;
constexpr float kForwardScattering = 0.8f;

CelestialSightLine Blocked(float farthestBlockAt)
{
    CelestialSightLine sight;
    sight.traced = true;
    sight.blocked = true;
    sight.farthestBlockAt = farthestBlockAt;
    return sight;
}

CelestialSightLine Clear()
{
    CelestialSightLine sight;
    sight.traced = true;
    return sight;
}

void CheckTheSmoothedShareOfTheSightLine()
{
    CelestialOcclusion occlusion;
    const bool firstDue = occlusion.SightLineDue(kStart, false, kMap);
    occlusion.Observe(kStart, false, kMap, Blocked(kRidgeDistance));
    occlusion.Advance(kStart);
    const CelestialShadow first = occlusion.Shadow();
    Check(firstDue && first.hidden == 1.0f && first.occluderDistance == kRidgeDistance,
          "the first sight line to the sun sets its hidden share and the farthest occluder at once");

    const double soon = kStart + kCelestialSightLineSeconds * 0.5;
    const double later = kStart + 2.0 * kCelestialSightLineSeconds;
    Check(!occlusion.SightLineDue(soon, false, kMap) && occlusion.SightLineDue(later, false, kMap) &&
              occlusion.SightLineDue(soon, true, kMap) && occlusion.SightLineDue(soon, false, kOtherMap) &&
              occlusion.SightLineDue(kStart - 1.0, false, kMap),
          "the sight line is traced again after its interval, at once for the other light, map or a clock jump");

    occlusion.Advance(later);
    occlusion.Observe(later, false, kMap, Clear());
    occlusion.Advance(later + kCelestialFadeSeconds);
    const CelestialShadow fading = occlusion.Shadow();
    occlusion.Advance(later + 2.0 * kCelestialFadeSeconds);
    const CelestialShadow faded = occlusion.Shadow();
    occlusion.Advance(later + 20.0 * kCelestialFadeSeconds);
    const CelestialShadow open = occlusion.Shadow();
    Check(std::fabs(fading.hidden - std::exp(-1.0f)) < 1e-3f && std::fabs(faded.hidden - std::exp(-2.0f)) < 1e-3f &&
              open.hidden == 0.0f && fading.occluderDistance == kRidgeDistance,
          "a light coming out from behind a ridge fades in with the fade time and keeps the ridge's distance");

    occlusion.Advance(kStart);
    Check(occlusion.Shadow().hidden == 0.0f, "a clock running backwards leaves the hidden share unchanged");

    const double night = later + 30.0;
    occlusion.Observe(night, true, kMap, Blocked(kNearOccluder));
    occlusion.Advance(night);
    const CelestialShadow moon = occlusion.Shadow();
    occlusion.Observe(night, true, kOtherMap, Clear());
    occlusion.Advance(night);
    const CelestialShadow otherMap = occlusion.Shadow();
    Check(moon.hidden == 1.0f && moon.occluderDistance == kNearOccluder && otherMap.hidden == 0.0f,
          "switching to the moon or to another map takes the new sight line at once instead of fading");

    occlusion.Advance(night + 1.0);
    occlusion.Observe(night + 1.0, true, kOtherMap, Blocked(kRidgeDistance));
    occlusion.Advance(night + 1.0 + kCelestialFadeSeconds);
    const float fadingOut = occlusion.Shadow().hidden;
    CelestialSightLine untraced = Blocked(kNearOccluder);
    untraced.traced = false;
    occlusion.Observe(night + 2.0, true, kOtherMap, untraced);
    occlusion.Advance(night + 40.0);
    Check(std::fabs(fadingOut - (1.0f - std::exp(-1.0f))) < 1e-3f && occlusion.Shadow().hidden == 0.0f &&
              occlusion.Shadow().occluderDistance == kRidgeDistance,
          "a light going behind a ridge fades out, and a sight line the client could not trace leaves it unhidden");
}

void FillHaloConstants(float constants[march_layers::kConstantRegisters][4], const float toLightInView[3],
                       float isotropic, float diffuse, float hidden, float occluderDistance)
{
    march_layers::FillConstants(constants, false, 0.0f);
    std::memcpy(constants[9], toLightInView, 3 * sizeof(float));
    constants[9][3] = 1.0f;
    constants[kHaloShadowRegister][0] = hidden;
    constants[kHaloShadowRegister][1] = occluderDistance;
    march_layers::Layer layers[4] = {};
    layers[0] = march_layers::kLayers[0];
    layers[0].g = kForwardScattering;
    layers[0].isotropic = isotropic;
    layers[0].density = 0.004f;
    layers[0].upperFalloff = 0.0f;
    layers[0].limit = kBeyondEverySurface;
    layers[0].densityVariation = 0.0f;
    for (int c = 0; c < 3; ++c)
        layers[0].diffuse[c] = diffuse;
    for (int i = 1; i < 4; ++i)
        layers[i].limit = march_layers::kMaxFogDistance;
    std::memcpy(constants[march_layers::kFirstLayerRegister], layers, sizeof(layers));
    for (int i = 0; i < 4; ++i)
        constants[kHaloEdgePhaseRegister][i] = HaloEdgePhase(layers[i].g);
    constants[78][0] = 0.0f;
}

struct HaloRender
{
    bool rendered = true;
    float pixels[march_layers::kSize * march_layers::kSize * 4] = {};

    const float* At(UINT x, UINT y) const { return pixels + (y * march_layers::kSize + x) * 4; }
};

HaloRender RenderHalo(IDirect3DDevice9* device, const march_layers::Resources& resources,
                      IDirect3DPixelShader9* shader, const float toLightInView[3], float isotropic, float diffuse,
                      float hidden, float occluderDistance)
{
    float constants[march_layers::kConstantRegisters][4];
    FillHaloConstants(constants, toLightInView, isotropic, diffuse, hidden, occluderDistance);
    HaloRender render;
    render.rendered = march_layers::RenderMarch(device, resources, shader, constants, render.pixels);
    return render;
}

float WorstDifference(const HaloRender& a, const HaloRender& b, bool coloursOnly)
{
    float worst = 0.0f;
    for (UINT i = 0; i < march_layers::kSize * march_layers::kSize * 4; ++i)
        if (!coloursOnly || i % 4 != 3)
            worst = std::max(worst, std::fabs(a.pixels[i] - b.pixels[i]) / std::max(1.0f, std::fabs(b.pixels[i])));
    return worst;
}

float Luminance(const float* pixel)
{
    return (pixel[0] + pixel[1] + pixel[2]) / 3.0f;
}

void CheckHaloOfAHiddenLight(IDirect3DDevice9* device, const BYTE* code, const char* variant)
{
    march_layers::Resources resources;
    IDirect3DPixelShader9* shader = nullptr;
    const bool ready = march_layers::CreateResources(device, resources) &&
                       SUCCEEDED(device->CreatePixelShader(reinterpret_cast<const DWORD*>(code), &shader));
    Check(ready, "halo march targets and shader created");
    if (!ready)
    {
        if (shader)
            shader->Release();
        return;
    }
    march_layers::BindResources(device, resources);
    const float ahead[3] = {0.0f, 0.0f, 1.0f};
    const float behind[3] = {0.0f, 0.0f, -1.0f};
    const HaloRender open = RenderHalo(device, resources, shader, ahead, 0.0f, 1.0f, 0.0f, 0.0f);
    const HaloRender openFar = RenderHalo(device, resources, shader, ahead, 0.0f, 1.0f, 0.0f, kBeyondEverySurface);
    const HaloRender unlit = RenderHalo(device, resources, shader, ahead, 0.0f, 0.0f, 0.0f, 0.0f);
    const HaloRender hidden = RenderHalo(device, resources, shader, ahead, 0.0f, 1.0f, 1.0f, kBeyondEverySurface);
    const HaloRender half = RenderHalo(device, resources, shader, ahead, 0.0f, 1.0f, 0.5f, kBeyondEverySurface);
    const HaloRender isotropicOpen = RenderHalo(device, resources, shader, ahead, 1.0f, 1.0f, 0.0f, 0.0f);
    const HaloRender isotropicHidden =
        RenderHalo(device, resources, shader, ahead, 1.0f, 1.0f, 1.0f, kBeyondEverySurface);
    const HaloRender behindOpen = RenderHalo(device, resources, shader, behind, 0.0f, 1.0f, 0.0f, 0.0f);
    const HaloRender behindHidden =
        RenderHalo(device, resources, shader, behind, 0.0f, 1.0f, 1.0f, kBeyondEverySurface);
    const float skyRay[3] = {-0.125f, 0.875f, 1.0f};
    const float skyRayLength = std::sqrt(skyRay[0] * skyRay[0] + skyRay[1] * skyRay[1] + skyRay[2] * skyRay[2]);
    const float aboveRidge[3] = {skyRay[0] / skyRayLength, skyRay[1] / skyRayLength, skyRay[2] / skyRayLength};
    const HaloRender skyOpen = RenderHalo(device, resources, shader, aboveRidge, 0.0f, 1.0f, 0.0f, 0.0f);
    const HaloRender skyNearRidge = RenderHalo(device, resources, shader, aboveRidge, 0.0f, 1.0f, 1.0f, kNearOccluder);
    const HaloRender skyHidden =
        RenderHalo(device, resources, shader, aboveRidge, 0.0f, 1.0f, 1.0f, kBeyondEverySurface);
    shader->Release();
    const bool rendered = open.rendered && openFar.rendered && unlit.rendered && hidden.rendered && half.rendered &&
                          isotropicOpen.rendered && isotropicHidden.rendered && behindOpen.rendered &&
                          behindHidden.rendered && skyOpen.rendered && skyNearRidge.rendered && skyHidden.rendered;
    char label[256];

    std::snprintf(label, sizeof(label), "%s: an open sky line leaves the march as it was, whatever the distance",
                  variant);
    Check(rendered && WorstDifference(open, openFar, false) == 0.0f, label);

    const UINT surfaceRow = kFirstSurfaceRow + 1;
    const float unlitRadiance = Luminance(unlit.At(kCentre, surfaceRow));
    const float haloRadiance = Luminance(open.At(kCentre, surfaceRow)) - unlitRadiance;
    const float evenRadiance = Luminance(isotropicOpen.At(kCentre, surfaceRow)) - unlitRadiance;
    const float hiddenRadiance = Luminance(hidden.At(kCentre, surfaceRow)) - unlitRadiance;
    const float halfRadiance = Luminance(half.At(kCentre, surfaceRow)) - unlitRadiance;
    const float edgeRadiance = HaloEdgePhase(kForwardScattering) * evenRadiance;
    std::snprintf(label, sizeof(label),
                  "%s: a hidden sun or moon cuts its halo on the surfaces in front of the ridge down to the halo's "
                  "edge and keeps their fog (halo %.3f, hidden %.4f, edge %.4f, half hidden %.3f)",
                  variant, haloRadiance, hiddenRadiance, edgeRadiance, halfRadiance);
    Check(rendered && haloRadiance > kMinHaloRadiance && hiddenRadiance < 0.1f * haloRadiance &&
              std::fabs(hiddenRadiance - edgeRadiance) <= kMaxDifference + 1e-3f * evenRadiance &&
              hidden.At(kCentre, surfaceRow)[3] == open.At(kCentre, surfaceRow)[3] &&
              std::fabs(halfRadiance - 0.5f * (haloRadiance + hiddenRadiance)) <= kMaxDifference + 1e-3f * haloRadiance,
          label);

    const float skyLit = Luminance(skyOpen.At(kCentre, kSkyRow));
    const float skyBehindRidge = Luminance(skyNearRidge.At(kCentre, kSkyRow));
    const float skyShadowed = Luminance(skyHidden.At(kCentre, kSkyRow));
    std::snprintf(label, sizeof(label),
                  "%s: the fog beyond the farthest occluder keeps its halo (sky %.3f open, %.3f behind a ridge %.0f "
                  "yd away, %.3f hidden throughout)",
                  variant, skyLit, skyBehindRidge, kNearOccluder, skyShadowed);
    Check(rendered && skyShadowed + kMinHaloRadiance < skyBehindRidge && skyBehindRidge < skyLit - kMinHaloRadiance,
          label);

    std::snprintf(label, sizeof(label),
                  "%s: isotropic fog and a light behind the camera keep their direct light when it is hidden",
                  variant);
    Check(rendered && WorstDifference(isotropicOpen, isotropicHidden, false) <= kMaxDifference &&
              WorstDifference(behindOpen, behindHidden, false) <= kMaxDifference,
          label);

    device->SetRenderTarget(0, resources.previousTarget);
    device->SetDepthStencilSurface(resources.previousDepth);
    resources.previousState->Apply();
}

void CheckHaloOfAHiddenLight(IDirect3DDevice9* device)
{
    const BYTE* unrolled[3] = {g_ps_march_low, g_ps_march_mid, g_ps_march_high};
    const BYTE* looped[3] = {g_ps_lit_march_low, g_ps_lit_march_mid, g_ps_lit_march_high};
    const char* unrolledNames[3] = {"low march", "mid march", "high march"};
    const char* loopedNames[3] = {"low lit march", "mid lit march", "high lit march"};
    for (int quality = 0; quality < 3; ++quality)
    {
        CheckHaloOfAHiddenLight(device, unrolled[quality], unrolledNames[quality]);
        CheckHaloOfAHiddenLight(device, looped[quality], loopedNames[quality]);
    }
}
}
