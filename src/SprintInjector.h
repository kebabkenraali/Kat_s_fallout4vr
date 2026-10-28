#pragma once

#include <atomic>

#include "vrcf/VRControllersSuppressor.h"

namespace kat_loco
{
    /**
     * Presses a controller button for the game without the player touching it.
     *
     * Hooks IVRSystem::GetControllerState[WithPose] (vtable slots 34/35) on top of whatever is already there
     * (the framework's own suppression hook and other mods' hooks), calls through to it, then ORs the forced
     * button into the state the game reads.
     *
     * Threading: the hook runs on the OpenVR polling thread and reads only atomics. setPressed / update are
     * called from the main thread. Never call F4SE / CommonLibF4VR from the hook.
     */
    class SprintInjector
    {
    public:
        // Install once the framework's own hook is installed, so ours wraps it. Retries every frame until done.
        void update(vr::TrackedDeviceIndex_t leftIndex, vr::TrackedDeviceIndex_t rightIndex);

        // Press (or release) the button on the given physical controller.
        void setPressed(bool leftController, vr::EVRButtonId button, bool pressed);

        // Release every forced button on both controllers.
        void releaseAll();

        bool isInstalled() const { return _installed; }

    private:
        bool install();

        static void applyTo(vr::TrackedDeviceIndex_t index, vr::VRControllerState_t* state);

        static bool hookedGetControllerState(vr::IVRSystem* system, vr::TrackedDeviceIndex_t index, vr::VRControllerState_t* state, uint32_t stateSize);
        static bool hookedGetControllerStateWithPose(vr::IVRSystem* system, vr::ETrackingUniverseOrigin origin, vr::TrackedDeviceIndex_t index, vr::VRControllerState_t* state,
            uint32_t stateSize, vr::TrackedDevicePose_t* pose);

        using GetControllerState_t = bool (*)(vr::IVRSystem*, vr::TrackedDeviceIndex_t, vr::VRControllerState_t*, uint32_t);
        using GetControllerStateWithPose_t = bool (*)(vr::IVRSystem*, vr::ETrackingUniverseOrigin, vr::TrackedDeviceIndex_t, vr::VRControllerState_t*, uint32_t,
            vr::TrackedDevicePose_t*);

        static inline GetControllerState_t _origGetControllerState = nullptr;
        static inline GetControllerStateWithPose_t _origGetControllerStateWithPose = nullptr;

        // buttons forced down per physical controller, read by the hook
        static inline std::atomic<uint64_t> _leftButtons{ 0 };
        static inline std::atomic<uint64_t> _rightButtons{ 0 };
        static inline std::atomic<vr::TrackedDeviceIndex_t> _leftIndex{ vr::k_unTrackedDeviceIndexInvalid };
        static inline std::atomic<vr::TrackedDeviceIndex_t> _rightIndex{ vr::k_unTrackedDeviceIndexInvalid };

        bool _installed = false;
    };

    inline SprintInjector g_sprintInjector;
}
