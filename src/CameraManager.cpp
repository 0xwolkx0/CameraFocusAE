#include "CameraManager.h"

/* void CameraManager::CycleState() {
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
        originalTranslation = tps->translation;
        hasStoredOriginal = true;
        logger::info("Stored original camera offset: ({}, {}, {})",
            originalCameraOffset.x, originalCameraOffset.y, originalCameraOffset.z);
    }

    // Get bone world position
    RE::NiPoint3 bonePos;
    if (!GetBoneWorldPosition(boneName, bonePos)) {
        logger::error("Failed to get bone position for: {}", boneName);
        return;
    }

    logger::info("Camera focused on bone '{}' at position: ({}, {}, {})",
        boneName, bonePos.x, bonePos.y, bonePos.z);
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
} */

/* void CameraManager::Update(RE::PlayerCharacter* player, RE::Actor* cameraRef, RE::PlayerCamera* playerCamera)
	noexcept
{
    RE::ThirdPersonState::Update(player, cameraRef, playerCamera);


} */

/* void CameraManager::SetPosition(const RE::NiPoint3& pos, RE::PlayerCamera* camera, RE::NiCamera* niCamera) {
    if (!camera) {
        logger::error("SetPosition: camera is null");
        return;
    }

    auto& cameraNode = camera->cameraRoot;
    if (!cameraNode) {
        logger::error("SetPosition: cameraRoot is null");
        return;
    }

    // Use provided niCamera or fall back to stored cameraNi
    auto camNi = niCamera ? niCamera : cameraNi.get();
    if (!camNi) {
        logger::error("SetPosition: niCamera is null");
        return;
    }

    // Set position for all three: local, world, and camera transform
    cameraNode->local.translate = cameraNode->world.translate = camNi->world.translate = pos;

    // Update ThirdPersonState translation if in third person mode
    if (camera->IsInThirdPerson() && camera->currentState) {
        auto state = reinterpret_cast<RE::ThirdPersonState*>(camera->currentState.get());
        state->translation = cameraNode->local.translate;
    }

    logger::trace("SetPosition: Camera position set to ({}, {}, {})", pos.x, pos.y, pos.z);
}

void CameraManager::ApplyLocalSpaceGameOffsets(const RE::Actor* player, RE::PlayerCamera* playerCamera) {
    if (!player || !playerCamera) {
        logger::error("ApplyLocalSpaceGameOffsets: player or camera is null");
        return;
    }

    if (!playerCamera->currentState) {
        logger::error("ApplyLocalSpaceGameOffsets: currentState is null");
        return;
    }

    auto state = reinterpret_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());

    // Get player position as the focus point
    RE::NiPoint3 playerPos = player->GetPosition();

    // Get camera position (from cameraNi)
    if (!cameraNi) {
        logger::error("ApplyLocalSpaceGameOffsets: cameraNi is null");
        return;
    }

    RE::NiPoint3 cameraPos = cameraNi->world.translate;

    // Calculate vector from player to camera
    RE::NiPoint3 offset;
    offset.x = cameraPos.x - playerPos.x;
    offset.y = cameraPos.y - playerPos.y;
    offset.z = cameraPos.z - playerPos.z;

    // Calculate rotation from the offset vector
    float yaw = std::atan2(offset.x, offset.y);
    float distance = std::sqrt(offset.x * offset.x + offset.y * offset.y);
    float pitch = std::atan2(offset.z, distance);

    // Set the rotation as a quaternion
    // Convert euler angles (pitch, yaw, roll=0) to quaternion
    float cy = std::cos(yaw * 0.5f);
    float sy = std::sin(yaw * 0.5f);
    float cp = std::cos(pitch * 0.5f);
    float sp = std::sin(pitch * 0.5f);

    state->rotation.w = cy * cp;
    state->rotation.x = sy * sp;
    state->rotation.y = sy * cp;
    state->rotation.z = cy * sp;

    // Set yaw angles
    state->targetYaw = yaw;
    state->currentYaw = yaw;

    // Clear the position offsets - we're setting absolute position
    state->posOffsetExpected.x = 0.0f;
    state->posOffsetExpected.y = 0.0f;
    state->posOffsetExpected.z = 0.0f;
    state->posOffsetActual = state->posOffsetExpected;

    // Update the world to screen matrix to reflect the new position
    UpdateInternalWorldToScreenMatrix(cameraNi.get());

    logger::trace("ApplyLocalSpaceGameOffsets: yaw={}, pitch={}", yaw, pitch);
}

void CameraManager::UpdateInternalWorldToScreenMatrix(RE::NiCamera* niCamera) {
    // Use provided niCamera or fall back to stored cameraNi
    auto camNi = niCamera ? niCamera : cameraNi.get();
    if (!camNi) {
        logger::error("UpdateInternalWorldToScreenMatrix: niCamera is null");
        return;
    }

    // Define the function signature for the internal Skyrim function
    typedef void(*UpdateWorldToScreenMtx)(RE::NiCamera*);

    // Get the function address from CommonLibSSE offsets
    // For SE: 69271, for AE: 70641
    static REL::Relocation<UpdateWorldToScreenMtx> toScreenFunc{ RELOCATION_ID(69271, 70641) };

    // Call the internal Skyrim function to update the matrix
    toScreenFunc(camNi);

    logger::trace("UpdateInternalWorldToScreenMatrix: Matrix updated");
}

RE::NiPointer<RE::NiCamera> CameraManager::GetNiCamera(RE::PlayerCamera* camera) const {
    if (!camera) {
        logger::error("GetNiCamera: camera is null");
        return nullptr;
    }

    auto& cameraNode = camera->cameraRoot;
    if (!cameraNode) {
        logger::error("GetNiCamera: cameraRoot is null");
        return nullptr;
    }

    // Check if there are any children
    if (cameraNode->children.size() == 0) {
        logger::error("GetNiCamera: cameraRoot has no children");
        return nullptr;
    }

    // Iterate through children and use skyrim_cast to find NiCamera
    for (auto& child : cameraNode->children) {
        if (child) {
            auto asCamera = skyrim_cast<RE::NiCamera*>(child.get());
            if (asCamera) {
                return RE::NiPointer<RE::NiCamera>(asCamera);
            }
        }
    }

    logger::error("GetNiCamera: No NiCamera found in camera hierarchy");
    return nullptr;
} */
