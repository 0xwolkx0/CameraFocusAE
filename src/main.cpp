#include <keyhandler/keyhandler.h>
#include "CameraManager.h"

using UpdateFunc = void(RE::ThirdPersonState*, RE::BSTSmartPointer<RE::TESCameraState>&);
using UpdateFunc2 = void(RE::ThirdPersonState* a_state,float* rotation,bool a_flag,bool a_someFlag);
std::uintptr_t _OriginalUpdate = 0;
std::uintptr_t func = 0;
CameraFocusState currentFocus = CameraFocusState::Default;
RE::NiPoint3 defaultOffset;
void HookedUpdate(RE::ThirdPersonState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState);

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message)
{
    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        // Next lines is custom KEY DOWN / KEY UP realisation which bases at "src/keyhandler".
        KeyHandler::RegisterSink();
        KeyHandler* keyHandler = KeyHandler::GetSingleton();
        const uint32_t CAMERA_CYCLE_KEY = 0x3D; // F3 key

        // Press F4 to cycle camera focus: Default -> Head -> Pelvis -> Default
        KeyHandlerEvent cameraEventHandler = keyHandler->Register(CAMERA_CYCLE_KEY, KeyEventType::KEY_DOWN, []() {
            CycleState();
        });

        // If you want to unregister the key event handlers:
        // keyHandler->Unregister(toggleEventHandler);
        // keyHandler->Unregister(cameraEventHandler);
        break;
    }
}

void CycleState() {
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
    auto tps = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
    // Cycle to next state
    switch (currentFocus) {
        case CameraFocusState::Default:
            defaultOffset = tps->posOffsetActual;
            currentFocus = CameraFocusState::Head;
            tps->posOffsetExpected = {0,0,0};
            logger::info("Camera state: Head");
            break;
        case CameraFocusState::Head:
            currentFocus = CameraFocusState::Pelvis;
            tps->posOffsetExpected = {0,0,0};
            logger::info("Camera state: Pelvis");
            break;
        case CameraFocusState::Pelvis:
            currentFocus = CameraFocusState::Default;
            tps->posOffsetExpected = defaultOffset;
            logger::info("Camera state: Default");
            break;
    }
}

/* void InstallThirdPersonUpdateHook()
{
    auto vtbl = REL::Relocation<std::uintptr_t>(RE::VTABLE_ThirdPersonState[0]);

    _OriginalUpdate = vtbl.write_vfunc(0x3, &HookedUpdate);

    SKSE::log::info("ThirdPersonState::Update hook installed");
} */



void Install()
{
    SKSE::AllocTrampoline(1 << 8);
    auto& trampoline = SKSE::GetTrampoline();

    // IDA / Ghidra Address [AE 1408e8640 REL: 50911]

    std::array targets{
        std::make_pair(
            RELOCATION_ID(49960, 50896),  // AE 1408E7810 SE 14084F490
            REL::VariantOffset{ 0x144, 0x1E8, 0x147 }  // Offset to the CALL instruction
        ),
        std::make_pair(
            RELOCATION_ID(49966, 50902),  // AE sub_1408E7C50 SE 14084f830  ThirdPersonState::ResetFreeRotation() 
            REL::VariantOffset{ 0x5B, 0x5B, 0x5B }  // Offset to the CALL instruction
        )
    };

    for (auto& [id, offset] : targets) {
        REL::Relocation<std::uintptr_t> target(id, offset);
        func = trampoline.write_call<5>(target.address(), thunk);
    }

    logger::info("ThirdPersonState_SetRotation hook installed");
}

void thunk(
    RE::ThirdPersonState* a_state,
    float* rotation,
    bool a_flag,
    bool a_someFlag)
{
    auto originalFunc = reinterpret_cast<UpdateFunc2*>(func);
    originalFunc(a_state, rotation, a_flag, a_someFlag);
    if(currentFocus == CameraFocusState::Default){
    } else {
        // Now modify the camera transform to override what the game just did
        auto focusBoneName = "";
        switch(currentFocus){
            case CameraFocusState::Head:
                focusBoneName = "NPC Head [Head]";
                break;
            case CameraFocusState::Pelvis:
                focusBoneName = "NPC Pelvis [Pelv]";
                break;
        }
        auto player = RE::PlayerCharacter::GetSingleton();

        if (a_state->camera && a_state->camera->cameraRoot) {
            RE::NiPoint3 bonePos = player->Get3D()->AsNode()->GetObjectByName(focusBoneName)->world.translate;
            a_state->camera->cameraRoot->world.translate = bonePos;
            if (a_state->camera->cameraRoot->parent) {
                RE::NiUpdateData updateData;
                updateData.time = 0.0f;
                updateData.flags = RE::NiUpdateData::Flag::kDirty;
                RE::NiNode* root = a_state->camera->cameraRoot->parent;
                while (root && root->parent) {
                    root = root->parent;
                }
                root->Update(updateData);
            }
        }
    };
}

void HookedUpdate(RE::ThirdPersonState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState)
{
    // Call the original game logic first
    // Fallback if default state
    auto originalFunc = reinterpret_cast<UpdateFunc*>(_OriginalUpdate);
    originalFunc(a_this, a_nextState);
    if(currentFocus == CameraFocusState::Default){

    } else {
        // Now modify the camera transform to override what the game just did
        auto focusBoneName = "";
        switch(currentFocus){
            case CameraFocusState::Head:
                focusBoneName = "NPC Head [Head]";
                break;
            case CameraFocusState::Pelvis:
                focusBoneName = "NPC Pelvis [Pelv]";
                break;
        }
        auto player = RE::PlayerCharacter::GetSingleton();

        if (a_this->camera && a_this->camera->cameraRoot) {
            RE::NiPoint3 bonePos = player->Get3D()->AsNode()->GetObjectByName(focusBoneName)->world.translate;
            a_this->camera->cameraRoot->world.translate = bonePos;
        }
    };

}


extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
    REL::Module::reset();

    auto g_messaging = reinterpret_cast<SKSE::MessagingInterface*>(a_skse->QueryInterface(SKSE::LoadInterface::kMessaging));

    if (!g_messaging) {
        logger::critical("Failed to load messaging interface! This error is fatal, plugin will not load.");
        return false;
    }

    logger::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

    SKSE::Init(a_skse);
    SKSE::AllocTrampoline(1 << 10);

    g_messaging->RegisterListener("SKSE", SKSEMessageHandler);
    //InstallThirdPersonUpdateHook();
    Install();

    return true;
}
