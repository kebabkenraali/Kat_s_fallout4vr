#include "KATLocomotionVR.h"

#include "SprintInjector.h"
#include "debug/DebugDraw.h"
#include "f4vr/F4VRUtils.h"

// This is the entry point to the mod.
extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_skse, F4SE::PluginInfo* a_info)
{
    return g_mod->onF4SEPluginQuery(a_skse, a_info);
}

// This is the entry point to the mod.
extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
    return g_mod->onF4SEPluginLoad(a_f4se);
}

namespace kat_loco
{
    namespace
    {
        // the player's base speed multiplier when the mod is off
        constexpr float DEFAULT_SPEED_MULT = 100.0f;

        Vec3 toVec3(const vr::HmdVector3_t& v)
        {
            return { v.v[0], v.v[1], v.v[2] };
        }

        Vec3 subtract(const Vec3& a, const Vec3& b)
        {
            return { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
        }

        float length(const Vec3& v)
        {
            return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        }

        // Bethesda game units per meter
        constexpr float GAME_UNITS_PER_METER = 70.0f;

        /**
         * A text bar of the swing speed: '#' up to the current speed, with '|' marking the stop and start thresholds.
         */
        std::string swingBar(const float activity, const float stopSpeed, const float startSpeed)
        {
            constexpr int WIDTH = 24;
            const float scale = (std::max)(startSpeed * 1.5f, 0.1f);
            const auto toColumn = [&](const float value) { return std::clamp(static_cast<int>(value / scale * WIDTH), 0, WIDTH - 1); };
            std::string bar(WIDTH, '.');
            if (activity > 0.01f) {
                for (int i = 0; i <= toColumn(activity); ++i) {
                    bar[i] = '#';
                }
            }
            bar[toColumn(stopSpeed)] = '|';
            bar[toColumn(startSpeed)] = '|';
            return "[" + bar + "]";
        }

        // Menus where swinging the arms must not sprint (the arms move for other reasons there)
        bool isSprintBlockingMenuOpen()
        {
            const auto ui = RE::UI::GetSingleton();
            return ui && (ui->GetMenuOpen("PipboyMenu") || ui->GetMenuOpen("PauseMenu") || ui->GetMenuOpen("LoadingMenu") || ui->GetMenuOpen("WorkshopMenu") ||
                             ui->GetMenuOpen("DialogueMenu"));
        }

        // The physical controller of a logical hand, honoring the game's left-handed mode
        bool isLeftController(const vrcf::Hand hand)
        {
            switch (hand) {
            case vrcf::Hand::Left:
                return true;
            case vrcf::Hand::Right:
                return false;
            case vrcf::Hand::Offhand:
                return !f4vr::isLeftHandedMode();
            case vrcf::Hand::Primary:
            default:
                return f4vr::isLeftHandedMode();
            }
        }
    }

    /**
     * Run F4SE plugin load and initialize the plugin given the init handle.
     */
    void KATLocomotionVR::onModLoaded(const F4SE::LoadInterface*)
    {
        //noop
    }

    /**
     * On game fully loaded initialize things that should be initialized only once.
     */
    void KATLocomotionVR::onGameLoaded()
    {
        //noop
    }

    /**
     * Game session can be initialized multiple times as it is fired on new game and save loaded events.
     */
    void KATLocomotionVR::onGameSessionLoaded()
    {
        _swing.reset();
        setSprintPressed(false);
        _sprintActive = false;
        _tapTimeLeft = 0.0f;
        _hasLastPos = false;
    }

    /**
     * On every frame if player is initialized.
     */
    void KATLocomotionVR::onFrameUpdate()
    {
        const auto now = std::chrono::steady_clock::now();
        // clamp so a hitch or a loading screen doesn't look like a long swing
        const float dt = _lastFrame.time_since_epoch().count() == 0 ? 0.0f : (std::min)(std::chrono::duration<float>(now - _lastFrame).count(), 0.1f);
        _lastFrame = now;

        const auto player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->loadedData) {
            logger::sample(3000, "no player data - noop");
            return;
        }

        // MCM saves its values to a file; check it twice a second so a change applies while the menu is still open
        _mcmCheckTimer += dt;
        if (_mcmCheckTimer >= 0.5f) {
            _mcmCheckTimer = 0.0f;
            g_config.checkMcmSettingsChanged();
        }

        updateCellSpeed(player);
        updateSwingSprint(dt);
        updateMovedSpeed(player, dt);
        if (g_config.showLiveStats) {
            drawLiveStats();
        }
    }

    /**
     * Set the player's base SpeedMult to the configured interior or exterior value. Checked every frame, so cell
     * changes and INI edits (hot-reload) apply right away. Only writes when the value differs.
     */
    void KATLocomotionVR::updateCellSpeed(RE::PlayerCharacter* player)
    {
        const auto cell = player->GetParentCell();
        if (!cell) {
            return;
        }
        const auto actorValues = RE::ActorValue::GetSingleton();
        if (!actorValues || !actorValues->speedMult) {
            return;
        }

        const bool interior = cell->IsInterior();
        const float target = !g_config.speedByCellEnabled ? DEFAULT_SPEED_MULT : interior ? g_config.interiorSpeedMult : g_config.exteriorSpeedMult;
        const float current = player->GetBaseActorValue(*actorValues->speedMult);
        _stats.interior = interior;
        _stats.targetSpeedMult = target;
        _stats.speedMult = player->GetActorValue(*actorValues->speedMult);
        if (std::abs(current - target) < 0.01f) {
            return;
        }

        logger::info("Set SpeedMult {:.1f} -> {:.1f} ({})", current, target, interior ? "interior" : "exterior");
        player->SetBaseActorValue(*actorValues->speedMult, target);

        // the game only recalculates movement speed when carry weight changes, so nudge it
        if (g_config.speedRefreshNudge && actorValues->carryWeight) {
            player->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *actorValues->carryWeight, 0.1f);
            player->ModActorValue(RE::ACTOR_VALUE_MODIFIER::kTemporary, *actorValues->carryWeight, -0.1f);
        }
    }

    /**
     * Read both controllers' velocities from the compositor's last poses, detect running-like arm swinging, and
     * press the game's sprint button while it lasts.
     */
    void KATLocomotionVR::updateSwingSprint(const float dt)
    {
        auto* system = vr::VRSystem();
        auto* compositor = vr::VRCompositor();
        if (!system || !compositor) {
            return;
        }
        const auto leftIndex = system->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
        const auto rightIndex = system->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
        g_sprintInjector.update(leftIndex, rightIndex);

        if (!g_config.swingSprintEnabled || isSprintBlockingMenuOpen() || leftIndex == vr::k_unTrackedDeviceIndexInvalid ||
            rightIndex == vr::k_unTrackedDeviceIndexInvalid) {
            _swing.reset();
            setSprintPressed(false);
            _sprintActive = false;
            _tapTimeLeft = 0.0f;
            return;
        }

        vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
        if (compositor->GetLastPoses(poses, vr::k_unMaxTrackedDeviceCount, nullptr, 0) != vr::VRCompositorError_None) {
            return;
        }
        const auto& hmd = poses[vr::k_unTrackedDeviceIndex_Hmd];
        const auto& left = poses[leftIndex];
        const auto& right = poses[rightIndex];
        if (!hmd.bPoseIsValid || !left.bPoseIsValid || !right.bPoseIsValid) {
            return;
        }

        // relative to the headset so walking in place (head bobbing, KAT) doesn't count as swinging
        const Vec3 hmdVelocity = toVec3(hmd.vVelocity);
        const Vec3 leftVelocity = subtract(toVec3(left.vVelocity), hmdVelocity);
        const Vec3 rightVelocity = subtract(toVec3(right.vVelocity), hmdVelocity);
        _stats.leftHandSpeed = length(leftVelocity);
        _stats.rightHandSpeed = length(rightVelocity);

        const SwingSettings settings{
            .requireAlternatingArms = g_config.requireAlternatingArms,
            .startSpeed = g_config.swingStartSpeed,
            .stopSpeed = g_config.swingStopSpeed,
            .smoothing = g_config.swingSmoothing,
            .stopDelay = g_config.sprintStopDelay,
        };
        const bool swinging = _swing.update(dt, leftVelocity, rightVelocity, settings);

        if (swinging != _sprintActive) {
            _sprintActive = swinging;
            logger::info("Arm swing sprint {} (activity {:.2f} m/s)", swinging ? "start" : "stop", _swing.getActivity());
            if (g_config.sprintMode == SprintMode::Tap) {
                _tapTimeLeft = g_config.sprintTapDuration;
            }
        }

        if (g_config.sprintMode == SprintMode::Hold) {
            setSprintPressed(_sprintActive);
        } else {
            _tapTimeLeft = (std::max)(0.0f, _tapTimeLeft - dt);
            setSprintPressed(_tapTimeLeft > 0.0f);
        }
    }

    /**
     * The player's actual horizontal movement speed, from the position change between frames, smoothed. Shows in
     * the live stats how the speed settings and the sprint really change the movement.
     */
    void KATLocomotionVR::updateMovedSpeed(RE::PlayerCharacter* player, const float dt)
    {
        const auto pos = player->GetPosition();
        if (_hasLastPos && dt > 0.0f) {
            const float dx = pos.x - _lastPos.x;
            const float dy = pos.y - _lastPos.y;
            const float speed = std::sqrt(dx * dx + dy * dy) / GAME_UNITS_PER_METER / dt;
            // a teleport, door or fast travel is not movement
            if (speed < 50.0f) {
                _stats.movedSpeed += (speed - _stats.movedSpeed) * (1.0f - std::exp(-dt / 0.3f));
            }
        }
        _lastPos = pos;
        _hasLastPos = true;
    }

    /**
     * Live stats floating in front of the player: what the mod sees and does this very frame, so the effect of a
     * setting changed in MCM or the INI can be watched while moving.
     */
    void KATLocomotionVR::drawLiveStats() const
    {
        using namespace render::colors;
        auto& dd = debug::dd();
        const float activity = _swing.getActivity();

        dd.watch("Settings from", g_config.usingMcm ? "MCM" : "INI");
        dd.watch("Location", _stats.interior ? "Interior" : "Exterior", _stats.interior ? Cyan : Green);
        dd.watch("Speed target", g_config.speedByCellEnabled ? std::format("{:.0f}", _stats.targetSpeedMult) : std::string("off (100)"));
        dd.watch("SpeedMult now", std::format("{:.0f}", _stats.speedMult), std::abs(_stats.speedMult - _stats.targetSpeedMult) < 0.5f ? White : Orange);
        dd.watch("Moving speed", std::format("{:.2f} m/s", _stats.movedSpeed));
        if (!g_config.swingSprintEnabled) {
            dd.watch("Arm swing sprint", "off", Grey);
            return;
        }
        dd.watch("Hands L / R", std::format("{:.2f} / {:.2f} m/s", _stats.leftHandSpeed, _stats.rightHandSpeed));
        dd.watch("Swing speed", std::format("{:.2f} m/s", activity), _sprintActive ? Green : activity >= g_config.swingStopSpeed ? Yellow : White);
        dd.watch("Swing bar", swingBar(activity, g_config.swingStopSpeed, g_config.swingStartSpeed), _sprintActive ? Green : Grey);
        dd.watch("Stop / start", std::format("{:.2f} / {:.2f} m/s", g_config.swingStopSpeed, g_config.swingStartSpeed));
        dd.watch("Sprint", _sprintActive ? "ON" : "off", _sprintActive ? Green : Grey);
        dd.watch("Button pressed", _sprintPressed ? "yes" : "no", _sprintPressed ? Green : Grey);
    }

    void KATLocomotionVR::setSprintPressed(const bool pressed)
    {
        if (pressed == _sprintPressed) {
            return;
        }
        _sprintPressed = pressed;
        // release everything so a hand/button change in the INI can't leave a button stuck
        g_sprintInjector.releaseAll();
        if (pressed) {
            g_sprintInjector.setPressed(isLeftController(g_config.sprintHand), g_config.sprintButton, true);
        }
    }
}
