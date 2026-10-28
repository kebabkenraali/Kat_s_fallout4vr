#pragma once

#include <array>

namespace kat_loco
{
    using Vec3 = std::array<float, 3>;

    struct SwingSettings
    {
        // only count swings where the two hands move in opposite directions (like running arms)
        bool requireAlternatingArms = true;
        // smoothed hand speed (m/s, relative to the headset) that starts the sprint
        float startSpeed = 1.4f;
        // smoothed hand speed below which the sprint stops (after stopDelay)
        float stopSpeed = 0.8f;
        // time constant of the smoothing, in seconds
        float smoothing = 0.25f;
        // how long the speed must stay below stopSpeed before the sprint stops, in seconds
        float stopDelay = 0.4f;
    };

    /**
     * Detects running-like arm swinging from the two controllers' velocities.
     * Pure logic, no game or OpenVR calls, so it can be unit tested.
     */
    class SwingDetector
    {
    public:
        // Feed one frame: dt in seconds, each hand's velocity relative to the headset in m/s.
        // Returns true while the player is swinging the arms enough to sprint.
        bool update(float dt, const Vec3& leftVelocity, const Vec3& rightVelocity, const SwingSettings& settings);

        void reset();

        bool isSwinging() const { return _swinging; }

        // the smoothed swing speed, for tuning the thresholds
        float getActivity() const { return _activity; }

    private:
        float _activity = 0.0f;
        float _belowStopTime = 0.0f;
        bool _swinging = false;
    };
}
