#include <new>

#include <stdlib.h>
#include <stdarg.h>

#include <base/math.h>
#include <base/system.h>

#include <engine/bot.h>
#include <engine/client.h>
#include <engine/config.h>
#include <engine/console.h>
#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/map.h>
#include <engine/masterserver.h>
#include <engine/serverbrowser.h>
#include <engine/sound.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <engine/shared/config.h>
#include <engine/shared/compression.h>
#include <engine/shared/datafile.h>
#include <engine/shared/demo.h>
#include <engine/shared/filecollection.h>
#include <engine/shared/mapchecker.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/ringbuffer.h>
#include <engine/shared/snapshot.h>

#include <game/version.h>

#include <mastersrv/mastersrv.h>
#include <versionsrv/versionsrv.h>

#include "bot.h"

CBot::CBot()
{
    m_pEngine = NULL;
    m_pGameBot = NULL;
    m_pMap = NULL;
    m_pConsole = NULL;
    m_pStorage = NULL;

    m_GameTickSpeed = SERVER_TICK_SPEED;

    // pinging
    m_PingStartTime = 0;

    //
    m_aCurrentMap[0] = 0;
    m_CurrentMapCrc = 0;

    m_State = IClient::STATE_OFFLINE;
    m_aServerAddressStr[0] = 0;
}

// send functions
int CBot::SendMsg(CMsgPacker *pMsg, int Flags)
{
    CNetChunk Packet;

    if (State() == IClient::STATE_OFFLINE)
        return 0;

    mem_zero(&Packet, sizeof(CNetChunk));
    Packet.m_ClientID = 0;
    Packet.m_pData = pMsg->Data();
    Packet.m_DataSize = pMsg->Size();

    if(Flags & MSGFLAG_VITAL)
        Packet.m_Flags |= NETSENDFLAG_VITAL;
    if(Flags & MSGFLAG_FLUSH)
        Packet.m_Flags |= NETSENDFLAG_FLUSH;

    if (!(Flags & MSGFLAG_NOSEND))
        m_NetClient.Send(&Packet);

    return 0;
}

void CBot::SendInfo()
{
    CMsgPacker Msg(NETMSG_INFO, true);
    Msg.AddString(GameBot()->NetVersion(), 128);
    Msg.AddString(g_Config.m_Password, 128);
    SendMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}

void CBot::SendEnterGame()
{
    CMsgPacker Msg(NETMSG_ENTERGAME, true);
    SendMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}

void CBot::SendReady()
{
    CMsgPacker Msg(NETMSG_READY, true);
    SendMsg(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}

bool CBot::ConnectionProblems()
{
    return m_NetClient.GotProblems() != 0;
}

void CBot::SetState(int s)
{
    if(m_State == IClient::STATE_QUITING)
        return;

    // int Old = m_State;
    if(g_Config.m_Debug)
    {
        char aBuf[128];
        str_format(aBuf, sizeof(aBuf), "state change. last=%d current=%d", m_State, s);
        m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
    }
    m_State = s;
    // if(Old != s)
    //     GameBot()->OnStateChange(m_State, Old);
}

void CBot::OnEnterGame()
{

}

void CBot::EnterGame()
{
    if(State() == IClient::STATE_DEMOPLAYBACK)
        return;

    // now we will wait for two snapshots
    // to finish the connection
    SendEnterGame();
    OnEnterGame();
}

void CBot::Connect(const char *pAddress)
{
    char aBuf[512];
    int Port = 8303;

    Disconnect();

    str_copy(m_aServerAddressStr, pAddress, sizeof(m_aServerAddressStr));

    str_format(aBuf, sizeof(aBuf), "connecting to '%s'", m_aServerAddressStr);
    m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);

    ServerInfoRequest();

    if(net_addr_from_str(&m_ServerAddress, m_aServerAddressStr) != 0 && net_host_lookup(m_aServerAddressStr, &m_ServerAddress, m_NetClient.NetType()) != 0)
    {
        char aBufMsg[256];
        str_format(aBufMsg, sizeof(aBufMsg), "could not find the address of %s, connecting to localhost", aBuf);
        m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBufMsg);
        net_host_lookup("localhost", &m_ServerAddress, m_NetClient.NetType());
    }

    if(m_ServerAddress.port == 0)
        m_ServerAddress.port = Port;
    m_NetClient.Connect(&m_ServerAddress);
    SetState(IClient::STATE_CONNECTING);
}

