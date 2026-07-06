# Camera Bone Focus Mod - Implementation Summary

## What We've Built

I've created a complete Skyrim SE mod implementation that allows you to cycle the camera focus between different bones on the player character in third-person view.

## Files Created

### 1. **src/CameraManager.h** (Header File)
Defines the `CameraManager` singleton class that manages camera states:
- Three camera states: Default, Head, Pelvis
- Methods for cycling states, focusing on bones, and restoring default camera
- Private helper method for bone position retrieval

### 2. **src/CameraManager.cpp** (Implementation)
Complete implementation with:
- **CycleState()**: Cycles through Default → Head → Pelvis → Default
- **FocusOnBone()**: 
  - Gets player and camera singletons
  - Retrieves bone world position
  - Calculates offset from player to bone
  - Applies smooth camera transition
- **RestoreDefaultCamera()**: Returns camera to original position
- **GetBoneWorldPosition()**: 
  - Accesses player's 3D skeleton
  - Uses `GetObjectByName()` to find bones
  - Extracts world transform data

### 3. **src/main.cpp** (Updated)
Added camera manager integration:
- Included `CameraManager.h`
- Registered F4 key handler for camera cycling
- Calls `CameraManager::GetSingleton()->CycleState()` on key press

### 4. **IMPLEMENTATION_GUIDE.md** (Documentation)
Comprehensive technical guide covering:
- Architecture overview
- Detailed API usage examples
- Bone name constants
- Coordinate system explanation
- Testing checklist
- Troubleshooting guide
- CommonLibSSE class references

### 5. **CAMERA_MOD_README.md** (User Documentation)
User-facing documentation with:
- Feature description
- Installation instructions
- Build instructions
- Controls and configuration
- Troubleshooting tips
- Known limitations

## How It Works

### Technical Flow

1. **User presses F4** → Key handler callback triggered
2. **CycleState() called** → Determines next state
3. **FocusOnBone() executed**:
   ```
   Player::GetSingleton() → Get player reference
   player->Get3D() → Get 3D skeleton root
   root->AsNode() → Cast to NiNode
   node->GetObjectByName("NPC Head [Head]") → Find bone
   bone->world.translate → Get world position
   ```
4. **Camera adjustment**:
   ```
   PlayerCamera::GetSingleton() → Get camera
   camera->GetRuntimeData() → Access runtime data
   runtimeData.cameraStates[kThirdPerson] → Get TPS state
   tps->posOffsetExpected = boneOffset → Apply offset
   ```
5. **Game interpolates** → Smooth camera transition

### Key Technical Details

**Bone Access Pattern (as per your guidance):**
```cpp
auto bone = rootNode->GetObjectByName("NPC Head [Head]");
RE::NiPoint3 worldPos = bone->world.translate;
```

**Camera Manipulation:**
```cpp
auto tps = static_cast<RE::ThirdPersonState*>(
    runtimeData.cameraStates[RE::CameraState::kThirdPerson].get()
);
tps->posOffsetExpected = offsetToBone;
```

**Coordinate Calculation:**
```cpp
// Bone is in world space, camera needs player-relative offset
RE::NiPoint3 playerPos = player->GetPosition();
RE::NiPoint3 offsetToBone;
offsetToBone.x = boneWorldPos.x - playerPos.x;
offsetToBone.y = boneWorldPos.y - playerPos.y;
offsetToBone.z = boneWorldPos.z - playerPos.z;
```

## What You Can Do Now

### Option 1: Build the Mod (Requires xmake)
```bash
# Install xmake if needed: https://xmake.io/

# Configure for your Skyrim version
xmake f --skyrim_se=y --skyrim_ae=y  # For SE + AE compatibility
# OR
xmake f --skyrim_se=y                # For SE only
# OR
xmake f --skyrim_ae=y                # For AE only
# OR
xmake f --skyrim_vr=y                # For VR

# Build
xmake

# Find the DLL in: build/windows/x64/release/Camera.dll
```

### Option 2: Customize Before Building

**Change the hotkey:**
Edit `src/main.cpp` line 28:
```cpp
const uint32_t CAMERA_CYCLE_KEY = 0x3E; // Change to desired scan code
```

**Add more bone focus points:**
1. Add new state to enum in `CameraManager.h`
2. Add new case in `CycleState()` switch
3. Call `FocusOnBone("Your Bone Name Here")`

**Adjust camera behavior:**
In `CameraManager.cpp`, modify `FocusOnBone()`:
- Change `posOffsetExpected` for smooth transitions
- Use `posOffsetActual` for instant snapping
- Modify `currentZoomOffset` to adjust camera distance

## Testing Checklist

Once built and installed:

1. ✓ Load a save game
2. ✓ Enter third-person view (press F)
3. ✓ Press F4 → Camera should focus on character's head
4. ✓ Press F4 again → Camera should focus on pelvis
5. ✓ Press F4 again → Camera should return to default
6. ✓ Switch to first-person → F4 should do nothing (logged as "Not in third person")
7. ✓ Check logs in `Documents/My Games/Skyrim Special Edition/SKSE/Camera.log`

## Project Structure

```
Camera/
├── src/
│   ├── main.cpp              ← Entry point, key handler setup
│   ├── CameraManager.h       ← Camera manager interface
│   ├── CameraManager.cpp     ← Camera logic implementation
│   ├── pch.h                 ← Precompiled headers
│   ├── plugin.h              ← Plugin metadata
│   ├── PrismaUI_API.h        ← PrismaUI interface
│   └── keyhandler/           ← Keyboard input handler
├── lib/
│   └── commonlibsse-ng/      ← CommonLibSSE-NG submodule
├── view/                     ← PrismaUI HTML/JS assets
├── xmake.lua                 ← Build configuration
├── IMPLEMENTATION_GUIDE.md   ← Technical documentation
├── CAMERA_MOD_README.md      ← User documentation
└── README.md                 ← Original template README
```

## Next Steps

1. **Install xmake** if you haven't:
   - Download from https://xmake.io/#/getting_started?id=installation
   - Or via scoop: `scoop install xmake`

2. **Build the project** using the commands above

3. **Test in-game**:
   - Copy `Camera.dll` to `Data/SKSE/Plugins/`
   - Launch via SKSE
   - Press F4 in third-person view

4. **Customize** (optional):
   - Change hotkeys
   - Add more bone focus points
   - Adjust camera behavior

## Troubleshooting

### Build Issues
- Ensure Visual Studio 2022 is installed
- Check that Git submodules are initialized: `git submodule update --init --recursive`
- Verify xmake version: `xmake --version` (should be ≥ 3.0.1)

### Runtime Issues
- Check SKSE is correctly installed
- Verify plugin loads: Check SKSE log
- Enable debug logging: Build with `xmake f -m debug`

## Additional Resources

- **CommonLibSSE-NG Docs**: https://github.com/CharmedBaryon/CommonLibSSE-NG
- **SKSE Documentation**: https://skse.silverlock.org/
- **Skyrim Modding Wiki**: https://www.creationkit.com/

## Implementation Highlights

✓ **Proper singleton pattern** for camera manager
✓ **Third-person only** with safety checks
✓ **Smooth camera transitions** using `posOffsetExpected`
✓ **Original state restoration** by storing initial offset
✓ **Comprehensive logging** for debugging
✓ **Null pointer checks** throughout
✓ **Clean separation of concerns** (manager vs. input handling)

The implementation follows CommonLibSSE-NG best practices and Skyrim modding conventions. The code is ready to build and test!
