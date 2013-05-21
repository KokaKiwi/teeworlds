#include <engine/plugins.h>
#include <engine/events.h>
#include <engine/server.h>

#include <game/server/gamecontroller.h>
#include <game/server/gamecontext.h>

#include "test.h"

PLUGIN_INFO(PLUGIN_SERVER, "test-plugin", "0.1.0", "KokaKiwi", "A test plugin");

static IKernel *s_Kernel;

KiwiGameController::KiwiGameController(CGameContext *pGameServer): IGameController(pGameServer)
{

}

void OnTickCallback(IEvents::EventType pEventType, void *pUserData, void *pEventData)
{

}

PLUGIN_EXPORT void PluginInit(IKernel *pKernel, IPlugin *pPlugin)
{
    s_Kernel = pKernel;

    IEvents *pEvents = pKernel->RequestInterface<IEvents>();
    CGameContext *pGameServer = pKernel->RequestInterface<CGameContext>();

    KiwiGameController *pKiwiGameController = new KiwiGameController(pGameServer);

    pEvents->Register("OnTick", OnTickCallback, 0);
    pGameServer->RegisterController("kiwi", pKiwiGameController);
}

PLUGIN_EXPORT void PluginDestroy(IKernel *pKernel, IPlugin *pPlugin)
{
    IEvents *pEvents = pKernel->RequestInterface<IEvents>();

    pEvents->Unregister("OnTick", OnTickCallback);
}
