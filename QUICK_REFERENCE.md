# Quick Reference - Camera Bone Focus Mod

## What This Mod Does
Allows you to cycle camera focus between character bones (Head/Pelvis/Default) in third-person view by pressing F4.

## Files You Need to Know About

```
src/CameraManager.h     → Camera state manager (interface)
src/CameraManager.cpp   → Camera logic (implementation)  
src/main.cpp            → Plugin entry + F4 key binding
```

## Key API Calls Used

### Getting Bone Position
```cpp
auto player = RE::PlayerCharacter::GetSingleton();
auto root3D = player->Get3D();
auto rootNode = root3D->AsNode();
auto bone = rootNode->GetObjectByName("NPC Head [Head]");
RE::NiPoint3 bonePos = bone->world.translate;
```

### Adjusting Camera
```cpp
auto camera = RE::PlayerCamera::GetSingleton();
auto& runtimeData = camera->GetRuntimeData();
auto tps = static_cast<RE::ThirdPersonState*>(
    runtimeData.cameraStates[RE::CameraState::kThirdPerson].get()
);
tps->posOffsetExpected = offsetToBone; // Smooth transition
```

## Bone Names
- Head: `"NPC Head [Head]"`
- Pelvis: `"NPC Pelvis [Pelv]"`

## Build Commands
```bash
# Configure (choose one)
xmake f --skyrim_se=y --skyrim_ae=y  # SE + AE
xmake f --skyrim_se=y                # SE only
xmake f --skyrim_ae=y                # AE only
xmake f --skyrim_vr=y                # VR only

# Build
xmake

# Output: build/windows/x64/release/Camera.dll
```

## Installation
1. Build the project → Get `Camera.dll`
2. Copy to: `<Skyrim>/Data/SKSE/Plugins/Camera.dll`
3. Launch game via SKSE

## In-Game Usage
- **F3**: Toggle PrismaUI focus (original feature)
- **F4**: Cycle camera (Default → Head → Pelvis → Default)
- Only works in third-person view

## How to Customize

### Change Hotkey
Edit `src/main.cpp` line 28:
```cpp
const uint32_t CAMERA_CYCLE_KEY = 0x3E; // F4
```

Common scan codes:
- F1: `0x3B`, F2: `0x3C`, F3: `0x3D`
- F4: `0x3E`, F5: `0x3F`, F6: `0x40`

### Add New Bone Focus
1. Add enum value to `CameraFocusState` in `CameraManager.h`:
```cpp
enum class CameraFocusState {
    Default,
    Head,
    Pelvis,
    Chest  // NEW
};
```

2. Add case in `CycleState()` in `CameraManager.cpp`:
```cpp
case CameraFocusState::Pelvis:
    currentState = CameraFocusState::Chest;
    FocusOnBone("NPC Spine2 [Spn2]");  // Example chest bone
    logger::info("Camera state: Chest");
    break;
case CameraFocusState::Chest:
    currentState = CameraFocusState::Default;
    RestoreDefaultCamera();
    logger::info("Camera state: Default");
    break;
```

### Find Bone Names
Add this debug code in `GetBoneWorldPosition()` to list all bones:
```cpp
auto rootNode = root3D->AsNode();
if (rootNode) {
    auto& children = rootNode->GetChildren();
    for (auto& child : children) {
        if (child) {
            logger::info("Bone: {}", child->name.c_str());
        }
    }
}
```

## Coordinate System
- **World Space**: Absolute positions in game world
- **Player Relative**: Offset from player position
- Camera uses player-relative coordinates:
  ```cpp
  offset = boneWorldPos - playerWorldPos
  ```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Camera doesn't move | Check you're in 3rd person (press F) |
| "Bone not found" | Check exact spelling with brackets |
| Build fails | Run `git submodule update --init --recursive` |
| Plugin doesn't load | Check SKSE log, verify DLL in plugins folder |

## Log Location
```
Documents/My Games/Skyrim Special Edition/SKSE/Camera.log
```

## Technical Notes

**Camera Interpolation:**
- `posOffsetExpected` → Game smoothly interpolates to this
- `posOffsetActual` → Current position (set for instant snap)

**Third-Person Check:**
```cpp
if (!playerCamera->IsInThirdPerson()) {
    // Skip camera manipulation
}
```

**Store Original State:**
```cpp
if (!hasStoredOriginal) {
    originalCameraOffset = tps->posOffsetActual;
    hasStoredOriginal = true;
}
```

## Class Hierarchy
```
NiObjectNET
  └─ NiAVObject (has world.translate)
      └─ NiNode (has GetObjectByName())
          └─ [Player 3D skeleton]
              ├─ NPC Head [Head]
              ├─ NPC Pelvis [Pelv]
              └─ [other bones...]
```

## State Flow
```
┌─────────┐  F4   ┌──────┐  F4   ┌────────┐  F4   ┌─────────┐
│ Default │ ───→  │ Head │ ───→  │ Pelvis │ ───→  │ Default │
└─────────┘       └──────┘       └────────┘       └─────────┘
```

## Memory Safety
✓ All pointer checks before dereferencing
✓ Singleton pattern for manager
✓ Store original state before modifying
✓ Log all failures for debugging

## Performance
- **Negligible overhead**: Only runs on key press
- **No per-frame updates**: Uses game's interpolation
- **Minimal memory**: One NiPoint3 stored

---

**Ready to build?** Run `xmake f --skyrim_se=y --skyrim_ae=y && xmake`

**Need help?** Check `IMPLEMENTATION_GUIDE.md` for detailed technical info.
