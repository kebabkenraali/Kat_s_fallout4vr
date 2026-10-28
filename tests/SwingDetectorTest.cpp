// Standalone test for SwingDetector (no game, no framework). Build: cl /std:c++20 /EHsc /I..\src SwingDetectorTest.cpp ..\src\SwingDetector.cpp
#include "SwingDetector.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace kat_loco;

namespace
{
    int g_failures = 0;

    void check(const bool condition, const char* what)
    {
        std::printf("%s %s\n", condition ? "PASS" : "FAIL", what);
        if (!condition) {
            ++g_failures;
        }
    }

    constexpr float DT = 1.0f / 90.0f;
    constexpr float PI = 3.14159265f;

    // Runs `seconds` of arm motion: each hand swings forward/back along z with the given peak speed and frequency.
    // alternating=true moves the hands in opposite directions, like running.
    void run(SwingDetector& detector, const SwingSettings& settings, const float seconds, const float peakSpeed, const float frequency, const bool alternating, float& time)
    {
        for (float t = 0.0f; t < seconds; t += DT) {
            const float v = peakSpeed * std::sin(2.0f * PI * frequency * time);
            detector.update(DT, { 0.0f, 0.0f, v }, { 0.0f, 0.0f, alternating ? -v : v }, settings);
            time += DT;
        }
    }
}

int main()
{
    const SwingSettings settings;

    {
        SwingDetector d;
        float time = 0.0f;
        run(d, settings, 2.0f, 0.0f, 1.5f, true, time);
        check(!d.isSwinging(), "still hands do not sprint");
    }
    {
        SwingDetector d;
        float time = 0.0f;
        run(d, settings, 2.0f, 3.0f, 1.5f, true, time);
        check(d.isSwinging(), "vigorous alternating swing sprints");
        run(d, settings, 2.0f, 0.0f, 1.5f, true, time);
        check(!d.isSwinging(), "sprint stops after the hands stop");
    }
    {
        SwingDetector d;
        float time = 0.0f;
        run(d, settings, 2.0f, 3.0f, 1.5f, false, time);
        check(!d.isSwinging(), "both hands moving the same way do not sprint");
    }
    {
        SwingDetector d;
        float time = 0.0f;
        run(d, settings, 2.0f, 1.0f, 1.0f, true, time);
        check(!d.isSwinging(), "gentle walking arm swing does not sprint");
    }
    {
        SwingDetector d;
        float time = 0.0f;
        run(d, settings, 2.0f, 3.0f, 1.5f, true, time);
        // a short pause shorter than the stop delay keeps the sprint
        run(d, settings, 0.2f, 0.0f, 1.5f, true, time);
        check(d.isSwinging(), "short pause keeps sprinting");
    }

    std::printf("%d failure(s)\n", g_failures);
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
