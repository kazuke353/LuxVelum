#pragma once

namespace dxvk_vendor_checks
{
DxvkVendorInputs AskedWithMultisampling()
{
    DxvkVendorInputs inputs;
    inputs.asked = true;
    inputs.multisampling = true;
    inputs.d3d9Loaded = false;
    inputs.nvidiaDriver = true;
    return inputs;
}

void CheckDxvkVendor()
{
    const DxvkVendorDecision bare = DecideDxvkVendor(AskedWithMultisampling());
    DxvkVendorInputs withOptions = AskedWithMultisampling();
    withOptions.dxvkConfig = "dxgi.syncInterval = 0";
    const DxvkVendorDecision appended = DecideDxvkVendor(withOptions);
    std::printf("     DXVK vendor: \"%s\"; with other options \"%s\"\n", bare.dxvkConfig.c_str(),
                appended.dxvkConfig.c_str());
    Check(bare.reportAmd && bare.dxvkConfig == "d3d9.customVendorId = 1002" && appended.reportAmd &&
              appended.dxvkConfig == "dxgi.syncInterval = 0; d3d9.customVendorId = 1002",
          "with DxvkAmdVendor=1, Multisampling=1 and an NVIDIA driver, before d3d9.dll loads, DXVK_CONFIG asks DXVK to "
          "report AMD and keeps the options it already holds");

    DxvkVendorInputs notAsked = AskedWithMultisampling();
    notAsked.asked = false;
    DxvkVendorInputs noMultisampling = AskedWithMultisampling();
    noMultisampling.multisampling = false;
    DxvkVendorInputs loaded = AskedWithMultisampling();
    loaded.d3d9Loaded = true;
    DxvkVendorInputs withoutNvidia = AskedWithMultisampling();
    withoutNvidia.nvidiaDriver = false;
    DxvkVendorInputs ownVendor = AskedWithMultisampling();
    ownVendor.dxvkConfig = "D3D9.CustomVendorId = 10de";
    const DxvkVendorInputs leftAlone[] = {notAsked, noMultisampling, withoutNvidia, loaded, ownVendor};
    bool anyReported = false;
    for (const DxvkVendorInputs& inputs : leftAlone)
    {
        const DxvkVendorDecision decision = DecideDxvkVendor(inputs);
        anyReported = anyReported || decision.reportAmd || !decision.dxvkConfig.empty() || !decision.reason[0];
    }
    Check(!anyReported,
          "DXVK keeps its vendor with DxvkAmdVendor=0, with Multisampling=0, without an NVIDIA driver, when d3d9.dll "
          "was loaded before Luxvelum and when DXVK_CONFIG already sets d3d9.customVendorId, each with its reason");

}
}
