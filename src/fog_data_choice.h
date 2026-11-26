#pragma once

#include "fog_data.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

using BandReader = std::function<bool(uint32_t bandId, uint32_t keyIndex, uint32_t& rgb)>;

struct FogDataCandidate
{
    std::string path;
    std::vector<FogBandProbe> probes;
};

struct FogDataScore
{
    size_t matched = 0;
    size_t read = 0;
};

constexpr float kFogDataMatchShare = 0.5f;

FogDataScore ScoreFogData(const std::vector<FogBandProbe>& probes, const BandReader& read);
int ChooseFogData(const std::vector<FogDataCandidate>& candidates, const BandReader& read,
                  std::vector<FogDataScore>& scores);
std::vector<FogDataCandidate> FindFogDataCandidates(const std::string& defaultPath);

class FogDataChoice
{
public:
    void Prepare(const std::string& defaultPath);
    bool Decided() const { return m_decided; }
    void Decide(FogData& data, const BandReader& read);

private:
    std::vector<FogDataCandidate> m_candidates;
    bool m_decided = true;
};

FogDataChoice& GlobalFogDataChoice();
