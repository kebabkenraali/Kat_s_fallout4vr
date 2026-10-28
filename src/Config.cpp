#include "Config.h"

#include <filesystem>

#include "vrcf/InputBindingParser.h"

using namespace common;

namespace
{
    const char* DEFAULT_SECTION = Version::PROJECT.data();

    // MCM keeps its values in its own INI files, relative to the game folder (MO2's virtual Data folder works the same)
    const char* MCM_SECTION = "Main";
    const std::filesystem::path MCM_DEFAULTS_PATH = "Data\\MCM\\Config\\SwingSprintVR\\settings.ini";
    const std::filesystem::path MCM_USER_PATH = "Data\\MCM\\Settings\\SwingSprintVR.ini";
    const std::filesystem::path MCM_PLUGIN_PATH = "Data\\F4SE\\Plugins\\mcm_vr.dll";

    // The sprint button choices of the MCM dropdown, in the order of its options
    struct SprintButtonOption
    {
        f4cf::vrcf::Hand hand;
        vr::EVRButtonId button;
    };

    constexpr SprintButtonOption SPRINT_BUTTON_OPTIONS[] = {
        { f4cf::vrcf::Hand::Offhand, vr::k_EButton_SteamVR_Touchpad },
        { f4cf::vrcf::Hand::Offhand, vr::k_EButton_Grip },
        { f4cf::vrcf::Hand::Offhand, vr::k_EButton_SteamVR_Trigger },
        { f4cf::vrcf::Hand::Offhand, vr::k_EButton_A },
        { f4cf::vrcf::Hand::Offhand, vr::k_EButton_ApplicationMenu },
        { f4cf::vrcf::Hand::Primary, vr::k_EButton_SteamVR_Touchpad },
        { f4cf::vrcf::Hand::Primary, vr::k_EButton_Grip },
        { f4cf::vrcf::Hand::Primary, vr::k_EButton_A },
        { f4cf::vrcf::Hand::Primary, vr::k_EButton_ApplicationMenu },
    };

    /**
     * Parse "<hand> <button>" (e.g. "offhand thumbstick") into the sprint hand and button, keeping the
     * current values for any part that doesn't parse.
     */
    void parseSprintButton(const std::string& text, f4cf::vrcf::Hand& hand, vr::EVRButtonId& button)
    {
        const auto space = text.find(' ');
        if (space == std::string::npos) {
            logger::warn("Invalid sSprintButton '{}', expected '<hand> <button>'", text);
            return;
        }
        const auto parsedHand = f4cf::vrcf::parseHand(std::string_view(text).substr(0, space));
        const auto parsedButton = f4cf::vrcf::parseButton(std::string_view(text).substr(space + 1));
        if (!parsedHand || !parsedButton) {
            logger::warn("Invalid sSprintButton '{}', expected '<hand> <button>'", text);
            return;
        }
        hand = *parsedHand;
        button = *parsedButton;
    }

    std::filesystem::file_time_type lastWriteTime(const std::filesystem::path& path)
    {
        std::error_code ec;
        const auto time = std::filesystem::last_write_time(path, ec);
        return ec ? std::filesystem::file_time_type{} : time;
    }
}

namespace swing_sprint
{
    void Config::loadIniConfigInternal(const CSimpleIniA& ini)
    {
        loadValues(ini, DEFAULT_SECTION);
        parseSprintButton(ini.GetValue(DEFAULT_SECTION, "sSprintButton", "offhand thumbstick"), sprintHand, sprintButton);
        const std::string mode = ini.GetValue(DEFAULT_SECTION, "sSprintMode", "hold");
        sprintMode = _stricmp(mode.c_str(), "tap") == 0 ? SprintMode::Tap : SprintMode::Hold;

        // MCM, when installed, has the last word on the values it shows
        loadMcmSettings();
    }

