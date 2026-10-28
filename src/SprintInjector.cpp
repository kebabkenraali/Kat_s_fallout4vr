#include "SprintInjector.h"

#include <Windows.h>

namespace swing_sprint
{
    namespace
    {
        // IVRSystem vtable indices for the controller-state polls, same as the framework's VRControllersSuppressor
        constexpr int VTABLE_INDEX_GET_CONTROLLER_STATE = 34;
        constexpr int VTABLE_INDEX_GET_CONTROLLER_STATE_WITH_POSE = 35;

        /**
         * Overwrites one pointer-sized vtable slot, saving the previous entry.
         */
        bool patchVtableSlot(void** slot, void* hook, void*& outOriginal)
        {
            if (*slot == hook) {
                return outOriginal != nullptr;
            }
            DWORD oldProtect = 0;
            if (!VirtualProtect(slot, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect)) {
                return false;
            }
            outOriginal = *slot;
            *slot = hook;
            FlushInstructionCache(GetCurrentProcess(), slot, sizeof(void*));
            VirtualProtect(slot, sizeof(void*), oldProtect, &oldProtect);
            return true;
        }

        // The analog axis the game re-derives an analog-backed button press from, or -1
        int backingAxisIndex(const vr::EVRButtonId button)
        {
            switch (button) {
            case vr::k_EButton_SteamVR_Trigger:
                return 1;
            case vr::k_EButton_Grip:
                return 2;
            default:
                return -1;
            }
        }
    }

    void SprintInjector::update(const vr::TrackedDeviceIndex_t leftIndex, const vr::TrackedDeviceIndex_t rightIndex)
    {
        _leftIndex.store(leftIndex, std::memory_order_relaxed);
        _rightIndex.store(rightIndex, std::memory_order_relaxed);
        if (!_installed) {
            _installed = install();
        }
    }

    /**
     * Patch the vtable after the framework's suppressor did, so a press we force is not then cleared by it.
     */
    bool SprintInjector::install()
    {
        if (!f4cf::vrcf::VRControllersSuppress.isInitialized()) {
            return false;
        }
        auto* system = vr::VRSystem();
        if (!system) {
            return false;
        }

        void** vtable = *reinterpret_cast<void***>(system);
        void* origState = reinterpret_cast<void*>(_origGetControllerState);
        void* origStateWithPose = reinterpret_cast<void*>(_origGetControllerStateWithPose);
        const bool ok1 = patchVtableSlot(&vtable[VTABLE_INDEX_GET_CONTROLLER_STATE], reinterpret_cast<void*>(&hookedGetControllerState), origState);
        const bool ok2 = patchVtableSlot(&vtable[VTABLE_INDEX_GET_CONTROLLER_STATE_WITH_POSE], reinterpret_cast<void*>(&hookedGetControllerStateWithPose), origStateWithPose);
        if (!ok1 || !ok2) {
            logger::warn("SprintInjector: failed to patch IVRSystem vtable, will retry");
            return false;
        }
        _origGetControllerState = reinterpret_cast<GetControllerState_t>(origState);
        _origGetControllerStateWithPose = reinterpret_cast<GetControllerStateWithPose_t>(origStateWithPose);
        logger::info("SprintInjector: IVRSystem vtable hook installed (slots 34/35)");
        return true;
    }

    void SprintInjector::setPressed(const bool leftController, const vr::EVRButtonId button, const bool pressed)
    {
        auto& mask = leftController ? _leftButtons : _rightButtons;
        const uint64_t bit = vr::ButtonMaskFromId(button);
        if (pressed) {
            mask.fetch_or(bit, std::memory_order_relaxed);
        } else {
            mask.fetch_and(~bit, std::memory_order_relaxed);
        }
    }

    void SprintInjector::releaseAll()
    {
        _leftButtons.store(0, std::memory_order_relaxed);
        _rightButtons.store(0, std::memory_order_relaxed);
    }

    void SprintInjector::applyTo(const vr::TrackedDeviceIndex_t index, vr::VRControllerState_t* state)
    {
        uint64_t buttons = 0;
        if (index == _leftIndex.load(std::memory_order_relaxed)) {
            buttons = _leftButtons.load(std::memory_order_relaxed);
        } else if (index == _rightIndex.load(std::memory_order_relaxed)) {
            buttons = _rightButtons.load(std::memory_order_relaxed);
        }
        if (buttons == 0) {
            return;
        }

        state->ulButtonPressed |= buttons;
        state->ulButtonTouched |= buttons;
        for (const auto button : { vr::k_EButton_SteamVR_Trigger, vr::k_EButton_Grip }) {
            if (buttons & vr::ButtonMaskFromId(button)) {
                state->rAxis[backingAxisIndex(button)].x = 1.0f;
            }
        }
    }

    bool SprintInjector::hookedGetControllerState(vr::IVRSystem* system, const vr::TrackedDeviceIndex_t index, vr::VRControllerState_t* state, const uint32_t stateSize)
    {
        const bool ok = _origGetControllerState(system, index, state, stateSize);
        if (ok && state) {
            applyTo(index, state);
        }
        return ok;
    }

    bool SprintInjector::hookedGetControllerStateWithPose(vr::IVRSystem* system, const vr::ETrackingUniverseOrigin origin, const vr::TrackedDeviceIndex_t index,
        vr::VRControllerState_t* state, const uint32_t stateSize, vr::TrackedDevicePose_t* pose)
    {
        const bool ok = _origGetControllerStateWithPose(system, origin, index, state, stateSize, pose);
        if (ok && state) {
            applyTo(index, state);
        }
        return ok;
    }
}
