#ifndef ENGINE_BOT_H_
#define ENGINE_BOT_H_
#include "kernel.h"

#include "message.h"

class IBot: public IInterface
{
	MACRO_INTERFACE("bot", 0)

protected:
	int m_State;

	int m_GameTickSpeed;

public:
	inline int State() const { return m_State; }

	// actions
	virtual void Connect(const char *pAddress) = 0;
	virtual void Disconnect(const char *pReason = NULL) = 0;
	virtual void Quit() = 0;

	// networking
	virtual void EnterGame() = 0;

	// server info
	virtual void GetServerInfo(class CServerInfo *pServerInfo) = 0;

	virtual int SendMsg(CMsgPacker *pMsg, int Flags) = 0;

	template<class T>
	int SendPackMsg(T *pMsg, int Flags)
	{
		CMsgPacker Packer(pMsg->MsgID(), false);
		if (pMsg->Pack(&Packer))
			return -1;
		return SendMsg(&Packer, Flags);
	}

	virtual const char *ErrorString() = 0;
	virtual bool ConnectionProblems() = 0;
};

class IGameBot: public IInterface
{
	MACRO_INTERFACE("gamebot", 0)

protected:
public:
	virtual void OnConsoleInit() = 0;

	virtual void OnInit() = 0;
	virtual void OnConnected() = 0;
	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker) = 0;

	virtual const char *Version() = 0;
	virtual const char *NetVersion() = 0;
};

IGameBot *CreateGameBot();

#endif /* ENGINE_BOT_H_ */
