# Camera Bone Focus Mod for Skyrim SE

A SKSE plugin that allows you to cycle the camera focus between different bones on your player character while in third-person view.

## Features

- **Three Camera States:**
  - **Default**: Normal third-person camera position
  - **Head Focus**: Camera centers on the character's head bone (`NPC Head [Head]`)
  - **Pelvis Focus**: Camera centers on the character's pelvis bone (`NPC Pelvis [Pelv]`)

- **Easy Toggling**: Press **F4** to cycle through camera states
- **Third-Person Only**: Automatically disabled in first-person view
- **Smooth Transitions**: Camera smoothly interpolates to new positions

## Controls

- **F3**: Toggle PrismaUI focus (original functionality)
- **F4**: Cycle camera focus (Default → Head → Pelvis → Default)

## Installation

1. Build the plugin using xmake
2. Copy the generated `.dll` to your Skyrim SE `Data/SKSE/Plugins/` folder
3. Launch Skyrim SE with SKSE

## Building

### Prerequisites
- Visual Studio 2022 or later
- xmake 3.0.1 or later
- SKSE and CommonLibSSE-NG (included as submodules)

### Build Steps

```bash
# Configure for Skyrim SE + AE
xmake f --skyrim_se=y --skyrim_ae=y

# Or configure for Skyrim SE only
xmake f --skyrim_se=y

# Or configure for Skyrim AE only
xmake f --skyrim_ae=y

# Or configure for Skyrim VR
xmake f --skyrim_vr=y

# Build
xmake

# The built plugin will be in build/windows/x64/[mode]/
```

## How It Works

The mod uses CommonLibSSE-NG to:

1. **Access the Skeleton**: Gets the player's 3D model and searches for specific bones by name
2. **Retrieve Bone Positions**: Extracts world transform data from bone nodes
3. **Manipulate Camera**: Modifies the third-person camera state to adjust its focus point
4. **Handle Input**: Listens for F4 key presses to cycle between states

### Technical Details

- Uses `NiNode::GetObjectByName()` to find bones in the skeleton hierarchy
- Accesses bone world positions via `NiAVObject::world.translate`
- Modifies `ThirdPersonState::posOffsetExpected` for smooth camera transitions
- Stores original camera offset to restore default position

## Code Structure

```
src/
├── main.cpp           - Plugin entry point and key handler setup
├── CameraManager.h    - Camera manager class declaration
├── CameraManager.cpp  - Camera state management and bone lookup
├── keyhandler/        - Keyboard input handling
└── pch.h             - Precompiled headers
```

## Configuration

Currently, the key bindings are hardcoded:
- F3 (scan code `0x3D`): PrismaUI toggle
- F4 (scan code `0x3E`): Camera cycle

To change the camera cycle key, modify `CAMERA_CYCLE_KEY` in `src/main.cpp`:

```cpp
const uint32_t CAMERA_CYCLE_KEY = 0x3E; // F4 key
```

Common scan codes:
- F1: `0x3B`
- F2: `0x3C`
- F3: `0x3D`
- F4: `0x3E`
- F5: `0x3F`
- F6: `0x40`

## Troubleshooting

### Camera doesn't move when pressing F4
1. Make sure you're in third-person view (press F to toggle)
2. Check the SKSE log for error messages
3. Verify that the player's 3D model is loaded (may take a moment after loading a save)

### Bone not found errors
- The bone names are case-sensitive and must match exactly
- Default bone names:
  - Head: `"NPC Head [Head]"`
  - Pelvis: `"NPC Pelvis [Pelv]"`
- Some custom skeleton mods may use different bone names

### Camera position seems wrong
- The camera calculates relative offset from player position to bone position
- Different character heights or skeleton scales may affect positioning
- Try different camera zoom levels with the mouse wheel

## Known Limitations

- Only works in third-person view
- Camera position is relative to player rotation
- Very fast character movement may cause temporary positioning issues
- Custom skeleton mods with different bone names are not automatically supported

## Future Improvements

- [ ] Configurable key bindings via INI file
- [ ] Support for custom bone names
- [ ] Per-frame camera update for smoother following during animations
- [ ] Camera distance adjustment along with position
- [ ] Support for multiple custom bone focus points
- [ ] MCM integration for in-game configuration

## Credits

- **CommonLibSSE-NG**: The foundation for this SKSE plugin
- **SKSE Team**: For making modding possible
- **PrismaUI**: For the UI framework used in the base template

## License

GPL-3.0 License

## Author

xwolkx

## Version

1.0.0
