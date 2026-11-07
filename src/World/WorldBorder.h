#pragma once
#include <string>

class WorldBorder
{
public:
    static constexpr double MAX_SIZE = 59999968.0;
    static constexpr double MAX_CENTER = 29999984.0;
    static constexpr double MAX_RADIUS = 29999984.0;

    enum class Stage { Stationary, Growing, Shrinking };

    double centerX() const { return m_centerX; }
    double centerZ() const { return m_centerZ; }
    double size() const;
    double target() const { return m_to; }
    double secondsLeft() const { return m_moving ? m_duration - m_elapsed : 0.0; }
    Stage stage() const;
    unsigned int color() const;
    double shrinkingSpeed() const;

    double west() const;
    double east() const;
    double north() const;
    double south() const;

    double distanceInside(double x, double z) const;
    bool contains(double x, double z, double margin = 0.0) const;
    bool containsBlock(int x, int z) const { return contains(x, z); }
    bool blocks(double x, double z, double width) const;
    float damageFor(double x, double z) const;
    float warning(double x, double z) const;

    void setCenter(double x, double z);
    void setSize(double size);
    void lerpTo(double from, double to, double seconds);
    void update(double seconds);

    double damagePerBlock = 0.2;
    double safeZone = 5.0;
    int warningBlocks = 5;
    int warningTime = 15;

    std::string save() const;
    bool load(const std::string& text);
    bool isDefault() const;

private:
    double m_centerX = 0.0;
    double m_centerZ = 0.0;
    double m_from = MAX_SIZE;
    double m_to = MAX_SIZE;
    double m_duration = 0.0;
    double m_elapsed = 0.0;
    bool m_moving = false;
};