    /**
     * Read the values that share a key name between the mod INI and MCM. A key missing from the section keeps
     * the current value, so MCM's files only override what they contain.
     */
    void Config::loadValues(const CSimpleIniA& ini, const char* section)
    {
        speedByCellEnabled = ini.GetBoolValue(section, "bSpeedByCellEnabled", speedByCellEnabled);
        interiorSpeedMult = static_cast<float>(ini.GetDoubleValue(section, "fInteriorSpeedMult", interiorSpeedMult));
        exteriorSpeedMult = static_cast<float>(ini.GetDoubleValue(section, "fExteriorSpeedMult", exteriorSpeedMult));
        speedRefreshNudge = ini.GetBoolValue(section, "bSpeedRefreshNudge", speedRefreshNudge);

        swingSprintEnabled = ini.GetBoolValue(section, "bSwingSprintEnabled", swingSprintEnabled);
        sprintTapDuration = static_cast<float>(ini.GetDoubleValue(section, "fSprintTapDuration", sprintTapDuration));
        requireAlternatingArms = ini.GetBoolValue(section, "bRequireAlternatingArms", requireAlternatingArms);
        swingStartSpeed = static_cast<float>(ini.GetDoubleValue(section, "fSwingStartSpeed", swingStartSpeed));
        swingStopSpeed = static_cast<float>(ini.GetDoubleValue(section, "fSwingStopSpeed", swingStopSpeed));
        swingSmoothing = static_cast<float>(ini.GetDoubleValue(section, "fSwingSmoothing", swingSmoothing));
        sprintStopDelay = static_cast<float>(ini.GetDoubleValue(section, "fSprintStopDelay", sprintStopDelay));
        // bShowSwingDebug is the old name of bShowLiveStats
        showLiveStats = ini.GetBoolValue(section, "bShowSwingDebug", showLiveStats);
        showLiveStats = ini.GetBoolValue(section, "bShowLiveStats", showLiveStats);
    }

    /**
     * Apply MCM's defaults and then the player's MCM changes on top of the current values. MCM only writes
     * the values the player changed to its Settings file, so both files are needed.
     */
    void Config::loadMcmSettings()
    {
        _mcmDefaultsTime = lastWriteTime(MCM_DEFAULTS_PATH);
        _mcmUserTime = lastWriteTime(MCM_USER_PATH);
        usingMcm = false;

        // our MCM files ship with the mod, so only let them override the INI when MCM itself is installed
        std::error_code ec;
        if (!std::filesystem::exists(MCM_PLUGIN_PATH, ec)) {
            return;
        }

        for (const auto& path : { MCM_DEFAULTS_PATH, MCM_USER_PATH }) {
            CSimpleIniA ini;
            if (ini.LoadFile(path.c_str()) < 0) {
                continue;
            }
            usingMcm = true;
            loadValues(ini, MCM_SECTION);

            const auto buttonIndex = ini.GetLongValue(MCM_SECTION, "iSprintButton", -1);
            if (buttonIndex >= 0 && buttonIndex < static_cast<long>(std::size(SPRINT_BUTTON_OPTIONS))) {
                sprintHand = SPRINT_BUTTON_OPTIONS[buttonIndex].hand;
                sprintButton = SPRINT_BUTTON_OPTIONS[buttonIndex].button;
            }
            const auto modeIndex = ini.GetLongValue(MCM_SECTION, "iSprintMode", -1);
            if (modeIndex >= 0) {
                sprintMode = modeIndex == 1 ? SprintMode::Tap : SprintMode::Hold;
            }
        }
        if (usingMcm) {
            logger::info("Loaded MCM settings: interior {:.0f}, exterior {:.0f}, swing start {:.2f} stop {:.2f}", interiorSpeedMult, exteriorSpeedMult, swingStartSpeed,
                swingStopSpeed);
        }
    }

    /**
     * Reload when MCM wrote a change. Cheap enough to call every frame-ish: two file time reads.
     */
    void Config::checkMcmSettingsChanged()
    {
        if (lastWriteTime(MCM_DEFAULTS_PATH) != _mcmDefaultsTime || lastWriteTime(MCM_USER_PATH) != _mcmUserTime) {
            loadMcmSettings();
        }
    }
}
