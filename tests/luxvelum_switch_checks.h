#pragma once

namespace luxvelum_switch_checks
{
constexpr Hotkey kCtrlF9 = {VK_F9, true, false, false};
constexpr const char* kSwitchedOff = "Luxvelum is switched off";

struct SwitchState
{
    bool latchedOff;
    bool flippedOff;
};

SwitchState CurrentSwitch()
{
    int flipped = 0;
    const bool latched = vf_test_luxvelum_switched_off(&flipped) != 0;
    return {latched, flipped != 0};
}

Config WithToggleKey(Config cfg)
{
    cfg.toggleKey = kCtrlF9;
    return cfg;
}

void FlipAndLatch()
{
    vf_test_flip_luxvelum_switch();
    vf_test_latch_luxvelum_switch();
}

std::wstring Wide(const std::string& text)
{
    return std::wstring(text.begin(), text.end());
}

class IniEdits
{
public:
    explicit IniEdits(const std::wstring& path) : m_path(path) { GetSystemTimeAsFileTime(&m_time); }

    void Write(const wchar_t* key, const std::wstring& value)
    {
        WritePrivateProfileStringW(L"Luxvelum", key, value.c_str(), m_path.c_str());
        ULARGE_INTEGER time = {};
        time.LowPart = m_time.dwLowDateTime;
        time.HighPart = m_time.dwHighDateTime;
        time.QuadPart += kSecondInFileTime * ++m_edits;
        const FILETIME stamp = {time.LowPart, time.HighPart};
        HANDLE file = CreateFileW(m_path.c_str(), FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE)
            return;
        SetFileTime(file, nullptr, nullptr, &stamp);
        CloseHandle(file);
    }

private:
    static constexpr ULONGLONG kSecondInFileTime = 10000000ull;

