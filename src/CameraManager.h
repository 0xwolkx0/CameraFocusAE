#pragma once

#include <RE/Skyrim.h>

enum class CameraFocusState {
    Default,
    Head,
    Pelvis
};
void HookedUpdate(RE::ThirdPersonState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState);