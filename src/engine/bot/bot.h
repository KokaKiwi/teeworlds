#ifndef ENGINE_BOT_BOT_H_
#define ENGINE_BOT_BOT_H_

class CBot: public IBot
{
    IEngine *m_pEngine;
    IGameBot *m_pGameBot;
    IEngineMap *m_pMap;
    IConsole *m_pConsole;
    IStorage *m_pStorage;

    class CNetClient m_NetClient;
    class CMapChecker m_MapChecker;

    char m_aServerAddressStr[256];

    NETADDR m_ServerAddress;

    // pinging
    int64 m_PingStartTime;

    char m_aCurrentMap[256];
    unsigned m_CurrentMapCrc;

    char m_aCmdConnect[256];

    class CServerInfo m_CurrentServerInfo;
    int64 m_CurrentServerInfoRequestTime;

public:
    IEngine *Engine() { return m_pEngine; }
    IGameBot *GameBot() { return m_pGameBot; }
    IStorage *Storage() { return m_pStorage; }

    CBot();

    // send functions
    virtual int SendMsg(CMsgPacker *pMsg, int Flags);

    void SendInfo();
    void SendEnterGame();
    void SendReady();

    virtual bool ConnectionProblems();

    // state handling
    void SetState(int s);

    void OnEnterGame();
    virtual void EnterGame();

    virtual void Connect(const char *pAddress);
    virtual void Disconnect(const char *pReason = NULL);

    virtual void GetServerInfo(CServerInfo *pServerInfo);
    void ServerInfoRequest();

    virtual void Quit();

    virtual const char *ErrorString();

    const char *LoadMap(const char *pName, const char *pFilename, unsigned WantedCrc);
    const char *LoadMapSearch(const char *pMapName, int WantedCrc);

    void ProcessServerPacket(CNetChunk *pPacket);

    void PumpNetwork();

    void Update();

    void RegisterInterfaces();
    void InitInterfaces();

    void Run();

    static void Con_Connect(IConsole::IResult *pResult, void *pUserData);
    static void Con_Disconnect(IConsole::IResult *pResult, void *pUserData);
    static void Con_Quit(IConsole::IResult *pResult, void *pUserData);

    void RegisterCommands();
};

#endif /* ENGINE_BOT_BOT_H_ */
