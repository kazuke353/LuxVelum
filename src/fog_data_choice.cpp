#include "fog_data_choice.h"

#include "log.h"

#include <windows.h>

#include <algorithm>

namespace
{
constexpr const char* kCandidatePattern = "fogdata*.bin";

std::string FolderOf(const std::string& path)
{
    const size_t slash = path.find_last_of("\\/");
    return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}

float MatchShare(const FogDataScore& score, size_t probes)
{
    return probes == 0 ? 0.0f : static_cast<float>(score.matched) / static_cast<float>(probes);
}

bool SamePath(const std::string& a, const std::string& b)
{
    return CompareStringA(LOCALE_INVARIANT, NORM_IGNORECASE, a.c_str(), -1, b.c_str(), -1) == CSTR_EQUAL;
}
}

FogDataScore ScoreFogData(const std::vector<FogBandProbe>& probes, const BandReader& read)
{
    FogDataScore score;
    for (const FogBandProbe& probe : probes)
    {
        uint32_t rgb = 0;
        if (!read(probe.bandId, probe.keyIndex, rgb))
            continue;
        ++score.read;
        if (rgb == probe.rgb)
            ++score.matched;
    }
    return score;
}

int ChooseFogData(const std::vector<FogDataCandidate>& candidates, const BandReader& read,
                  std::vector<FogDataScore>& scores)
{
    scores.clear();
    bool anyRead = false;
    for (const FogDataCandidate& candidate : candidates)
    {
        scores.push_back(ScoreFogData(candidate.probes, read));
        anyRead = anyRead || scores.back().read > 0;
    }
    if (!anyRead)
        return -1;
    int best = 0;
    float bestShare = MatchShare(scores[0], candidates[0].probes.size());
    for (size_t i = 1; i < candidates.size(); ++i)
    {
        const float share = MatchShare(scores[i], candidates[i].probes.size());
        if (share > bestShare)
        {
            best = static_cast<int>(i);
            bestShare = share;
        }
    }
    return bestShare > kFogDataMatchShare ? best : 0;
}

std::vector<FogDataCandidate> FindFogDataCandidates(const std::string& defaultPath)
{
    std::vector<FogDataCandidate> candidates(1);
    candidates[0].path = defaultPath;
    ReadFogBandProbes(defaultPath, candidates[0].probes);
    const std::string folder = FolderOf(defaultPath);
    WIN32_FIND_DATAA found = {};
    const HANDLE search = FindFirstFileA((folder + kCandidatePattern).c_str(), &found);
    if (search == INVALID_HANDLE_VALUE)
        return candidates;
    do
    {
        const std::string path = folder + found.cFileName;
        if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || SamePath(path, defaultPath))
            continue;
        FogDataCandidate candidate;
        candidate.path = path;
        if (ReadFogBandProbes(path, candidate.probes) && !candidate.probes.empty())
            candidates.push_back(candidate);
    } while (FindNextFileA(search, &found));
    FindClose(search);
    std::sort(candidates.begin() + 1, candidates.end(),
              [](const FogDataCandidate& a, const FogDataCandidate& b) { return a.path < b.path; });
    return candidates;
}

void FogDataChoice::Prepare(const std::string& defaultPath)
{
    m_candidates = FindFogDataCandidates(defaultPath);
    m_decided = m_candidates.size() < 2;
    if (!m_decided)
        VF_LOG_INFO("fog data: %zu files to choose from by the client's LightIntBand once it is loaded",
                    m_candidates.size());
}

void FogDataChoice::Decide(FogData& data, const BandReader& read)
{
    if (m_decided)
        return;
    std::vector<FogDataScore> scores;
    const int chosen = ChooseFogData(m_candidates, read, scores);
    if (chosen < 0)
        return;
    m_decided = true;
    for (size_t i = 0; i < m_candidates.size(); ++i)
        VF_LOG_INFO("fog data: %s matches %zu of %zu band probes (%zu read)%s", m_candidates[i].path.c_str(),
                    scores[i].matched, m_candidates[i].probes.size(), scores[i].read,
                    static_cast<int>(i) == chosen ? ", chosen" : "");
    if (!SamePath(m_candidates[chosen].path, data.Path()))
        data.Load(m_candidates[chosen].path);
}

FogDataChoice& GlobalFogDataChoice()
{
    static FogDataChoice choice;
    return choice;
}
