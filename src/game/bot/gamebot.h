#ifndef GAME_BOT_GAMEBOT_H_
#define GAME_BOT_GAMEBOT_H_

#include <base/vmath.h>
#include <engine/bot.h>
#include <engine/console.h>
#include <game/layers.h>
#include <game/gamecore.h>

class CGameBot: public IGameBot
{
    CNetObjHandler m_NetObjHandler;

    class IEngine *m_pEngine;
    class IInput *m_pInput;
    class IBot *m_pBot;
    class IConsole *m_pConsole;
    class IStorage *m_pStorage;

public:
    // client data
    struct CClientData
    {
        char m_aName[MAX_NAME_LENGTH];
        char m_aClan[MAX_CLAN_LENGTH];
        int m_Country;
        char m_aaSkinPartNames[6][24];
        int m_aUseCustomColors[6];
        int m_aSkinPartColors[6];
        int m_SkinPartIDs[6];
        int m_Team;
        int m_Emoticon;
        int m_EmoticonStart;
        CCharacterCore m_Predicted;

        CTeeRenderInfo m_SkinInfo; // this is what the server reports
        CTeeRenderInfo m_RenderInfo; // this is what we use

        float m_Angle;
        bool m_Active;
        bool m_ChatIgnore;
        bool m_Friend;

        void UpdateRenderInfo(CGameBot *pGameBot, bool UpdateSkinInfo);
        void Reset(CGameBot *pGameBot);
    };

    CClientData m_aClients[MAX_CLIENTS];
    int m_LocalClientID;
    int m_TeamCooldownTick;

    struct CGameInfo
    {
        int m_GameFlags;
        int m_ScoreLimit;
        int m_TimeLimit;
        int m_MatchNum;
        int m_MatchCurrent;

        int m_NumPlayers;
        int m_aTeamSize[NUM_TEAMS];
    };

    CGameInfo m_GameInfo;

    struct CServerSettings
    {
        bool m_KickVote;
        int m_KickMin;
        bool m_SpecVote;
        bool m_TeamLock;
        bool m_TeamBalance;
        int m_PlayerSlots;
    } m_ServerSettings;

    IKernel *Kernel() { return IInterface::Kernel(); }
    IEngine *Engine() const { return m_pEngine; }
    class IBot *Bot() const { return m_pBot; }
    class IStorage *Storage() const { return m_pStorage; }
    class IConsole *Console() { return m_pConsole; }

    virtual void OnConsoleInit();

    virtual void OnInit();
    virtual void OnConnected();
    virtual void OnMessage(int MsgID, CUnpacker *pUnpacker);

    virtual const char *Version();
    virtual const char *NetVersion();

    // void DoEnterMessage(const char *pName, int Team);
    // void DoLeaveMessage(const char *pName, const char *pReason);
    // void DoTeamChangeMessage(const char *pName, int Team);

    // actions
    void SendSwitchTeam(int Team);
    void SendStartInfo();
    void SendKill();
    void SendReadyChange();
};

#endif /* GAME_BOT_GAMEBOT_H_ */
