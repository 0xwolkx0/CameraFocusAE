#include "PrismaUI_API.h"
#include <keyhandler/keyhandler.h>
#include "CameraManager.h"

PRISMA_UI_API::IVPrismaUI1* PrismaUI;

static void SKSEMessageHandler(SKSE::MessagingInterface::Message* message)
{
    switch (message->type) {
    case SKSE::MessagingInterface::kDataLoaded:
        // Next lines is custom KEY DOWN / KEY UP realisation which bases at "src/keyhandler".
        KeyHandler::RegisterSink();
        KeyHandler* keyHandler = KeyHandler::GetSingleton();
        const uint32_t CAMERA_CYCLE_KEY = 0x3E; // F4 key

        // Press F4 to cycle camera focus: Default -> Head -> Pelvis -> Default
        KeyHandlerEvent cameraEventHandler = keyHandler->Register(CAMERA_CYCLE_KEY, KeyEventType::KEY_DOWN, []() {
            CameraManager::GetSingleton()->CycleState();
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

    return true;
}