    std::wstring m_path;
    FILETIME m_time = {};
    ULONGLONG m_edits = 0;
};

void CheckToggleKeySetting(const std::wstring& outDir, const std::wstring& shippedIni)
{
    ConfigStore shipped;
    shipped.Load(NarrowPath(shippedIni));
    const std::vector<std::string> lines = water_settings_checks::IniLines(ReadText(shippedIni));
    Check(!IsHotkeySet(Config().toggleKey) && !IsHotkeySet(shipped.Get().toggleKey) &&
              std::count(lines.begin(), lines.end(), std::string("ToggleKey=")) == 1 &&
              HotkeyName(kNoHotkey) == "none",
          "ToggleKey takes no key by default and the shipped INI lists it empty for each player to set");

    const std::wstring iniPath = FullPath(outDir + L"\\settings-toggle-key.ini");
    Check(CopyFileW(shippedIni.c_str(), iniPath.c_str(), FALSE) != FALSE, "shipped Luxvelum.ini copied for ToggleKey");
    ConfigStore store;
    store.Load(NarrowPath(iniPath));
    LogSetLevel(static_cast<int>(LogLevel::Info));
    const size_t start = ReadText(g_harnessLog).size();
    IniEdits edits(iniPath);
    edits.Write(L"ToggleKey", L"Ctrl+F9");
    const bool reloaded = store.ReloadIfChanged();
    const Hotkey set = store.Get().toggleKey;
    Config edited = store.Get();
    edited.toggleKey = kNoHotkey;
    store.Apply(edited);
    const bool keptByTheWindow = SameHotkey(store.Get().toggleKey, kCtrlF9);
    const std::string overlayKey = HotkeyName(store.Get().overlayKey);
    edits.Write(L"ToggleKey", Wide(overlayKey));
    store.ReloadIfChanged();
    const Hotkey clash = store.Get().toggleKey;
    edits.Write(L"ToggleKey", L"Ctrl+F9");
    store.ReloadIfChanged();
    edits.Write(L"ToggleKey", L"");
    store.ReloadIfChanged();
    const Hotkey cleared = store.Get().toggleKey;
    edits.Write(L"OverlayKey", L"Ctrl+Pause");
    edits.Write(L"ToggleKey", L"Ctrl+ScrollLock");
    store.ReloadIfChanged();
    const Hotkey alias = store.Get().toggleKey;
    const std::string text = water_settings_checks::LoggedByTheHarnessSince(start);
    std::printf("%s", text.c_str());
    Check(reloaded && SameHotkey(set, kCtrlF9) &&
              runtime_cost::HasLine(text, "settings from Luxvelum.ini: ToggleKey none -> Ctrl+F9"),
          "an INI reload sets ToggleKey while the game runs and logs the new key");
    Check(keptByTheWindow, "settings window edits keep the ToggleKey read from the INI");
    const std::string refused =
        "ToggleKey=" + overlayKey + " cannot be told apart from OverlayKey=" + overlayKey + "; ToggleKey is off";
    Check(!IsHotkeySet(clash) && runtime_cost::HasLine(text, refused.c_str()),
          "a ToggleKey equal to the OverlayKey is refused and logged, and the key keeps opening the settings window");
    Check(!IsHotkeySet(cleared) && runtime_cost::HasLine(text, "ToggleKey Ctrl+F9 -> none"),
          "an empty ToggleKey takes the key away again");
    Check(!IsHotkeySet(alias) &&
              runtime_cost::HasLine(text, "ToggleKey=Ctrl+ScrollLock cannot be told apart from OverlayKey=Ctrl+Pause; "
                                          "ToggleKey is off"),
          "a ToggleKey that Windows sends as the OverlayKey (Ctrl+ScrollLock and Ctrl+Pause both arrive as Ctrl+Break) "
          "is refused too");

    const size_t saveStart = ReadText(g_harnessLog).size();
    edits.Write(L"ToggleKey", L"Ctrl+F9");
    Config windowEdit = store.Get();
    windowEdit.density = 2.0f;
    store.Apply(windowEdit);
    const bool savedOverAHandEdit = store.Save();
    const std::string saveText = water_settings_checks::LoggedByTheHarnessSince(saveStart);
    std::printf("%s", saveText.c_str());
    Check(savedOverAHandEdit && SameHotkey(store.Get().toggleKey, kCtrlF9) &&
              runtime_cost::HasLine(saveText, "ToggleKey none -> Ctrl+F9"),
          "Save takes a ToggleKey written into the INI since the last reload instead of dropping it");
}

void CheckTheSwitchTakesEffectAtTheNextFrame()
{
    const SwitchState start = CurrentSwitch();
    vf_test_flip_luxvelum_switch();
    const SwitchState flipped = CurrentSwitch();
    const bool latched = vf_test_latch_luxvelum_switch() != 0;
    const SwitchState off = CurrentSwitch();
    const bool latchedAgain = vf_test_latch_luxvelum_switch() != 0;
    FlipAndLatch();
    const SwitchState on = CurrentSwitch();
    Check(!start.latchedOff && !start.flippedOff && flipped.flippedOff && !flipped.latchedOff && latched &&
              off.latchedOff && !latchedAgain && !on.latchedOff && !on.flippedOff,
          "Luxvelum starts switched on, a flip takes effect once at the next frame, and a second flip switches it on "
          "again");
}

Image RenderTheClientAlone(Harness& h, const transparent_fog_checks::View& v,
                           const transparent_fog_checks::FrameOptions& options)
{
    h.BeginFrame();
    h.dev->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    transparent_fog_checks::DrawBackdropFor(h, v, options);
    Image image = Capture(h.dev);
    h.dev->EndScene();
    h.dev->Present(nullptr, nullptr, nullptr, nullptr);
    return image;
}

bool SameStockFog(const engine::StockFog& a, float start, float end)
{
    for (int group = 0; group < engine::kDayNightFogGroupCount; ++group)
        if (a.start[group] != start || a.end[group] != end)
            return false;
    return true;
}

void CheckSwitchedOffFogLeavesTheFrameToTheClient(Harness& h)
{
    using namespace transparent_fog_checks;
    Config saved;
    vf_test_get_config(&saved);
    const View v;
    engine::StockFog clientFog = {{kStockFogStart, kStockFogStart}, {kStockFogEnd, kStockFogEnd}};
    vf_test_hook_stock_fog(nullptr, &clientFog);
    const Config cfg = WithToggleKey(HookConfig(true));
    Config counted = cfg;
    counted.logLevel = static_cast<int>(LogLevel::Debug);
    const HookedFrame on = RenderSettledHookedFrame(h, v, cfg, FrameOptions());

    vf_test_flip_luxvelum_switch();
    WorldTextCapture text;
    text.dev = h.dev;
    FrameOptions withText;
    withText.worldText = &text;
    const HookedFrame firstOff = RenderHookedFrame(h, v, cfg, withText);
    const Image firstText = text.image;
    const HookedFrame off = RenderHookedFrame(h, v, cfg, withText);
    const char* fogReason = "";
    const bool fogDrawn = vf_test_fog_status(&fogReason) != 0;
    const size_t statsStart = water_settings_checks::DllLogSize();
    vf_test_log_transparent_fog_stats();
    RenderHookedFrame(h, v, counted, FrameOptions());
    const std::string offStats = runtime_cost::LogWrittenSince(statsStart);
    const Image client = RenderTheClientAlone(h, v, FrameOptions());

    vf_test_flip_luxvelum_switch();
    const size_t resumedStart = water_settings_checks::DllLogSize();
    RenderHookedFrame(h, v, counted, FrameOptions());
    const std::string resumedStats = runtime_cost::LogWrittenSince(resumedStart);
    const HookedFrame back = RenderSettledHookedFrame(h, v, cfg, FrameOptions());
    WorldTextCapture onText;
    onText.dev = h.dev;
    FrameOptions withOnText;
    withOnText.worldText = &onText;
    RenderHookedFrame(h, v, cfg, withOnText);
    const int fogOverClient = LargestDifference(on.image, client, v.world);
    const int firstOffVsClient = LargestDifference(firstOff.image, client, v.world);
    const int offVsClient = LargestDifference(off.image, client, v.world);
    const int textVsClient =
        std::max(LargestDifference(firstText, client, v.world), LargestDifference(text.image, client, v.world));
    const int backVsOn = LargestDifference(back.image, on.image, v.world);
    std::printf("     switched off: frames vs the client alone %d/255 and %d/255 (%d/255 at the world-name text), fog "
                "\"%s\"; on: fog over the client %d/255, switched on again vs before %d/255\n",
                firstOffVsClient, offVsClient, textVsClient, fogReason, fogOverClient, backVsOn);
    Check(fogOverClient > 0 && firstOffVsClient == 0 && offVsClient == 0 && textVsClient == 0 && text.draws == 2,
          "switched off, the hooked frames draw no fog from the first frame on, neither before the world-name text "
          "nor after the world, and the frame is the client's own");
    Check(SameStockFog(firstOff.stockFogInFrame, kStockFogStart, kStockFogEnd) && PassedThrough(firstOff) &&
              SameStockFog(off.stockFogInFrame, kStockFogStart, kStockFogEnd) && PassedThrough(off) &&
              firstOff.glarePassesBeforeTheFog == 0 && firstOff.glarePassesAtOwnCall == 1 &&
              off.glarePassesBeforeTheFog == 0 && off.glarePassesAtOwnCall == 1,
          "switched off, the client's stock fog stays in place from the first frame on, the M2 batch fog arguments "
          "stay bit-identical and the glare draws at its own call");
    Check(text.depthWrite == TRUE && onText.depthWrite == FALSE,
          "switched off, the world-name text keeps the client's depth writes; switched on, they are suppressed again");
    Check(!fogDrawn && std::strcmp(fogReason, kSwitchedOff) == 0,
          "the settings window reports the fog as not drawn because Luxvelum is switched off");
    Check(offStats.find("transparent fog:") == std::string::npos &&
              runtime_cost::HasLine(resumedStats, "transparent fog: 1 frames, 0 early composites"),
          "at LogLevel 2 the M2 batch fog is not counted and no transparent fog summary is logged while Luxvelum is "
          "switched off; switched on again, the counting starts afresh");
    Check(backVsOn == 0 && Rewritten(back) && SameStockFog(back.stockFogInFrame, kPushedStockFogStart,
                                                           kPushedStockFogEnd),
          "switched on again, the same frame is drawn as before, with the stock fog replaced and the early composite "
          "armed");
    vf_test_set_config(&saved);
}

void CheckSwitchedOffWaterIsReleased(Harness& h)
{
    using water_checks::BasinClient;
    using water_checks::WaterCalls;
    using water_checks::WaterFrame;
    using water_checks::WaterFrameResult;
    Config saved;
    vf_test_get_config(&saved);
    BasinClient client(h, water_checks::DefaultWaterView());
    const Config on = WithToggleKey(water_checks::WaterConfig(saved));
    Config waterOff = on;
    waterOff.water = false;
    vf_test_set_water_seconds(water_checks::kFrameSeconds);
    WaterFrame frame;
    frame.calls = WaterCalls::Hooks;
    vf_test_set_config(&waterOff);
    const WaterFrameResult stock = client.Render(frame);
    const unsigned heldWithWaterOff = vf_test_water_resources_held();
    vf_test_set_config(&on);
    const WaterFrameResult before = client.Render(frame);
    FlipAndLatch();
    const WaterFrameResult off = client.Render(frame);
    const unsigned held = vf_test_water_resources_held();
    const char* reason = "";
    const bool drawn = vf_test_water_status(&reason) != 0;
    FlipAndLatch();
    const WaterFrameResult after = client.Render(frame);
    std::printf("     water switched off: began %d, resources held 0x%X, status \"%s\"\n", off.began, held, reason);
    Check(heldWithWaterOff == 0 && before.began && !off.began && held == 0 && !drawn &&
              std::strcmp(reason, kSwitchedOff) == 0 && water_checks::SameImage(off.image, stock.image),
          "switched off, the water pass stays the client's, as with Water=0, and its copies, wave maps and foam masks "
          "are released at the end of the frame");
    Check(after.began && water_checks::SameImage(before.image, after.image),
          "switched on again, the water pass recreates them and renders the same frame");
    vf_test_set_water_seconds(water_checks::kRealTime);
    vf_test_set_config(&saved);
}

void CheckSwitchedOffGivesBackTheSpriteGate(Harness& h)
{
    using namespace client_sprite_checks;
    Config saved;
    vf_test_get_config(&saved);
    const Config on = WithToggleKey(water_checks::RippleConfig(saved, 1.0f));
    BasinClient client(h, water_checks::DefaultWaterView());
    SpriteFrames frames{client, on};
    g_gate = kPlayerChoice;
    const bool bound = BindGate(ClientCodeWithGuards());
    const int32_t held = frames.After(on);
    FlipAndLatch();
    const int32_t switchedOff = frames.After(on);
    FlipAndLatch();
    const int32_t heldAgain = frames.After(on);
    std::printf("     waterRipples %d: %d held, %d switched off, %d switched on again\n", kPlayerChoice, held,
                switchedOff, heldAgain);
    Check(bound && held == 0 && switchedOff == kPlayerChoice && heldAgain == 0,
          "switched off, the client gets its waterRipples value back at the end of the frame, and switched on again "
          "it is held once more");
    vf_test_bind_client_ripple_gate(nullptr, 0, nullptr, 0);
    vf_test_set_water_seconds(water_checks::kRealTime);
    vf_test_set_config(&saved);
}

void CheckToggleKeyInput(Harness& h)
{
    Config saved;
    vf_test_get_config(&saved);
    Config keyed = saved;
    keyed.toggleKey = kCtrlF9;
    vf_test_set_config(&keyed);
    g_clientInput = {};
    const SwitchState start = CurrentSwitch();
    PressHotkey(h.window, kCtrlF9);
    const SwitchState pressed = CurrentSwitch();
    const bool settingsClosed = vf_test_overlay_visible() == 0;
    PressHotkey(h.window, kCtrlF9);
    const SwitchState again = CurrentSwitch();
    Check(!start.flippedOff && pressed.flippedOff && !again.flippedOff && settingsClosed &&
              g_clientInput.keyDowns == 0 && g_clientInput.keyUps == 0,
          "ToggleKey switches Luxvelum off and on again, without reaching the client or opening the settings window");

    const Hotkey overlayKey = keyed.overlayKey;
    HoldHotkeyDown(h.window, overlayKey);
    const bool opened = vf_test_overlay_visible() == 1;
    const ClientInput beforeTap = g_clientInput;
    PressKey(h.window, kCtrlF9.virtualKey);
    const ClientInput afterTap = g_clientInput;
    SendMessageW(h.window, WM_KEYUP, overlayKey.virtualKey, KeyReleasedParameter(overlayKey.virtualKey));
    PressHotkey(h.window, overlayKey);
    Check(opened && afterTap.keyDowns == beforeTap.keyDowns + 1 && afterTap.keyUps == beforeTap.keyUps + 1 &&
              !CurrentSwitch().flippedOff && vf_test_overlay_visible() == 0,
          "while the OverlayKey is held, the ToggleKey's key without its modifiers goes down and up in the client, so "
          "neither hotkey swallows the other's key-up and no game key sticks");

    g_clientInput = {};
    Config unset = saved;
    unset.toggleKey = kNoHotkey;
    vf_test_set_config(&unset);
    PressHotkey(h.window, kCtrlF9);
    Check(!CurrentSwitch().flippedOff && g_clientInput.keyDowns == 1,
          "without a ToggleKey the same key reaches the client and Luxvelum stays on");

    Config withoutOverlay = keyed;
    withoutOverlay.overlay = false;
    vf_test_set_config(&withoutOverlay);
    PressHotkey(h.window, kCtrlF9);
    const bool offWithoutOverlay = CurrentSwitch().flippedOff;
    PressHotkey(h.window, kCtrlF9);
    Check(offWithoutOverlay && !CurrentSwitch().flippedOff && g_clientInput.keyDowns == 1,
          "with Overlay=0 ToggleKey still switches Luxvelum off and on");
    vf_test_latch_luxvelum_switch();
    vf_test_set_config(&saved);
}

bool SwitchedOffAfterAFrame()
{
    vf_test_hook_frame_begin();
    vf_test_hook_frame_end();
    return CurrentSwitch().latchedOff;
}

void CheckSwitchedOnWhenNothingCouldSwitchItOn()
{
    Config saved;
    vf_test_get_config(&saved);
    Config keyless = saved;
    keyless.overlay = false;
    keyless.toggleKey = kNoHotkey;
    const Config keyed = WithToggleKey(keyless);
    vf_test_set_config(&keyed);
    vf_test_flip_luxvelum_switch();
    const bool offWhileTheKeyIsSet = SwitchedOffAfterAFrame();
    const size_t start = water_settings_checks::DllLogSize();
    vf_test_set_config(&keyless);
    const bool offWithoutAKey = SwitchedOffAfterAFrame();
    const std::string text = runtime_cost::LogWrittenSince(start);
    const SwitchState after = CurrentSwitch();
    std::printf("%s", text.c_str());
    Check(offWhileTheKeyIsSet && !offWithoutAKey && !after.flippedOff &&
              runtime_cost::HasLine(text, "Luxvelum switched on because neither ToggleKey nor the settings window "
                                          "could switch it on again") &&
              runtime_cost::HasLine(text, "Luxvelum switched on again"),
          "with Overlay=0, taking the ToggleKey away while Luxvelum is switched off switches it on at the next frame, "
          "so it never stays off with no way back");
    vf_test_set_config(&saved);
}
}
