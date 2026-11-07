#include "WorldBorder.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>

double WorldBorder::size() const
{
    if (!m_moving || m_duration <= 0.0) return m_to;
    const double t = std::clamp(m_elapsed / m_duration, 0.0, 1.0);
    return m_from + (m_to - m_from) * t;
}

WorldBorder::Stage WorldBorder::stage() const
{
    if (!m_moving) return Stage::Stationary;
    return m_to < m_from ? Stage::Shrinking : Stage::Growing;
}

unsigned int WorldBorder::color() const
{
    switch (stage())
    {
        case Stage::Growing: return 0x40FF80u;
        case Stage::Shrinking: return 0xFF3030u;
        default: return 0x20A0FFu;
    }
}

double WorldBorder::shrinkingSpeed() const
{
    if (!m_moving || m_duration <= 0.0) return 0.0;
    return std::abs(m_from - m_to) / (m_duration * 1000.0);
}

double WorldBorder::west() const
{
    return std::clamp(m_centerX - size() / 2.0, -MAX_RADIUS, MAX_RADIUS);
}

double WorldBorder::east() const
{
    return std::clamp(m_centerX + size() / 2.0, -MAX_RADIUS, MAX_RADIUS);
}

double WorldBorder::north() const
{
    return std::clamp(m_centerZ - size() / 2.0, -MAX_RADIUS, MAX_RADIUS);
}

double WorldBorder::south() const
{
    return std::clamp(m_centerZ + size() / 2.0, -MAX_RADIUS, MAX_RADIUS);
}

double WorldBorder::distanceInside(double x, double z) const
{
    const double toNorth = z - north();
    const double toSouth = south() - z;
    const double toWest = x - west();
    const double toEast = east() - x;
    return std::min(std::min(toWest, toEast), std::min(toNorth, toSouth));
}

bool WorldBorder::contains(double x, double z, double margin) const
{
    return x >= west() - margin && x < east() + margin && z >= north() - margin && z < south() + margin;
}

bool WorldBorder::blocks(double x, double z, double width) const
{
    const double reach = std::max(width, 1.0);
    return distanceInside(x, z) < reach * 2.0 && contains(x, z, reach);
}

float WorldBorder::damageFor(double x, double z) const
{
    const double outside = distanceInside(x, z) + safeZone;
    if (outside >= 0.0 || damagePerBlock <= 0.0) return 0.0f;
    return static_cast<float>(std::max(1.0, std::floor(-outside * damagePerBlock)));
}

float WorldBorder::warning(double x, double z) const
{
    const double inside = distanceInside(x, z);
    const double moving = std::min(shrinkingSpeed() * warningTime * 1000.0, std::abs(m_to - size()));
    const double reach = std::max(static_cast<double>(warningBlocks), moving);
    if (reach <= 0.0 || inside >= reach) return 0.0f;
    return static_cast<float>(std::clamp(1.0 - inside / reach, 0.0, 1.0));
}

void WorldBorder::setCenter(double x, double z)
{
    m_centerX = x;
    m_centerZ = z;
}

void WorldBorder::setSize(double value)
{
    m_from = m_to = std::clamp(value, 1.0, MAX_SIZE);
    m_moving = false;
    m_duration = m_elapsed = 0.0;
}

void WorldBorder::lerpTo(double from, double to, double seconds)
{
    if (seconds <= 0.0 || from == to)
    {
        setSize(to);
        return;
    }
    m_from = std::clamp(from, 1.0, MAX_SIZE);
    m_to = std::clamp(to, 1.0, MAX_SIZE);
    m_duration = seconds;
    m_elapsed = 0.0;
    m_moving = true;
}

void WorldBorder::update(double seconds)
{
    if (!m_moving) return;
    m_elapsed += seconds;
    if (m_elapsed >= m_duration) setSize(m_to);
}

std::string WorldBorder::save() const
{
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), "%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%.17g,%d,%d",
                  m_centerX, m_centerZ, size(), m_to, secondsLeft(), damagePerBlock, safeZone, warningBlocks, warningTime);
    return buffer;
}

bool WorldBorder::load(const std::string& text)
{
    std::stringstream in(text);
    std::string part;
    double values[9] = {};
    int count = 0;
    while (count < 9 && std::getline(in, part, ','))
    {
        char* end = nullptr;
        values[count] = std::strtod(part.c_str(), &end);
        if (end == part.c_str() || !std::isfinite(values[count])) return false;
        ++count;
    }
    if (count != 9) return false;
    *this = WorldBorder();
    setCenter(std::clamp(values[0], -MAX_CENTER, MAX_CENTER), std::clamp(values[1], -MAX_CENTER, MAX_CENTER));
    lerpTo(values[2], values[3], values[4]);
    damagePerBlock = std::max(0.0, values[5]);
    safeZone = std::max(0.0, values[6]);
    warningBlocks = std::max(0, static_cast<int>(values[7]));
    warningTime = std::max(0, static_cast<int>(values[8]));
    return true;
}

bool WorldBorder::isDefault() const
{
    const WorldBorder fresh;
    return m_centerX == fresh.m_centerX && m_centerZ == fresh.m_centerZ && size() == fresh.size() && !m_moving &&
           damagePerBlock == fresh.damagePerBlock && safeZone == fresh.safeZone && warningBlocks == fresh.warningBlocks &&
           warningTime == fresh.warningTime;
}