void CBot::Disconnect(const char *pReason)
{
    char aBuf[512];
    str_format(aBuf, sizeof(aBuf), "disconnecting. reason='%s'", pReason?pReason:"unknown");
    m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);

    //
    m_pConsole->DeregisterTempAll();
    m_NetClient.Disconnect(pReason);
    SetState(IClient::STATE_OFFLINE);
    m_pMap->Unload();

    // clear the current server info
    mem_zero(&m_CurrentServerInfo, sizeof(m_CurrentServerInfo));
    mem_zero(&m_ServerAddress, sizeof(m_ServerAddress));
}

void CBot::GetServerInfo(CServerInfo *pServerInfo)
{
    mem_copy(pServerInfo, &m_CurrentServerInfo, sizeof(m_CurrentServerInfo));
}

void CBot::ServerInfoRequest()
{
    mem_zero(&m_CurrentServerInfo, sizeof(m_CurrentServerInfo));
    m_CurrentServerInfoRequestTime = 0;
}

void CBot::Quit()
{
    SetState(IClient::STATE_QUITING);
}

const char *CBot::ErrorString()
{
    return m_NetClient.ErrorString();
}

const char *CBot::LoadMap(const char *pName, const char *pFilename, unsigned WantedCrc)
{
    static char aErrorMsg[128];

    SetState(IClient::STATE_LOADING);

    if(!m_pMap->Load(pFilename))
    {
        str_format(aErrorMsg, sizeof(aErrorMsg), "map '%s' not found", pFilename);
        return aErrorMsg;
    }

    // get the crc of the map
    if(m_pMap->Crc() != WantedCrc)
    {
        str_format(aErrorMsg, sizeof(aErrorMsg), "map differs from the server. %08x != %08x", m_pMap->Crc(), WantedCrc);
        m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aErrorMsg);
        m_pMap->Unload();
        return aErrorMsg;
    }

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "loaded map '%s'", pFilename);
    m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);

    str_copy(m_aCurrentMap, pName, sizeof(m_aCurrentMap));
    m_CurrentMapCrc = m_pMap->Crc();

    return 0x0;
}



const char *CBot::LoadMapSearch(const char *pMapName, int WantedCrc)
{
    const char *pError = 0;
    char aBuf[512];
    str_format(aBuf, sizeof(aBuf), "loading map, map=%s wanted crc=%08x", pMapName, WantedCrc);
    m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
    SetState(IClient::STATE_LOADING);

    // try the normal maps folder
    str_format(aBuf, sizeof(aBuf), "maps/%s.map", pMapName);
    pError = LoadMap(pMapName, aBuf, WantedCrc);
    if(!pError)
        return pError;

    // try the downloaded maps
    str_format(aBuf, sizeof(aBuf), "downloadedmaps/%s_%08x.map", pMapName, WantedCrc);
    pError = LoadMap(pMapName, aBuf, WantedCrc);
    if(!pError)
        return pError;

    // search for the map within subfolders
    char aFilename[128];
    str_format(aFilename, sizeof(aFilename), "%s.map", pMapName);
    if(Storage()->FindFile(aFilename, "maps", IStorage::TYPE_ALL, aBuf, sizeof(aBuf)))
        pError = LoadMap(pMapName, aBuf, WantedCrc);

    return pError;
}

