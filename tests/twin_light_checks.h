#pragma once

namespace twin_light_checks
{
constexpr uintptr_t kFirstTorchNode = 0x20000;
constexpr uintptr_t kTorchNodeStride = 0x70;
constexpr float kTorchReach = 1.0f;
constexpr PointLightUpload kGammaUpload = {false, 1.0f, kTorchReach};

uintptr_t TorchNode(uint32_t index)
{
    return kFirstTorchNode + index * kTorchNodeStride;
}

LocalPointLight Torch(uintptr_t id, float x, float z, float peak = 1.0f)
{
    LocalPointLight light;
    light.position[0] = x;
    light.position[2] = z;
    light.color[0] = peak;
    light.color[1] = peak * 0.6f;
    light.color[2] = peak * 0.3f;
    light.attenuation[0] = 0.0f;
    light.attenuation[1] = 0.7f;
    light.attenuation[2] = 0.03f;
    light.nativeId = id;
    return light;
}

LocalLightInputs PoolOf(const std::vector<LocalPointLight>& lights)
{
    LocalLightInputs inputs;
    const float camera[3] = {};
    for (const LocalPointLight& light : lights)
        engine::SelectLocalPointLight(inputs, light, camera, kGammaUpload);
    return inputs;
}

uint32_t Merge(LocalLightInputs& inputs)
{
    const float camera[3] = {};
    return engine::MergeTwinPointLights(inputs, camera, kTorchReach);
}

LocalLightInputs MergedPoolOf(const std::vector<LocalPointLight>& lights)
{
    LocalLightInputs inputs = PoolOf(lights);
    Merge(inputs);
    return inputs;
}

bool SamePool(const LocalLightInputs& a, const LocalLightInputs& b)
{
    return a.candidateCount == b.candidateCount && a.pointLightCount == b.pointLightCount &&
           a.mergedTwins == b.mergedTwins &&
           std::memcmp(a.candidates, b.candidates, a.candidateCount * sizeof(LocalPointLight)) == 0 &&
           std::memcmp(a.pointLights, b.pointLights, a.pointLightCount * sizeof(LocalPointLight)) == 0;
}

bool Uploaded(const LocalLightInputs& inputs, uintptr_t id)
{
    for (uint32_t i = 0; i < inputs.pointLightCount; ++i)
        if (inputs.pointLights[i].nativeId == id)
            return true;
    return false;
}

void CheckTwinTorchesBecomeOneLight()
{
    const LocalPointLight left = Torch(TorchNode(1), -0.6f, 20.0f);
    const LocalPointLight right = Torch(TorchNode(0), 0.6f, 20.0f);
    LocalLightInputs inputs = PoolOf({left, right});
    const LocalPointLight single = inputs.candidates[0];
    const uint32_t absorbed = Merge(inputs);
    const LocalPointLight& merged = inputs.candidates[0];
    const float summed[3] = {2.0f, 1.2f, 0.6f};
    Check(absorbed == 1 && inputs.mergedTwins == 1 && inputs.candidateCount == 1 && inputs.pointLightCount == 1 &&
              LocalNear(merged.position[0], 0.0f) && LocalNear(merged.position[2], 20.0f) &&
              LocalNear(merged.uploadedColor[0], summed[0]) && LocalNear(merged.uploadedColor[1], summed[1]) &&
              LocalNear(merged.uploadedColor[2], summed[2]) && LocalNear(merged.color[0], 1.0f) &&
              merged.nativeId == TorchNode(0) && merged.mergedLights == 2 &&
              LocalNear(merged.cutoff, engine::PointLightCutoff(summed, merged.attenuation, kTorchReach)) &&
              merged.cutoff > single.cutoff &&
              std::memcmp(&inputs.pointLights[0], &merged, sizeof(merged)) == 0,
          "two torch lights 1.2 yd apart with the same colour and attenuation become one light at their midpoint "
          "with their summed colour, the lower node id and the reach of that sum");

    LocalLightInputs weighted = MergedPoolOf({Torch(TorchNode(0), 0.0f, 20.0f, 1.0f),
                                              Torch(TorchNode(1), 1.5f, 20.0f, 0.5f)});
    Check(weighted.candidateCount == 1 && LocalNear(weighted.candidates[0].position[0], 0.5f) &&
              LocalNear(weighted.candidates[0].uploadedColor[0], 1.5f),
          "a twin of half the brightness pulls the merged light a third of the way toward itself");
}

void CheckDistinctLightsStaySeparate()
{
    LocalLightInputs apart = PoolOf({Torch(TorchNode(0), 0.0f, 20.0f), Torch(TorchNode(1), 6.3f, 20.0f)});
    LocalLightInputs within = PoolOf({Torch(TorchNode(0), 0.0f, 20.0f), Torch(TorchNode(1), 3.0f, 20.0f)});
    LocalPointLight weaker = Torch(TorchNode(1), 1.0f, 20.0f);
    weaker.attenuation[1] = 0.9f;
    LocalLightInputs attenuated = PoolOf({Torch(TorchNode(0), 0.0f, 20.0f), weaker});
    LocalPointLight blue = Torch(TorchNode(1), 1.0f, 20.0f);
    std::swap(blue.color[0], blue.color[2]);
    LocalLightInputs coloured = PoolOf({Torch(TorchNode(0), 0.0f, 20.0f), blue});
    Check(Merge(apart) == 0 && apart.candidateCount == 2 && Merge(within) == 0 && within.candidateCount == 2 &&
              Merge(attenuated) == 0 && attenuated.candidateCount == 2 && Merge(coloured) == 0 &&
              coloured.candidateCount == 2 && coloured.mergedTwins == 0,
          "torches 6.3 or 3 yd apart, and lights 1 yd apart with 0.7 against 0.9 linear attenuation or another "
          "colour stay separate lights");
}

void CheckTwinClustersStayCompact()
{
    LocalLightInputs triple = MergedPoolOf(
        {Torch(TorchNode(0), 0.0f, 20.0f), Torch(TorchNode(1), 1.0f, 20.0f), Torch(TorchNode(2), 2.0f, 20.0f)});
    LocalLightInputs chain = MergedPoolOf({Torch(TorchNode(0), 0.0f, 20.0f), Torch(TorchNode(1), 2.0f, 20.0f),
                                           Torch(TorchNode(2), 4.0f, 20.0f), Torch(TorchNode(3), 6.0f, 20.0f)});
    Check(triple.candidateCount == 1 && triple.candidates[0].mergedLights == 3 && triple.mergedTwins == 2 &&
              LocalNear(triple.candidates[0].position[0], 1.0f) && chain.candidateCount == 4 &&
              chain.mergedTwins == 0,
          "three lights within 2 yd merge into one, but a chain of lights 2 yd apart spanning 6 yd stays separate");
}

LocalLightInputs Reordered(const LocalLightInputs& ranked, const std::vector<uint32_t>& order)
{
    LocalLightInputs inputs = ranked;
    for (uint32_t i = 0; i < order.size(); ++i)
        inputs.candidates[i] = ranked.candidates[order[i]];
    return inputs;
}

void CheckMergeIgnoresPoolOrder()
{
    std::vector<LocalPointLight> lights;
    for (uint32_t i = 0; i < 6; ++i)
    {
        const float x = 7.0f * static_cast<float>(i);
        lights.push_back(Torch(TorchNode(2 * i), x - 0.4f, 25.0f, 1.0f + 0.1f * static_cast<float>(i)));
        lights.push_back(Torch(TorchNode(2 * i + 1), x + 0.5f, 25.3f, 0.9f));
    }
    const LocalLightInputs ranked = PoolOf(lights);
    std::vector<uint32_t> reversed;
    std::vector<uint32_t> interleaved;
    for (uint32_t i = 0; i < ranked.candidateCount; ++i)
    {
        reversed.push_back(ranked.candidateCount - 1 - i);
        interleaved.push_back((i * 5) % ranked.candidateCount);
    }
    LocalLightInputs forward = ranked;
    LocalLightInputs backward = Reordered(ranked, reversed);
    LocalLightInputs shuffled = Reordered(ranked, interleaved);
    Merge(forward);
    Merge(backward);
    Merge(shuffled);
    Check(forward.candidateCount == 6 && forward.mergedTwins == 6 && SamePool(forward, backward) &&
              SamePool(forward, shuffled),
          "six torch pairs merge into the same six lights, ranked the same, whatever order the pool holds them in");
}

LocalPointLight TorchAround(uintptr_t id, float degrees, float radius, float peak)
{
    const float radians = degrees * 3.14159265f / 180.0f;
    return Torch(id, radius * std::sin(radians), radius * std::cos(radians), peak);
}

void CheckMergedTwinsAreRankedAgain()
{
    std::vector<LocalPointLight> lights;
    for (uint32_t i = 0; i < 9; ++i)
        lights.push_back(TorchAround(TorchNode(10 + i), 40.0f * static_cast<float>(i), 12.0f, 1.0f));
    lights.push_back(TorchAround(TorchNode(0), 18.0f, 12.0f, 0.7f));
    lights.push_back(TorchAround(TorchNode(1), 22.0f, 12.0f, 0.7f));
    LocalLightInputs inputs = PoolOf(lights);
    const bool twinsOutranked = !Uploaded(inputs, TorchNode(0)) && !Uploaded(inputs, TorchNode(1));
    Merge(inputs);
    Check(twinsOutranked && inputs.candidateCount == 10 && inputs.pointLightCount == kMaxLocalPointLights &&
              inputs.pointLights[0].nativeId == TorchNode(0) && inputs.pointLights[0].mergedLights == 2,
          "two dim twin torches that each lost their slot to brighter single lights take the first slot once merged");
}

void CheckTwinsAtThePoolEdgeStillMerge()
{
    std::vector<LocalPointLight> lights;
    for (uint32_t i = 0; i + 1 < kLocalLightCandidatePool; ++i)
        lights.push_back(TorchAround(TorchNode(10 + i), 15.0f * static_cast<float>(i), 30.0f, 1.0f));
    lights.push_back(TorchAround(TorchNode(0), 343.0f, 30.0f, 0.7f));
    lights.push_back(TorchAround(TorchNode(1), 347.0f, 30.0f, 0.7f));
    LocalLightInputs inputs = PoolOf(lights);
    const bool bothPooled = inputs.candidateCount == kLocalLightCandidatePool + 1;
    Merge(inputs);
    engine::KeepStrongestCandidates(inputs);
    Check(bothPooled && inputs.candidateCount == kLocalLightCandidatePool &&
              inputs.pointLights[0].nativeId == TorchNode(0) && inputs.pointLights[0].mergedLights == 2,
          "a dim torch pair ranked 24th and 25th behind 23 brighter single lights is captured whole, merges and "
          "takes the first slot, and the pool is cut back to 24 after the merge");
}

void CheckTwinTorchesShareTheEightSlots()
{
    using namespace local_light_stabiliser_checks;
    const LocalLightCamera camera = ForwardCamera();
    LocalLightStabiliser stabiliser;
    bool everyTorchLit = true;
    bool steady = true;
    for (int frame = 0; frame <= 3 * kFadeFrames; ++frame)
    {
        std::vector<LocalPointLight> lights;
        for (uint32_t torch = 0; torch < kMaxLocalPointLights; ++torch)
        {
            const float x = 7.0f * static_cast<float>(torch) - 24.5f;
            const float flicker = (frame + torch) % 2 ? 1.15f : 0.85f;
            lights.push_back(Torch(TorchNode(2 * torch), x - 0.6f, 40.0f, flicker));
            lights.push_back(Torch(TorchNode(2 * torch + 1), x + 0.6f, 40.0f, flicker));
        }
        for (uint32_t single = 0; single < 4; ++single)
            lights.push_back(Torch(TorchNode(100 + single), 7.0f * static_cast<float>(single) - 10.5f, 45.0f, 1.5f));
        LocalLightInputs inputs = CaptureInView(lights, camera);
        engine::MergeTwinPointLights(inputs, camera.position, kTorchReach);
        engine::KeepStrongestCandidates(inputs);
        StepFrame(stabiliser, camera, frame * kStabiliserFrameSeconds, inputs);
        if (frame < 2 * kFadeFrames)
            continue;
        for (uint32_t torch = 0; torch < kMaxLocalPointLights; ++torch)
        {
            const LocalPointLight* uploaded = UploadedLight(inputs, TorchNode(2 * torch));
            everyTorchLit = everyTorchLit && uploaded && uploaded->mergedLights == 2;
        }
        steady = steady && inputs.pointLightCount == kMaxLocalPointLights;
    }
    Check(everyTorchLit && steady && stabiliser.Activity().evictions == 0,
          "eight torch pairs flickering by 15 % beside four brighter single lights fill the eight slots with all "
          "sixteen torch lights, one slot per pair, without a swap");
}

void CheckTwinLights()
{
    CheckTwinTorchesBecomeOneLight();
    CheckDistinctLightsStaySeparate();
    CheckTwinClustersStayCompact();
    CheckMergeIgnoresPoolOrder();
    CheckMergedTwinsAreRankedAgain();
    CheckTwinsAtThePoolEdgeStillMerge();
    CheckTwinTorchesShareTheEightSlots();
}
}
