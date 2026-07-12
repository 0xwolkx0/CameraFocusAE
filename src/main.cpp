#include <keyhandler/keyhandler.h>
#include "CameraManager.h"

using UpdateFunc = void(RE::ThirdPersonState*, RE::BSTSmartPointer<RE::TESCameraState>&);
std::uintptr_t _OriginalUpdate = 0;
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
RE::NiPointer<RE::NiCamera> GetNiCamera(RE::PlayerCamera* camera)
{
    if (camera->cameraRoot->children.empty()) return nullptr;
    for (auto& entry : camera->cameraRoot->children) {
        auto asCamera = skyrim_cast<RE::NiCamera*>(entry.get());
        if (asCamera) return RE::NiPointer(asCamera);
    }
    return nullptr;
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
    auto player = RE::PlayerCharacter::GetSingleton();
    RE::NiPoint3 headPos = player->Get3D()->AsNode()->GetObjectByName("NPC Head [Head]")->world.translate;
    RE::NiPoint3 headLocalPos = player->Get3D()->AsNode()->GetObjectByName("NPC Head [Head]")->local.translate;
    RE::NiPoint3 pelvisPos = player->Get3D()->AsNode()->GetObjectByName("NPC Pelvis [Pelv]")->world.translate;
    RE::NiPoint3 pelvisLocalPos = player->Get3D()->AsNode()->GetObjectByName("NPC Pelvis [Pelv]")->local.translate;
    // Cycle to next state
    switch (currentFocus) {
        case CameraFocusState::Default:
            defaultOffset = tps->posOffsetActual;
            logger::info("Camera default position: World ({}, {}, {}) Local ({}, {}, {})", GetNiCamera(playerCamera)->world.translate.x, GetNiCamera(playerCamera)->world.translate.y, GetNiCamera(playerCamera)->world.translate.z, GetNiCamera(playerCamera)->local.translate.x, GetNiCamera(playerCamera)->local.translate.y, GetNiCamera(playerCamera)->local.translate.z);

            currentFocus = CameraFocusState::Head;
            tps->posOffsetExpected = {0,0,0};
            logger::info("Camera state: Head");
            break;
        case CameraFocusState::Head:
            logger::info("Camera Head coordinates: World ({}, {}, {}) Local ({}, {}, {})", GetNiCamera(playerCamera)->world.translate.x, GetNiCamera(playerCamera)->world.translate.y, GetNiCamera(playerCamera)->world.translate.z, GetNiCamera(playerCamera)->local.translate.x, GetNiCamera(playerCamera)->local.translate.y, GetNiCamera(playerCamera)->local.translate.z);
            logger::info("Head position: ({}, {}, {})", headPos.x, headPos.y, headPos.z);
            logger::info("Head local position: ({}, {}, {})", headLocalPos.x, headLocalPos.y, headLocalPos.z);
            currentFocus = CameraFocusState::Pelvis;
            tps->posOffsetExpected = {0,0,0};
            logger::info("Camera state: Pelvis");
            break;
        case CameraFocusState::Pelvis:
            logger::info("Camera Pelvis coordinates: World ({}, {}, {}) Local ({}, {}, {})", GetNiCamera(playerCamera)->world.translate.x, GetNiCamera(playerCamera)->world.translate.y, GetNiCamera(playerCamera)->world.translate.z, GetNiCamera(playerCamera)->local.translate.x, GetNiCamera(playerCamera)->local.translate.y, GetNiCamera(playerCamera)->local.translate.z);
            logger::info("Pelvis position: ({}, {}, {})", pelvisPos.x, pelvisPos.y, pelvisPos.z);
            logger::info("Pelvis local position: ({}, {}, {})", pelvisLocalPos.x, pelvisLocalPos.y, pelvisLocalPos.z);
            currentFocus = CameraFocusState::Default;
            tps->posOffsetExpected = defaultOffset;
            logger::info("Camera state: Default");
            break;
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

    auto playerCamera = RE::PlayerCamera::GetSingleton();
    auto& cameraNode = playerCamera->cameraRoot;

    if(currentFocus != CameraFocusState::Default){
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
        RE::NiPoint3 bonePos = player->Get3D()->AsNode()->GetObjectByName(focusBoneName)->world.translate;
        auto currentPos = GetNiCamera(playerCamera)->world.translate;
        GetNiCamera(playerCamera)->world.translate = {(currentPos.x*2) - bonePos.x, (currentPos.y*2) - bonePos.y, (currentPos.z*2) - bonePos.z};
        //cameraNode->local.translate = cameraNode->world.translate = GetNiCamera(playerCamera)->world.translate = bonePos;
/*         if (playerCamera->currentState->id == RE::CameraState::kThirdPerson)
            skyrim_cast<RE::ThirdPersonState*>(playerCamera->currentState.get())->translation = cameraNode->local.translate; */
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
    InstallThirdPersonUpdateHook();

    return true;
}