void CBot::ProcessServerPacket(CNetChunk *pPacket)
{
    CUnpacker Unpacker;
    Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);

    // unpack msgid and system flag
    int Msg = Unpacker.GetInt();
    int Sys = Msg&1;
    Msg >>= 1;

    if(Unpacker.Error())
        return;

    if (Sys)
    {
        // system message
        if(Msg == NETMSG_MAP_CHANGE)
        {
            const char *pMap = Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
            int MapCrc = Unpacker.GetInt();
            int MapSize = Unpacker.GetInt();
            // int MapChunkNum = Unpacker.GetInt();
            // int MapChunkSize = Unpacker.GetInt();
            const char *pError = 0;

            if(Unpacker.Error())
                return;

            // check for valid standard map
            if(!m_MapChecker.IsMapValid(pMap, MapCrc, MapSize))
                pError = "invalid standard map";

            // protect the player from nasty map names
            for(int i = 0; pMap[i]; i++)
            {
                if(pMap[i] == '/' || pMap[i] == '\\')
                    pError = "strange character in map name";
            }

            if(MapSize <= 0)
                pError = "invalid map size";

            if(pError)
                Disconnect(pError);
            else
            {
                pError = LoadMapSearch(pMap, MapCrc);

                if(!pError)
                {
                    m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/network", "loading done");
                    SendReady();
                }
                else
                {
                    // start map download
                }
            }
        }
        else if(Msg == NETMSG_CON_READY)
        {
            GameBot()->OnConnected();
        }
        else if(Msg == NETMSG_PING)
        {
            CMsgPacker Msg(NETMSG_PING_REPLY, true);
            SendMsg(&Msg, 0);
        }
        else if(Msg == NETMSG_PING_REPLY)
        {
            char aBuf[256];
            str_format(aBuf, sizeof(aBuf), "latency %.2f", (time_get() - m_PingStartTime)*1000 / (float)time_freq());
            m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client/network", aBuf);
        }
    }
    else
    {
        GameBot()->OnMessage(Msg, &Unpacker);
    }
}

void CBot::PumpNetwork()
{
    m_NetClient.Update();

    if(State() != IClient::STATE_DEMOPLAYBACK)
    {
        // check for errors
        if(State() != IClient::STATE_OFFLINE && State() != IClient::STATE_QUITING && m_NetClient.State() == NETSTATE_OFFLINE)
        {
            SetState(IClient::STATE_OFFLINE);
            Disconnect();
            char aBuf[256];
            str_format(aBuf, sizeof(aBuf), "offline error='%s'", m_NetClient.ErrorString());
            m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);
        }

        //
        if(State() == IClient::STATE_CONNECTING && m_NetClient.State() == NETSTATE_ONLINE)
        {
            // we switched to online
            m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", "connected, sending info");
            SetState(IClient::STATE_LOADING);
            SendInfo();
        }
    }

    // process packets
    CNetChunk Packet;
    while(m_NetClient.Recv(&Packet))
    {
        if(Packet.m_ClientID != -1)
            ProcessServerPacket(&Packet);
    }
}

void CBot::Update()
{
    // pump the network
    PumpNetwork();
}

void CBot::RegisterInterfaces()
{

}

void CBot::InitInterfaces()
{
    // fetch interfaces
    m_pEngine = Kernel()->RequestInterface<IEngine>();
    m_pGameBot = Kernel()->RequestInterface<IGameBot>();
    m_pMap = Kernel()->RequestInterface<IEngineMap>();
    m_pStorage = Kernel()->RequestInterface<IStorage>();
}

