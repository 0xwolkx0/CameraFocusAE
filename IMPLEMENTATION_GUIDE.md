# Skyrim SE Camera Bone Focus Mod - Implementation Guide

## Overview
This guide provides a complete implementation for a mod that allows toggling the camera focus between specific bones on the player character (Head, Pelvis, and Default position) in third-person view.

## Architecture

### Core Components
1. **Camera State Manager** - Tracks current focus state (Head/Pelvis/Default)
2. **Bone Position Retriever** - Gets world transform of skeleton bones
3. **Camera Controller** - Adjusts camera position to center on selected bone
4. **Input Handler** - Detects button press to cycle through states

## Implementation Details

### 1. Bone Position Retrieval

```cpp
// Get player's 3D skeleton
auto player = RE::PlayerCharacter::GetSingleton();
if (!player) return;

auto root3D = player->Get3D();  // Returns NiAVObject*
if (!root3D) return;

// Cast to NiNode to access GetObjectByName
auto rootNode = root3D->AsNode();
if (!rootNode) return;

// Get specific bone by name
auto headBone = rootNode->GetObjectByName("NPC Head [Head]");
auto pelvisBone = rootNode->GetObjectByName("NPC Pelvis [Pelv]");

if (headBone) {
    // Get world transform
    RE::NiPoint3 headWorldPos = headBone->world.translate;
    // headWorldPos now contains the bone's world coordinates
}
```

### 2. Camera Access and Manipulation

```cpp
auto playerCamera = RE::PlayerCamera::GetSingleton();
if (!playerCamera) return;

// Check if we're in third person
if (!playerCamera->IsInThirdPerson()) {
    logger::info("Not in third person, camera adjustment skipped");
    return;
}

// Get the third person camera state
auto& runtimeData = playerCamera->GetRuntimeData();
auto thirdPersonState = runtimeData.cameraStates[RE::CameraState::kThirdPerson];

if (thirdPersonState) {
    auto tps = static_cast<RE::ThirdPersonState*>(thirdPersonState.get());
    
    // Access current camera properties
    RE::NiPoint3 currentTranslation = tps->translation;
    RE::NiPoint3 currentOffset = tps->posOffsetActual;
    
    // Modify camera to focus on bone
    // Calculate offset from bone to camera target
    RE::NiPoint3 targetBonePos = /* bone world position */;
    
    // Set new camera focus
    tps->posOffsetExpected = targetBonePos;
    // The game will smoothly interpolate to this position
}
```

### 3. Camera State Manager

```cpp
enum class CameraFocusState {
    Default,
    Head,
    Pelvis
};

class CameraManager {
private:
    CameraFocusState currentState = CameraFocusState::Default;
    RE::NiPoint3 originalCameraOffset; // Store original offset
    bool hasStoredOriginal = false;

public:
    void CycleState() {
        switch (currentState) {
            case CameraFocusState::Default:
                currentState = CameraFocusState::Head;
                FocusOnBone("NPC Head [Head]");
                break;
            case CameraFocusState::Head:
                currentState = CameraFocusState::Pelvis;
                FocusOnBone("NPC Pelvis [Pelv]");
                break;
            case CameraFocusState::Pelvis:
                currentState = CameraFocusState::Default;
                RestoreDefaultCamera();
                break;
        }
    }

    void FocusOnBone(const char* boneName) {
        auto player = RE::PlayerCharacter::GetSingleton();
        auto playerCamera = RE::PlayerCamera::GetSingleton();
        
        if (!player || !playerCamera) return;
        if (!playerCamera->IsInThirdPerson()) return;

        // Store original offset on first use
        if (!hasStoredOriginal) {
            auto& runtimeData = playerCamera->GetRuntimeData();
            auto tps = static_cast<RE::ThirdPersonState*>(
                runtimeData.cameraStates[RE::CameraState::kThirdPerson].get()
            );
            if (tps) {
                originalCameraOffset = tps->posOffsetActual;
                hasStoredOriginal = true;
            }
        }

        // Get bone position
        auto root3D = player->Get3D();
        if (!root3D) return;
        
        auto rootNode = root3D->AsNode();
        if (!rootNode) return;
        
        auto bone = rootNode->GetObjectByName(boneName);
        if (!bone) {
            logger::warn("Bone {} not found", boneName);
            return;
        }

        // Get bone world position
        RE::NiPoint3 boneWorldPos = bone->world.translate;
        
        // Adjust camera to center on bone
        auto& runtimeData = playerCamera->GetRuntimeData();
        auto tps = static_cast<RE::ThirdPersonState*>(
            runtimeData.cameraStates[RE::CameraState::kThirdPerson].get()
        );
        
        if (tps) {
            // Calculate player position
            RE::NiPoint3 playerPos = player->GetPosition();
            
            // Calculate offset from player to bone in player's local space
            RE::NiPoint3 offsetToBone;
            offsetToBone.x = boneWorldPos.x - playerPos.x;
            offsetToBone.y = boneWorldPos.y - playerPos.y;
            offsetToBone.z = boneWorldPos.z - playerPos.z;
            
            // Apply offset to camera target
            tps->posOffsetExpected = offsetToBone;
            
            logger::info("Camera focused on {}", boneName);
        }
    }

    void RestoreDefaultCamera() {
        auto playerCamera = RE::PlayerCamera::GetSingleton();
        if (!playerCamera || !hasStoredOriginal) return;

        auto& runtimeData = playerCamera->GetRuntimeData();
        auto tps = static_cast<RE::ThirdPersonState*>(
            runtimeData.cameraStates[RE::CameraState::kThirdPerson].get()
        );
        
        if (tps) {
            tps->posOffsetExpected = originalCameraOffset;
            logger::info("Camera restored to default");
        }
    }
};
```

