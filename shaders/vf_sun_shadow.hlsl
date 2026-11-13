#include "vf_integrate.hlsli"

float4 main(float2 lowResTexel : VPOS) : COLOR0
{
    float2 pixel = LowResTexelToFullPixel(lowResTexel);
    float depth = SampleDepth(sDepth, pixel);
    float3 viewRay = ViewRayAtUnitDepth(pixel);
    float distancePerViewZ = length(viewRay);
    float viewZ = LinearDepth(depth);
    float horizonBlend = max(BeyondFarClip(depth) ? 1 : 0, smoothstep(HorizonBlendStart(), FarClip(), viewZ));
    viewZ = lerp(viewZ, MaxFogDistance(), horizonBlend);
    float marchLength = min(viewZ * distancePerViewZ, MaxFogDistance());
    return SunVisibility(viewRay / distancePerViewZ, marchLength, StepJitter(lowResTexel)).xxxx;
}