void CBot::Run()
{
    // open socket
    {
        NETADDR BindAddr;
        if(g_Config.m_Bindaddr[0] && net_host_lookup(g_Config.m_Bindaddr, &BindAddr, NETTYPE_ALL) == 0)
        {
            // got bindaddr
            BindAddr.type = NETTYPE_ALL;
        }
        else
        {
            mem_zero(&BindAddr, sizeof(BindAddr));
            BindAddr.type = NETTYPE_ALL;
        }
        if(!m_NetClient.Open(BindAddr, 0))
        {
            dbg_msg("client", "couldn't open socket");
            return;
        }
    }

    GameBot()->OnInit();

    char aBuf[256];
    str_format(aBuf, sizeof(aBuf), "version %s", GameBot()->NetVersion());
    m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);

    // process pending commands
    m_pConsole->StoreCommands(false);

    while (1)
    {
        // handle pending connects
        if(m_aCmdConnect[0])
        {
            str_copy(g_Config.m_UiServerAddress, m_aCmdConnect, sizeof(g_Config.m_UiServerAddress));
            Connect(m_aCmdConnect);
            m_aCmdConnect[0] = 0;
        }

        Update();

        // check conditions
        if(State() == IClient::STATE_QUITING)
            break;
    }

    Disconnect();
}

void CBot::Con_Connect(IConsole::IResult *pResult, void *pUserData)
{
    CBot *pSelf = (CBot *)pUserData;
    str_copy(pSelf->m_aCmdConnect, pResult->GetString(0), sizeof(pSelf->m_aCmdConnect));
}

void CBot::Con_Disconnect(IConsole::IResult *pResult, void *pUserData)
{
    CBot *pSelf = (CBot *)pUserData;
    pSelf->Disconnect();
}

void CBot::Con_Quit(IConsole::IResult *pResult, void *pUserData)
{
    CBot *pSelf = (CBot *)pUserData;
    pSelf->Quit();
}

void CBot::RegisterCommands()
{
    m_pConsole = Kernel()->RequestInterface<IConsole>();

    m_pConsole->Register("quit", "", CFGFLAG_CLIENT | CFGFLAG_STORE, Con_Quit, this, "Quit Teeworlds");
    m_pConsole->Register("connect", "s", CFGFLAG_CLIENT, Con_Connect, this, "Connect to the specified host/ip");
    m_pConsole->Register("disconnect", "", CFGFLAG_CLIENT, Con_Disconnect, this, "Disconnect from the server");
}

static CBot *CreateBot()
{
    return new CBot();
}

int main(int argc, const char **argv)
{
    CBot *pBot = CreateBot();
    IKernel *pKernel = IKernel::Create();
    pKernel->RegisterInterface(pBot);
    pBot->RegisterInterfaces();

    // create the components
    IEngine *pEngine = CreateEngine("Teeworlds");
    IConsole *pConsole = CreateConsole(CFGFLAG_CLIENT);
    IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_CLIENT, argc, argv); // ignore_convention
    IConfig *pConfig = CreateConfig();
    IEngineMap *pEngineMap = CreateEngineMap();

    {
        bool RegisterFail = false;

        RegisterFail = RegisterFail
            || !pKernel->RegisterInterface(pEngine)
            || !pKernel->RegisterInterface(pConsole)
            || !pKernel->RegisterInterface(pConfig)
            || !pKernel->RegisterInterface(static_cast<IEngineMap*>(pEngineMap))
            || !pKernel->RegisterInterface(static_cast<IMap*>(pEngineMap))
            || !pKernel->RegisterInterface(CreateGameBot())
            || !pKernel->RegisterInterface(pStorage);

        if (RegisterFail)
            return -1;
    }

    pEngine->Init();
    pConfig->Init();

    pBot->RegisterCommands();

    pBot->InitInterfaces();

    pKernel->RequestInterface<IGameBot>()->OnConsoleInit();

    if (argc > 1)
        pConsole->ParseArguments(argc - 1, &argv[1]);

    pConfig->RestoreStrings();

    pBot->Engine()->InitLogfile();

    dbg_msg("bot", "starting...");
    pBot->Run();

    pConfig->Save();

    delete pBot;
    delete pKernel;
    delete pEngine;
    delete pConsole;
    delete pStorage;
    delete pConfig;
    delete pEngineMap;
}
