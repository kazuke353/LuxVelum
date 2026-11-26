#pragma once

constexpr double kCelestialSightLineSeconds = 0.05;
constexpr double kCelestialFadeSeconds = 0.2;

struct CelestialSightLine
{
    bool traced = false;
    bool blocked = false;
    float farthestBlockAt = 0.0f;
};

struct CelestialShadow
{
    float hidden = 0.0f;
    float occluderDistance = 0.0f;
};

class CelestialOcclusion
{
public:
    bool SightLineDue(double seconds, bool moon, int mapId) const;
    void Observe(double seconds, bool moon, int mapId, const CelestialSightLine& sight);
    void Advance(double seconds);
    CelestialShadow Shadow() const { return {m_hidden, m_occluderDistance}; }

private:
    bool SameLight(bool moon, int mapId) const;

    bool m_observed = false;
    bool m_moon = false;
    int m_mapId = -1;
    double m_observedAt = 0.0;
    double m_advancedTo = 0.0;
    float m_target = 0.0f;
    float m_hidden = 0.0f;
    float m_occluderDistance = 0.0f;
};
