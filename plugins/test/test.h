#ifndef TEST_H
#define TEST_H

#include <game/server/gamecontroller.h>
#include <game/server/gamecontext.h>

class KiwiGameController : public IGameController
{
public:
    KiwiGameController(CGameContext *pGameServer);
};

#endif