### 4. Key Input Integration

Update your `main.cpp` to add the camera cycling functionality:

```cpp
#include "CameraManager.h"  // Your camera manager class

// In SKSEMessageHandler after KeyHandler setup:
KeyHandler* keyHandler = KeyHandler::GetSingleton();

// Create camera manager instance
static CameraManager cameraManager;

// Register key for camera cycling (e.g., F4)
const uint32_t CAMERA_CYCLE_KEY = 0x3E; // F4 key

KeyHandlerEvent cameraEventHandler = keyHandler->Register(
    CAMERA_CYCLE_KEY, 
    KeyEventType::KEY_DOWN, 
    []() {
        cameraManager.CycleState();
    }
);
```

## Key Constants

### Bone Names
- Head: `"NPC Head [Head]"`
- Pelvis: `"NPC Pelvis [Pelv]"`

### Scan Codes for Common Keys
- F3: `0x3D`
- F4: `0x3E`
- F5: `0x3F`
- F6: `0x40`

## Important Notes

### 1. Third-Person Only
Always check `playerCamera->IsInThirdPerson()` before manipulating camera. First-person camera works differently and should be ignored.

### 2. World Transform
The `NiAVObject::world` member contains the complete world transform:
- `world.translate` - World position (NiPoint3)
- `world.rotate` - World rotation (NiMatrix3)
- `world.scale` - World scale (float)

### 3. Camera Interpolation
The game smoothly interpolates between `posOffsetActual` and `posOffsetExpected`. Setting `posOffsetExpected` will cause the camera to smoothly move to the new position.

### 4. Coordinate System
Skyrim uses a right-handed coordinate system where:
- X: East/West
- Y: North/South  
- Z: Up/Down

### 5. Player Position Reference
Since bones are in world space and camera offset is relative to player, you need to:
1. Get player world position: `player->GetPosition()`
2. Get bone world position: `bone->world.translate`
3. Calculate relative offset: `bonePos - playerPos`
4. Apply to camera: `tps->posOffsetExpected = offset`

## Testing Checklist

1. ✓ Load into game and enter third-person view
2. ✓ Press configured key (F4) - camera should focus on head
3. ✓ Press again - camera should focus on pelvis
4. ✓ Press again - camera should return to default position
5. ✓ Switch to first-person - key should have no effect
6. ✓ Switch back to third-person - cycling should work again

## Troubleshooting

### Camera doesn't move
- Check log for bone lookup failures
- Verify `IsInThirdPerson()` returns true
- Ensure player's 3D is loaded (`Get3D()` not null)

### Bone not found
- Verify exact bone name spelling with spaces and brackets
- Check if player skeleton is fully loaded
- Try logging all bone names with a recursive traversal

### Camera snaps instead of smooth movement
- This is expected when changing `posOffsetExpected`
- For instant snap, also set `posOffsetActual`
- For smooth transition, only set `posOffsetExpected`

## Additional Features to Consider

1. **Configurable Keys** - Use TOML/INI for key configuration
2. **Custom Bone Support** - Allow user to specify custom bone names
3. **Camera Distance Adjustment** - Modify `currentZoomOffset` along with position
4. **Smooth Follow** - Update camera offset every frame for dynamic following
5. **Animation-Aware** - Account for bone animation when tracking

## References

### CommonLibSSE Classes
- `RE::PlayerCamera` - Main camera singleton
- `RE::ThirdPersonState` - Third-person camera state
- `RE::PlayerCharacter` - Player singleton
- `RE::NiNode` - Scene graph node with `GetObjectByName`
- `RE::NiAVObject` - Base class with world transform
- `RE::NiPoint3` - 3D vector for positions

### Header Locations
- `RE/P/PlayerCamera.h`
- `RE/T/ThirdPersonState.h`
- `RE/P/PlayerCharacter.h`
- `RE/N/NiNode.h`
- `RE/N/NiAVObject.h`
