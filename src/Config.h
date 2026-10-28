#pragma once

#include <filesystem>

#include "ConfigBase.h"
#include "Resources.h"
#include "common/CommonUtils.h"
#include "vrcf/VRControllersManager.h"

namespace swing_sprint
{
    static const auto BASE_MOD_PATH = BASE_PATH + "\\" + std::string(Version::PROJECT);
    static const auto INI_PATH = BASE_MOD_PATH + "\\" + std::string(Version::PROJECT) + ".ini";

    // How the sprint button is fed to the game while the arms swing
    enum class SprintMode : std::uint8_t
    {
        // the button is held down for as long as the arms swing (game sprints while the button is held)
        Hold,
        // a short press when swinging starts and another when it stops (game toggles sprint on each press)
        Tap,
    };

    class Config : public ConfigBase
    {
    public:
        explicit Config()
            : ConfigBase(Version::PROJECT, INI_PATH, IDR_CONFIG_INI)
        {}

        // Speed by cell type
        bool speedByCellEnabled = true;
        float interiorSpeedMult = 80.0f;
        float exteriorSpeedMult = 100.0f;
        bool speedRefreshNudge = true;

        // Sprint by swinging arms
        bool swingSprintEnabled = true;
        vrcf::Hand sprintHand = vrcf::Hand::Offhand;
        vr::EVRButtonId sprintButton = vr::k_EButton_SteamVR_Touchpad;
        SprintMode sprintMode = SprintMode::Hold;
        float sprintTapDuration = 0.15f;
        bool requireAlternatingArms = true;
        float swingStartSpeed = 1.4f;
        float swingStopSpeed = 0.8f;
        float swingSmoothing = 0.25f;
        float sprintStopDelay = 0.4f;

        // Testing: live stats floating in front of the player
        bool showLiveStats = false;

        // true when MCM's settings files were found and override the values above
        bool usingMcm = false;

        // Reload the MCM values if MCM saved a change since the last load. Main thread.
        void checkMcmSettingsChanged();

    protected:
        virtual void loadIniConfigInternal(const CSimpleIniA& ini) override;

    private:
        void loadValues(const CSimpleIniA& ini, const char* section);
        void loadMcmSettings();

        std::filesystem::file_time_type _mcmDefaultsTime{};
        std::filesystem::file_time_type _mcmUserTime{};
    };

    // Global singleton for easy access
    inline Config g_config;
}
