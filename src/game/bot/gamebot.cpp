#include <engine/client.h>
#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/demo.h>
#include <engine/map.h>
#include <engine/storage.h>
#include <engine/sound.h>
#include <engine/serverbrowser.h>
#include <engine/shared/demo.h>
#include <engine/shared/config.h>

#include <game/client/components/skins.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/version.h>

#include "gamebot.h"

const char * const CSkins::ms_apSkinPartNames[NUM_SKINPARTS] = {"body", "tattoo", "decoration", "hands", "feet", "eyes"};
const char * const CSkins::ms_apColorComponents[NUM_COLOR_COMPONENTS] = {"hue", "sat", "lgt", "alp"};

char *const CSkins::ms_apSkinVariables[NUM_SKINPARTS] = {g_Config.m_PlayerSkinBody, g_Config.m_PlayerSkinTattoo, g_Config.m_PlayerSkinDecoration,
    g_Config.m_PlayerSkinHands, g_Config.m_PlayerSkinFeet, g_Config.m_PlayerSkinEyes};
int *const CSkins::ms_apUCCVariables[NUM_SKINPARTS] = {&g_Config.m_PlayerUseCustomColorBody, &g_Config.m_PlayerUseCustomColorTattoo, &g_Config.m_PlayerUseCustomColorDecoration,
    &g_Config.m_PlayerUseCustomColorHands, &g_Config.m_PlayerUseCustomColorFeet, &g_Config.m_PlayerUseCustomColorEyes};
int *const CSkins::ms_apColorVariables[NUM_SKINPARTS] = {&g_Config.m_PlayerColorBody, &g_Config.m_PlayerColorTattoo, &g_Config.m_PlayerColorDecoration,
    &g_Config.m_PlayerColorHands, &g_Config.m_PlayerColorFeet, &g_Config.m_PlayerColorEyes};

const char *CGameBot::Version() { return GAME_VERSION; }
const char *CGameBot::NetVersion() { return GAME_NETVERSION; }

void CGameBot::OnConsoleInit()
{
    m_pEngine = Kernel()->RequestInterface<IEngine>();
    m_pBot = Kernel()->RequestInterface<IBot>();
    m_pConsole = Kernel()->RequestInterface<IConsole>();
    m_pStorage = Kernel()->RequestInterface<IStorage>();
}

void CGameBot::OnInit()
{
    int64 Start = time_get();

    int64 End = time_get();
    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "initialisation finished after %.2fms", ((End-Start)*1000)/(float)time_freq());
    Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "gameclient", aBuf);
}

void CGameBot::OnConnected()
{
    SendStartInfo();
}

