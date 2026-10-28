#include "SwingDetector.h"

#include <algorithm>
#include <cmath>

namespace kat_loco
{
    namespace
    {
        float length(const Vec3& v)
        {
            return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        }

        float dot(const Vec3& a, const Vec3& b)
        {
            return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        }
    }

    /**
     * Running arms move fast and in opposite directions. The raw activity of a frame is the slower hand's
     * speed (both arms must move), zeroed when the hands move the same way if alternating is required.
     * The activity is smoothed so the turnaround points of each swing, where the hands stop for a moment,
     * don't drop the sprint, and start/stop use separate thresholds with a stop delay to avoid flicker.
     */
    bool SwingDetector::update(const float dt, const Vec3& leftVelocity, const Vec3& rightVelocity, const SwingSettings& settings)
    {
        if (dt <= 0.0f) {
            return _swinging;
        }

        const float leftSpeed = length(leftVelocity);
        const float rightSpeed = length(rightVelocity);
        float raw = (std::min)(leftSpeed, rightSpeed);
        if (settings.requireAlternatingArms && dot(leftVelocity, rightVelocity) > 0.0f) {
            raw = 0.0f;
        }

        const float alpha = settings.smoothing > 0.0f ? 1.0f - std::exp(-dt / settings.smoothing) : 1.0f;
        _activity += (raw - _activity) * alpha;

        if (!_swinging) {
            if (_activity >= settings.startSpeed) {
                _swinging = true;
                _belowStopTime = 0.0f;
            }
        } else if (_activity < settings.stopSpeed) {
            _belowStopTime += dt;
            if (_belowStopTime >= settings.stopDelay) {
                _swinging = false;
            }
        } else {
            _belowStopTime = 0.0f;
        }
        return _swinging;
    }

    void SwingDetector::reset()
    {
        _activity = 0.0f;
        _belowStopTime = 0.0f;
        _swinging = false;
    }
}
