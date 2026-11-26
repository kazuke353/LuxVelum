#include "dxvk_vendor.h"

#include "log.h"

#include <windows.h>

#include <cctype>
#include <cstring>

namespace
{
constexpr DWORD kMaxDxvkConfigLength = 4096;
constexpr const char* kNvidiaDriverLibrary = "nvapi.dll";

bool ContainsIgnoringCase(const std::string& text, const char* part)
{
    const size_t length = std::strlen(part);
    if (length > text.size())
        return false;
    for (size_t start = 0; start + length <= text.size(); ++start)
    {
        size_t i = 0;
        while (i < length && std::tolower(static_cast<unsigned char>(text[start + i])) ==
                                 std::tolower(static_cast<unsigned char>(part[i])))
            ++i;
        if (i == length)
            return true;
    }
    return false;
}

bool NvidiaDriverInstalled()
{
    char systemFolder[MAX_PATH] = {};
    const UINT length = GetSystemDirectoryA(systemFolder, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return false;
    const std::string nvapi = std::string(systemFolder) + "\\" + kNvidiaDriverLibrary;
    return GetFileAttributesA(nvapi.c_str()) != INVALID_FILE_ATTRIBUTES;
}

std::string CurrentDxvkConfig()
{
    std::string value(kMaxDxvkConfigLength, '\0');
    const DWORD length = GetEnvironmentVariableA(kDxvkConfigVariable, value.data(), kMaxDxvkConfigLength);
    value.resize(length < kMaxDxvkConfigLength ? length : 0);
    return value;
}
}

bool DxvkConfigSetsVendor(const std::string& dxvkConfig)
{
    return ContainsIgnoringCase(dxvkConfig, kDxvkVendorOption);
}

std::string DxvkConfigReportingAmd(const std::string& dxvkConfig)
{
    if (dxvkConfig.empty())
        return kDxvkAmdVendorSetting;
    return dxvkConfig + "; " + kDxvkAmdVendorSetting;
}

DxvkVendorDecision DecideDxvkVendor(const DxvkVendorInputs& inputs)
{
    if (!inputs.asked)
        return {false, "", "DxvkAmdVendor=0"};
    if (!inputs.multisampling)
        return {false, "", "Multisampling=0"};
    if (!inputs.nvidiaDriver)
        return {false, "", "no NVIDIA driver (nvapi.dll) in the system folder, so DXVK offers RESZ already"};
    if (inputs.d3d9Loaded)
        return {false, "", "d3d9.dll was loaded before Luxvelum, so DXVK has read its settings already"};
    if (DxvkConfigSetsVendor(inputs.dxvkConfig))
        return {false, "", "DXVK_CONFIG already sets d3d9.customVendorId"};
    return {true, DxvkConfigReportingAmd(inputs.dxvkConfig), ""};
}

void ReportAmdToDxvkWhenAsked(bool asked, bool multisampling)
{
    if (!asked)
        return;
    DxvkVendorInputs inputs;
    inputs.asked = asked;
    inputs.multisampling = multisampling;
    inputs.d3d9Loaded = GetModuleHandleA("d3d9.dll") != nullptr;
    inputs.nvidiaDriver = NvidiaDriverInstalled();
    inputs.dxvkConfig = CurrentDxvkConfig();
    const DxvkVendorDecision decision = DecideDxvkVendor(inputs);
    if (!decision.reportAmd)
    {
        VF_LOG_INFO("DXVK vendor: left as it is (%s)", decision.reason);
        return;
    }
    if (!SetEnvironmentVariableA(kDxvkConfigVariable, decision.dxvkConfig.c_str()))
    {
        VF_LOG_ERROR("DXVK vendor: setting %s failed with Windows error %lu", kDxvkConfigVariable, GetLastError());
        return;
    }
    VF_LOG_INFO("DXVK vendor: %s=\"%s\" before d3d9.dll loads, so DXVK reports AMD and offers RESZ for "
                "multisampling",
                kDxvkConfigVariable, decision.dxvkConfig.c_str());
}