void CGameBot::OnMessage(int MsgId, CUnpacker *pUnpacker)
{
    void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgId, pUnpacker);
    if(!pRawMsg)
    {
        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgId), MsgId, m_NetObjHandler.FailedMsgOn());
        Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
        return;
    }

    if(MsgId == NETMSGTYPE_SV_CLIENTINFO && Bot()->State() != IClient::STATE_DEMOPLAYBACK)
    {
        CNetMsg_Sv_ClientInfo *pMsg = (CNetMsg_Sv_ClientInfo *)pRawMsg;

        if(pMsg->m_Local)
        {
            if(m_LocalClientID != -1)
            {
                if(g_Config.m_Debug)
                    Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", "invalid local clientinfo");
                return;
            }
            m_LocalClientID = pMsg->m_ClientID;
        }
        else
        {
            if(m_aClients[pMsg->m_ClientID].m_Active)
            {
                if(g_Config.m_Debug)
                    Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", "invalid clientinfo");
                return;
            }
        }

        m_aClients[pMsg->m_ClientID].m_Active = true;
        m_aClients[pMsg->m_ClientID].m_Team  = pMsg->m_Team;
        str_copy(m_aClients[pMsg->m_ClientID].m_aName, pMsg->m_pName, sizeof(m_aClients[pMsg->m_ClientID].m_aName));
        str_copy(m_aClients[pMsg->m_ClientID].m_aClan, pMsg->m_pClan, sizeof(m_aClients[pMsg->m_ClientID].m_aClan));
        m_aClients[pMsg->m_ClientID].m_Country = pMsg->m_Country;
        for(int i = 0; i < 6; i++)
        {
            str_copy(m_aClients[pMsg->m_ClientID].m_aaSkinPartNames[i], pMsg->m_apSkinPartNames[i], 24);
            m_aClients[pMsg->m_ClientID].m_aUseCustomColors[i] = pMsg->m_aUseCustomColors[i];
            m_aClients[pMsg->m_ClientID].m_aSkinPartColors[i] = pMsg->m_aSkinPartColors[i];
        }

        // update friend state
        // m_aClients[pMsg->m_ClientID].m_Friend = Friends()->IsFriend(m_aClients[pMsg->m_ClientID].m_aName, m_aClients[pMsg->m_ClientID].m_aClan, true);

        m_GameInfo.m_NumPlayers++;
        // calculate team-balance
        if(m_aClients[pMsg->m_ClientID].m_Team != TEAM_SPECTATORS)
            m_GameInfo.m_aTeamSize[m_aClients[pMsg->m_ClientID].m_Team]++;
    }
    else if(MsgId == NETMSGTYPE_SV_CLIENTDROP && Bot()->State() != IClient::STATE_DEMOPLAYBACK)
    {
        CNetMsg_Sv_ClientDrop *pMsg = (CNetMsg_Sv_ClientDrop *)pRawMsg;

        if(m_LocalClientID == pMsg->m_ClientID || !m_aClients[pMsg->m_ClientID].m_Active)
        {
            if(g_Config.m_Debug)
                Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", "invalid clientdrop");
            return;
        }

        CNetMsg_De_ClientLeave Msg;
        Msg.m_pName = m_aClients[pMsg->m_ClientID].m_aName;
        Msg.m_pReason = pMsg->m_pReason;
        Bot()->SendPackMsg(&Msg, MSGFLAG_NOSEND | MSGFLAG_RECORD);

        m_GameInfo.m_NumPlayers--;
        // calculate team-balance
        if(m_aClients[pMsg->m_ClientID].m_Team != TEAM_SPECTATORS)
            m_GameInfo.m_aTeamSize[m_aClients[pMsg->m_ClientID].m_Team]--;

        m_aClients[pMsg->m_ClientID].Reset(this);
    }
    else if(MsgId == NETMSGTYPE_SV_GAMEINFO && Bot()->State() != IClient::STATE_DEMOPLAYBACK)
    {
        CNetMsg_Sv_GameInfo *pMsg = (CNetMsg_Sv_GameInfo *)pRawMsg;

        m_GameInfo.m_GameFlags = pMsg->m_GameFlags;
        m_GameInfo.m_ScoreLimit = pMsg->m_ScoreLimit;
        m_GameInfo.m_TimeLimit = pMsg->m_TimeLimit;
        m_GameInfo.m_MatchNum = pMsg->m_MatchNum;
        m_GameInfo.m_MatchCurrent = pMsg->m_MatchCurrent;
    }
    else if(MsgId == NETMSGTYPE_SV_SERVERSETTINGS && Bot()->State() != IClient::STATE_DEMOPLAYBACK)
    {
        CNetMsg_Sv_ServerSettings *pMsg = (CNetMsg_Sv_ServerSettings *)pRawMsg;

        m_ServerSettings.m_KickVote = pMsg->m_KickVote;
        m_ServerSettings.m_KickMin = pMsg->m_KickMin;
        m_ServerSettings.m_SpecVote = pMsg->m_SpecVote;
        m_ServerSettings.m_TeamLock = pMsg->m_TeamLock;
        m_ServerSettings.m_TeamBalance = pMsg->m_TeamBalance;
        m_ServerSettings.m_PlayerSlots = pMsg->m_PlayerSlots;
    }
    else if(MsgId == NETMSGTYPE_SV_TEAM)
    {
        CNetMsg_Sv_Team *pMsg = (CNetMsg_Sv_Team *)pRawMsg;

        if(Bot()->State() != IClient::STATE_DEMOPLAYBACK)
        {
            // calculate team-balance
            if(m_aClients[pMsg->m_ClientID].m_Team != TEAM_SPECTATORS)
                m_GameInfo.m_aTeamSize[m_aClients[pMsg->m_ClientID].m_Team]--;
            m_aClients[pMsg->m_ClientID].m_Team = pMsg->m_Team;
            if(m_aClients[pMsg->m_ClientID].m_Team != TEAM_SPECTATORS)
                m_GameInfo.m_aTeamSize[m_aClients[pMsg->m_ClientID].m_Team]++;

            if(pMsg->m_ClientID == m_LocalClientID)
                m_TeamCooldownTick = pMsg->m_CooldownTick;
        }
    }
    else if (MsgId == NETMSGTYPE_SV_READYTOENTER)
    {
        Bot()->EnterGame();
    }
    else if(MsgId == NETMSGTYPE_DE_CLIENTENTER && Bot()->State() == IClient::STATE_DEMOPLAYBACK)
    {
        // CNetMsg_De_ClientEnter *pMsg = (CNetMsg_De_ClientEnter *)pRawMsg;
        // DoEnterMessage(pMsg->m_pName, pMsg->m_Team);
    }
    else if(MsgId == NETMSGTYPE_DE_CLIENTLEAVE && Bot()->State() == IClient::STATE_DEMOPLAYBACK)
    {
        // CNetMsg_De_ClientLeave *pMsg = (CNetMsg_De_ClientLeave *)pRawMsg;
        // DoLeaveMessage(pMsg->m_pName, pMsg->m_pReason);
    }
}

void CGameBot::CClientData::Reset(CGameBot *pGameBot)
{
    m_aName[0] = 0;
    m_aClan[0] = 0;
    m_Country = -1;
    m_Team = 0;
    m_Angle = 0;
    m_Emoticon = 0;
    m_EmoticonStart = -1;
    m_Active = false;
    m_ChatIgnore = false;
    m_Friend = false;
}

void CGameBot::SendSwitchTeam(int Team)
{
    CNetMsg_Cl_SetTeam Msg;
    Msg.m_Team = Team;
    Bot()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

void CGameBot::SendStartInfo()
{
    CNetMsg_Cl_StartInfo Msg;
    Msg.m_pName = g_Config.m_PlayerName;
    Msg.m_pClan = g_Config.m_PlayerClan;
    Msg.m_Country = g_Config.m_PlayerCountry;
    for(int p = 0; p < CSkins::NUM_SKINPARTS; p++)
    {
        Msg.m_apSkinPartNames[p] = CSkins::ms_apSkinVariables[p];
        Msg.m_aUseCustomColors[p] = *CSkins::ms_apUCCVariables[p];
        Msg.m_aSkinPartColors[p] = *CSkins::ms_apColorVariables[p];
    }
    Bot()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_FLUSH);
}

void CGameBot::SendKill()
{
    CNetMsg_Cl_Kill Msg;
    Bot()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

void CGameBot::SendReadyChange()
{
    CNetMsg_Cl_ReadyChange Msg;
    Bot()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

IGameBot *CreateGameBot()
{
    return new CGameBot();
}
