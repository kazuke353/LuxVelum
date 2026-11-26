#include "celestial_occlusion.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kSettledShare = 1.0e-3f;
}

bool CelestialOcclusion::SameLight(bool moon, int mapId) const
{
    return m_observed && moon == m_moon && mapId == m_mapId;
}

bool CelestialOcclusion::SightLineDue(double seconds, bool moon, int mapId) const
{
    const double sinceObserved = seconds - m_observedAt;
    return !SameLight(moon, mapId) || !(sinceObserved >= 0.0 && sinceObserved < kCelestialSightLineSeconds);
}

void CelestialOcclusion::Observe(double seconds, bool moon, int mapId, const CelestialSightLine& sight)
{
    const bool blocked = sight.traced && sight.blocked;
    m_target = blocked ? 1.0f : 0.0f;
    if (blocked)
        m_occluderDistance = sight.farthestBlockAt;
    if (!SameLight(moon, mapId))
    {
        m_hidden = m_target;
        m_advancedTo = seconds;
    }
    m_observed = true;
    m_moon = moon;
    m_mapId = mapId;
    m_observedAt = seconds;
}

void CelestialOcclusion::Advance(double seconds)
{
    const double elapsed = std::max(seconds - m_advancedTo, 0.0);
    m_advancedTo = seconds;
    m_hidden += (m_target - m_hidden) * static_cast<float>(1.0 - std::exp(-elapsed / kCelestialFadeSeconds));
    if (std::fabs(m_target - m_hidden) < kSettledShare)
        m_hidden = m_target;
}
