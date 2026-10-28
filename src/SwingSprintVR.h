#pragma once

#include "Config.h"
#include "ModBase.h"
#include "SwingDetector.h"

namespace swing_sprint
{
    class SwingSprintVR : public ModBase
    {
    public:
        SwingSprintVR()
            : ModBase(Settings(Version::PROJECT, Version::NAME, &g_config, 32, true))
        {}

    protected:
        virtual void onModLoaded(const F4SE::LoadInterface* f4SE) override;
        virtual void onGameLoaded() override;
        virtual void onGameSessionLoaded() override;
        virtual void onFrameUpdate() override;

    private:
        void updateCellSpeed(RE::PlayerCharacter* player);
        void updateSwingSprint(float dt);
        void setSprintPressed(bool pressed);
        void updateMovedSpeed(RE::PlayerCharacter* player, float dt);
        void drawLiveStats() const;

        // what the live stats show, filled while updating
        struct LiveStats
        {
            bool interior = false;
            float targetSpeedMult = 100.0f;
            float speedMult = 100.0f;
            float movedSpeed = 0.0f;
            float leftHandSpeed = 0.0f;
            float rightHandSpeed = 0.0f;
        };

        SwingDetector _swing;
        bool _sprintPressed = false;
        bool _sprintActive = false;
        float _tapTimeLeft = 0.0f;
        std::chrono::steady_clock::time_point _lastFrame{};
        float _mcmCheckTimer = 0.0f;
        LiveStats _stats;
        RE::NiPoint3 _lastPos{};
        bool _hasLastPos = false;
    };

    // The ONE global to rule them ALL
    inline SwingSprintVR g_swingSprint;
}
