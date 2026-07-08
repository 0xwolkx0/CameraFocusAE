#pragma once

#include <RE/Skyrim.h>

enum class CameraFocusState {
    Default,
    Head,
    Pelvis
};

class CameraManager {
public:
    static CameraManager* GetSingleton() {
        static CameraManager instance;
        return &instance;
    }

    // Cycle through camera states: Default -> Head -> Pelvis -> Default
    void CycleState();

    // Get current camera state
    CameraFocusState GetCurrentState() const { return currentState; }

    // Dump all ThirdPersonState data to console
    void DumpThirdPersonState();

private:
    CameraManager() = default;
    ~CameraManager() = default;
    CameraManager(const CameraManager&) = delete;
    CameraManager& operator=(const CameraManager&) = delete;

    // Focus camera on a specific bone
    void FocusOnBone(const char* boneName);

    // Restore camera to default position
    void RestoreDefaultCamera();

    // Get world position of a bone by name
    bool GetBoneWorldPosition(const char* boneName, RE::NiPoint3& outPosition);

    CameraFocusState currentState = CameraFocusState::Default;
    RE::NiPoint3 originalCameraOffset;
    bool hasStoredOriginal = false;
};
