#pragma once

#include <RE/Skyrim.h>

enum class CameraFocusState {
    Default,
    Head,
    Pelvis
};
void CycleState();
void HookedUpdate(RE::ThirdPersonState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState);
void thunk(RE::ThirdPersonState* a_state, float* rotation,bool a_flag,bool a_someFlag);
void Install();

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

    // List all bones in the player's skeleton with their positions
    void ListAllBones();

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
/* 
    // Recursively traverse and log all bones in the skeleton
    void TraverseBones(RE::NiAVObject* node, int depth = 0);

    // Set the camera world position (similar to SmoothCam implementation)
    void SetPosition(const RE::NiPoint3& pos, RE::PlayerCamera* camera, RE::NiCamera* niCamera = nullptr);

    // Apply local space offsets to ThirdPersonState for proper game integration
    void ApplyLocalSpaceGameOffsets(const RE::Actor* player, RE::PlayerCamera* playerCamera);

    // Update the internal world to screen matrix
    void UpdateInternalWorldToScreenMatrix(RE::NiCamera* niCamera = nullptr);

    // Get the NiCamera from PlayerCamera
    RE::NiPointer<RE::NiCamera> GetNiCamera(RE::PlayerCamera* camera) const; */

    CameraFocusState currentState = CameraFocusState::Default;
    RE::NiPoint3 originalCameraOffset;
    RE::NiPoint3 originalTranslation;
    bool hasStoredOriginal = false;
    RE::NiPointer<RE::NiCamera> cameraNi = nullptr;  // Active NiCamera
};
