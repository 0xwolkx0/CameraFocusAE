#include "CameraManager.h"

void CameraManager::CycleState() {
    auto playerCamera = RE::PlayerCamera::GetSingleton();
    if (!playerCamera) {
        logger::error("Failed to get PlayerCamera singleton");
        return;
    }

    // Only work in third person
    if (!playerCamera->IsInThirdPerson()) {
        logger::info("Not in third person view, camera cycling disabled");
        return;
    }

    // Cycle to next state
    switch (currentState) {
        case CameraFocusState::Default:
            currentState = CameraFocusState::Head;
            FocusOnBone("NPC Head [Head]");
            logger::info("Camera state: Head");
            break;
        case CameraFocusState::Head:
            currentState = CameraFocusState::Pelvis;
            FocusOnBone("NPC Pelvis [Pelv]");
            logger::info("Camera state: Pelvis");
            break;
        case CameraFocusState::Pelvis:
            currentState = CameraFocusState::Default;
            RestoreDefaultCamera();
            logger::info("Camera state: Default");
            break;
    }
}

void CameraManager::FocusOnBone(const char* boneName) {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto playerCamera = RE::PlayerCamera::GetSingleton();

    if (!player || !playerCamera) {
        logger::error("Failed to get player or camera singleton");
        return;
    }

    // Get the third person camera state
    auto& runtimeData = playerCamera->GetRuntimeData();
    auto thirdPersonState = runtimeData.cameraStates[RE::CameraState::kThirdPerson];

    if (!thirdPersonState) {
        logger::error("Failed to get third person camera state");
        return;
    }

    auto tps = static_cast<RE::ThirdPersonState*>(thirdPersonState.get());

    // Store original offset on first use
    if (!hasStoredOriginal) {
        originalCameraOffset = tps->posOffsetActual;
        hasStoredOriginal = true;
        logger::info("Stored original camera offset: ({}, {}, {})",
            originalCameraOffset.x, originalCameraOffset.y, originalCameraOffset.z);
    }

    // Get bone world position
    RE::NiPoint3 boneWorldPos;
    if (!GetBoneWorldPosition(boneName, boneWorldPos)) {
        logger::error("Failed to get bone world position for: {}", boneName);
        return;
    }

    // Get player world position
    RE::NiPoint3 playerPos = player->GetPosition();

    // Calculate offset from player to bone
    RE::NiPoint3 offsetToBone;
    offsetToBone.x = boneWorldPos.x - playerPos.x;
    offsetToBone.y = boneWorldPos.y - playerPos.y;
    offsetToBone.z = boneWorldPos.z - playerPos.z;

    logger::info("Player position: ({}, {}, {})", playerPos.x, playerPos.y, playerPos.z);
    logger::info("Bone world position: ({}, {}, {})", boneWorldPos.x, boneWorldPos.y, boneWorldPos.z);
    logger::info("Calculated offset: ({}, {}, {})", offsetToBone.x, offsetToBone.y, offsetToBone.z);

    // Apply offset to camera target
    // Using posOffsetExpected for smooth interpolation
    tps->posOffsetExpected = offsetToBone;

    logger::info("Camera focused on bone: {}", boneName);
}

void CameraManager::RestoreDefaultCamera() {
    auto playerCamera = RE::PlayerCamera::GetSingleton();

    if (!playerCamera) {
        logger::error("Failed to get PlayerCamera singleton");
        return;
    }

    if (!hasStoredOriginal) {
        logger::warn("No original camera offset stored, cannot restore");
        return;
    }

    auto& runtimeData = playerCamera->GetRuntimeData();
    auto thirdPersonState = runtimeData.cameraStates[RE::CameraState::kThirdPerson];

    if (!thirdPersonState) {
        logger::error("Failed to get third person camera state");
        return;
    }

    auto tps = static_cast<RE::ThirdPersonState*>(thirdPersonState.get());

    // Restore original offset
    tps->posOffsetExpected = originalCameraOffset;

    logger::info("Camera restored to default position");
}

bool CameraManager::GetBoneWorldPosition(const char* boneName, RE::NiPoint3& outPosition) {
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player) {
        logger::error("Failed to get player singleton");
        return false;
    }

    // Get player's 3D model
    auto root3D = player->Get3D();
    if (!root3D) {
        logger::error("Player 3D model not loaded");
        return false;
    }

    // Cast to NiNode to access GetObjectByName
    auto rootNode = root3D->AsNode();
    if (!rootNode) {
        logger::error("Failed to cast player 3D to NiNode");
        return false;
    }

    // Find the bone by name
    auto bone = rootNode->GetObjectByName(boneName);
    if (!bone) {
        logger::error("Bone not found: {}", boneName);
        return false;
    }

    // Get the world transform
    outPosition = bone->world.translate;

    return true;
}
