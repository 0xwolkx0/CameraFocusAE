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
void LogPlayerBonesAndFindNearNeck()
{
    auto player = RE::PlayerCharacter::GetSingleton();
    if (!player || !player->Get3D()) {
        logger::info("Player or 3D not available");
        return;
    }

    auto root = player->Get3D();

    // First, find the neck bone to get reference Z coordinate
    auto neckNode = root->GetObjectByName("NPC Neck [Neck]");
    if (!neckNode) {
        logger::info("Neck bone not found");
        return;
    }

    float neckZ = neckNode->world.translate.z;
    logger::info("=== Neck bone Z coordinate: {:.3f} ===", neckZ);
    logger::info("");

    // Collect all bones
    std::vector<RE::NiAVObject*> allBones;
    std::function<void(RE::NiAVObject*)> collectBones = [&](RE::NiAVObject* node) {
        if (!node) return;

        allBones.push_back(node);

        auto asNode = node->AsNode();
        if (asNode) {
            for (auto& child : asNode->children) {
                if (child) {
                    collectBones(child.get());
                }
            }
        }
    };

    collectBones(root);

    // Log all bones
    logger::info("=== All Player Bones ({} total) ===", allBones.size());
    for (auto* bone : allBones) {
        auto pos = bone->world.translate;
        logger::info("{}: ({:.3f}, {:.3f}, {:.3f})",
            bone->name.c_str(),
            pos.x, pos.y, pos.z);
    }

    logger::info("");

    // Find bones with similar Z to neck (within ±3)
    logger::info("=== Bones near neck Z coordinate ({:.3f} ± 3) ===", neckZ);
    for (auto* bone : allBones) {
        float boneZ = bone->world.translate.z;
        float diff = std::abs(boneZ - neckZ);

        if (diff <= 3.0f) {
            auto pos = bone->world.translate;
            logger::info("{}: ({:.3f}, {:.3f}, {:.3f}) [diff: {:.3f}]",
                bone->name.c_str(),
                pos.x, pos.y, pos.z,
                diff);
        }
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
    //auto tps = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
    auto player = RE::PlayerCharacter::GetSingleton();
    neckPos = player->Get3D()->AsNode()->GetObjectByName("NPC Neck [Neck]")->world.translate;
    neckPos = {neckPos.x,neckPos.y,neckPos.z};
    rootPos = player->Get3D()->AsNode()->GetObjectByName("NPC Root [Root]")->world.translate;
    auto niCamera = GetNiCamera(playerCamera);
    auto tps = skyrim_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
    //tps->posOffsetActual = tps-> posOffsetExpected = {0,0,0};
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

    if(currentFocus != CameraFocusState::Default){
        // Now modify the camera transform to override what the game just did
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
        //RE::NiPoint3 currentPos = GetNiCamera(playerCamera)->world.translate;

        RE::NiPoint3 rootPosOffset = {rootPos.x, rootPos.y, neckPos.z}; 
        
        //GetNiCamera(playerCamera)->world.rotate = rotation;
        GetNiCamera(playerCamera)->world.translate -= rootPosOffset - bonePos;
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
