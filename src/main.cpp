#include <keyhandler/keyhandler.h>
#include "CameraManager.h"
#include "RE/N/NiMatrix3.h"
#include "RE/N/NiPoint3.h"

using UpdateFunc = void(RE::ThirdPersonState*, RE::BSTSmartPointer<RE::TESCameraState>&);
std::uintptr_t _OriginalUpdate = 0;
CameraFocusState currentFocus = CameraFocusState::Default;
RE::NiPoint3 defaultOffset;
RE::NiPoint3 neckPos;
RE::NiPoint3 rootPos;

RE::NiPointer<RE::NiCamera> GetNiCamera(RE::PlayerCamera* camera)
{
    if (camera->cameraRoot->children.empty()) return nullptr;
    for (auto& entry : camera->cameraRoot->children) {
        auto asCamera = skyrim_cast<RE::NiCamera*>(entry.get());
        if (asCamera) return RE::NiPointer(asCamera);
    }
    return nullptr;
}

void CycleState(bool reset = false){
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
    auto player = RE::PlayerCharacter::GetSingleton();
    neckPos = player->Get3D()->AsNode()->GetObjectByName("Camera3rd [Cam3]")->world.translate;
    neckPos = {neckPos.x,neckPos.y,neckPos.z};
    rootPos = player->Get3D()->AsNode()->GetObjectByName("NPC")->world.translate;
    auto niCamera = GetNiCamera(playerCamera);
    auto tps = skyrim_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
    // Cycle to next state
    if(reset){
        currentFocus = CameraFocusState::Default;
        tps->posOffsetExpected = defaultOffset;
        logger::info("Camera state: Default");
    } else {
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
                currentFocus = CameraFocusState::Head;
                tps->posOffsetExpected = {0,0,0};
                logger::info("Camera state: Head");
                break;
        }
    }
}

void InstallThirdPersonUpdateHook()
{
    auto vtbl = REL::Relocation<std::uintptr_t>(RE::VTABLE_ThirdPersonState[0]);

    _OriginalUpdate = vtbl.write_vfunc(0x3, &HookedUpdate);

    SKSE::log::info("ThirdPersonState::Update hook installed");
}



void HookedUpdate(RE::ThirdPersonState* a_this, RE::BSTSmartPointer<RE::TESCameraState>& a_nextState)
{
    auto originalFunc = reinterpret_cast<UpdateFunc*>(_OriginalUpdate);
    originalFunc(a_this, a_nextState);

    if(currentFocus != CameraFocusState::Default){
        auto playerCamera = RE::PlayerCamera::GetSingleton();
        RE::NiMatrix3 rotation = GetNiCamera(playerCamera)->world.rotate;
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
        RE::NiPoint3 bonePos = player->Get3D()->AsNode()->GetObjectByName(focusBoneName)->world.translate;
        RE::NiPoint3 rootPosOffset = {rootPos.x, rootPos.y, neckPos.z}; 
        GetNiCamera(playerCamera)->world.translate -= rootPosOffset - bonePos;
    };
}

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message)
{
    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        // Next lines is custom KEY DOWN / KEY UP realisation which bases at "src/keyhandler".
        KeyHandler::RegisterSink();
        KeyHandler* keyHandler = KeyHandler::GetSingleton();
        const uint32_t CAMERA_CYCLE_KEY = 0x4; // Mouse button 4
        const uint32_t CAMERA_RESET_KEY = 0x3; // Mouse button 5

        // Press F4 to cycle camera focus: Default -> Head -> Pelvis -> Default
        KeyHandlerEvent focusCameraEventHandler = keyHandler->Register(CAMERA_CYCLE_KEY, KeyEventType::KEY_DOWN, []() {
            CycleState();
        });
        KeyHandlerEvent resetCameraEventHandler = keyHandler->Register(CAMERA_RESET_KEY, KeyEventType::KEY_DOWN, []() {
            CycleState(true);
        });
        // If you want to unregister the key event handlers:
        // keyHandler->Unregister(toggleEventHandler);
        // keyHandler->Unregister(cameraEventHandler);
        break;
    }
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
    InstallThirdPersonUpdateHook();

    return true;
}
