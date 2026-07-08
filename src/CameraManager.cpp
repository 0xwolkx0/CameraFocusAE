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
    RE::NiPoint3 neckWorldPos;
    if (!GetBoneWorldPosition(boneName, boneWorldPos)) {
        logger::error("Failed to get bone world position for: {}", boneName);
        return;
    }
    if (!GetBoneWorldPosition("NPC Neck [Neck]", neckWorldPos)) {
        logger::error("Failed to get bone world position for: NPC Neck [Neck]");
        return;
    }

    // Get player world position
    RE::NiPoint3 playerPos = player->GetPosition();
    auto neckOffset = neckWorldPos.z - playerPos.z;

    // Calculate offset from player to bone
    RE::NiPoint3 offsetToBone;
    offsetToBone.x = boneWorldPos.x - playerPos.x;
    offsetToBone.y = boneWorldPos.y - playerPos.y;
    offsetToBone.z = boneWorldPos.z - playerPos.z - neckOffset;

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

void CameraManager::DumpThirdPersonState() {
    auto player = RE::PlayerCharacter::GetSingleton();
    auto playerCamera = RE::PlayerCamera::GetSingleton();
    if (!player || !playerCamera) {
        logger::error("Failed to get player or camera singleton");
        return;
    }

    auto& runtimeData = playerCamera->GetRuntimeData();
    auto thirdPersonState = runtimeData.cameraStates[RE::CameraState::kThirdPerson];
    if (!thirdPersonState) {
        logger::error("Failed to get third person camera state");
        return;
    }

    auto tps = static_cast<RE::ThirdPersonState*>(thirdPersonState.get());

    logger::info("========== ThirdPersonState Data Dump ==========");

    // Player position
    RE::NiPoint3 playerPos = player->GetPosition();
    logger::info("Player Position: ({}, {}, {})", playerPos.x, playerPos.y, playerPos.z);

    // Bone positions
    RE::NiPoint3 bonePos;
    if (GetBoneWorldPosition("NPC Neck [Neck]", bonePos)) {
        logger::info("NPC Neck [Neck]: ({}, {}, {}) - Offset from player: ({}, {}, {})",
            bonePos.x, bonePos.y, bonePos.z,
            bonePos.x - playerPos.x, bonePos.y - playerPos.y, bonePos.z - playerPos.z);
    }
    if (GetBoneWorldPosition("NPC Head [Head]", bonePos)) {
        logger::info("NPC Head [Head]: ({}, {}, {}) - Offset from player: ({}, {}, {})",
            bonePos.x, bonePos.y, bonePos.z,
            bonePos.x - playerPos.x, bonePos.y - playerPos.y, bonePos.z - playerPos.z);
    }
    if (GetBoneWorldPosition("NPC Pelvis [Pelv]", bonePos)) {
        logger::info("NPC Pelvis [Pelv]: ({}, {}, {}) - Offset from player: ({}, {}, {})",
            bonePos.x, bonePos.y, bonePos.z,
            bonePos.x - playerPos.x, bonePos.y - playerPos.y, bonePos.z - playerPos.z);
    }
    if (GetBoneWorldPosition("Camera3rd [Cam3]", bonePos)) {
        logger::info("Camera3rd [Cam3]: ({}, {}, {}) - Offset from player: ({}, {}, {})",
            bonePos.x, bonePos.y, bonePos.z,
            bonePos.x - playerPos.x, bonePos.y - playerPos.y, bonePos.z - playerPos.z);
    }

    logger::info("========== ThirdPersonState Data Dump ==========");

    // Object pointers
    logger::info("thirdPersonCameraObj: {:X}", reinterpret_cast<uintptr_t>(tps->thirdPersonCameraObj));
    if (tps->thirdPersonCameraObj) {
        logger::info("  - Name: {}", tps->thirdPersonCameraObj->name.c_str());
        logger::info("  - World Position: ({}, {}, {})",
            tps->thirdPersonCameraObj->world.translate.x,
            tps->thirdPersonCameraObj->world.translate.y,
            tps->thirdPersonCameraObj->world.translate.z);
    }

    logger::info("thirdPersonFOVControl: {:X}", reinterpret_cast<uintptr_t>(tps->thirdPersonFOVControl));

    // Vectors
    logger::info("translation: ({}, {}, {})", tps->translation.x, tps->translation.y, tps->translation.z);
    logger::info("rotation: ({}, {}, {}, {})", tps->rotation.w, tps->rotation.x, tps->rotation.y, tps->rotation.z);
    logger::info("posOffsetExpected: ({}, {}, {})", tps->posOffsetExpected.x, tps->posOffsetExpected.y, tps->posOffsetExpected.z);
    logger::info("posOffsetActual: ({}, {}, {})", tps->posOffsetActual.x, tps->posOffsetActual.y, tps->posOffsetActual.z);

    // Floats
    logger::info("targetZoomOffset: {}", tps->targetZoomOffset);
    logger::info("currentZoomOffset: {}", tps->currentZoomOffset);
    logger::info("targetYaw: {} rad ({} deg)", tps->targetYaw, tps->targetYaw * 57.2958f);
    logger::info("currentYaw: {} rad ({} deg)", tps->currentYaw, tps->currentYaw * 57.2958f);
    logger::info("savedZoomOffset: {}", tps->savedZoomOffset);
    logger::info("pitchZoomOffset: {}", tps->pitchZoomOffset);

    logger::info("collisionPos: ({}, {}, {})", tps->collisionPos.x, tps->collisionPos.y, tps->collisionPos.z);
    logger::info("collisionPosValid: {}", tps->collisionPosValid);

    // Animation
    logger::info("animatedBoneName: {}", tps->animatedBoneName.c_str());
    logger::info("animationRotation: ({}, {}, {}, {})",
        tps->animationRotation.w, tps->animationRotation.x,
        tps->animationRotation.y, tps->animationRotation.z);

    // Free rotation
    logger::info("freeRotation: ({}, {})", tps->freeRotation.x, tps->freeRotation.y);
    logger::info("freeRotationEnabled: {}", tps->freeRotationEnabled);

    // Booleans
    logger::info("stateNotActive: {}", tps->stateNotActive);
    logger::info("toggleAnimCam: {}", tps->toggleAnimCam);
    logger::info("applyOffsets: {}", tps->applyOffsets);

    // Unknown fields
    logger::info("unkA0: 0x{:X}", tps->unkA0);
    logger::info("unkC0: 0x{:X}", tps->unkC0);
    logger::info("unkC8: 0x{:X}", tps->unkC8);
    logger::info("unkD0: 0x{:X}", tps->unkD0);
    logger::info("unkDC: 0x{:X}", tps->unkDC);
    logger::info("unkE2: 0x{:X}", tps->unkE2);
    logger::info("unkE4: 0x{:X}", tps->unkE4);

    logger::info("================================================");
}
