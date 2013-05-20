/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_PLUGINS_H
#define ENGINE_SHARED_PLUGINS_H

#include <base/tl/array.h>

#include <engine/plugins.h>

class CPlugin : public IPlugin
{
	void *m_pHandle;

    const char *m_pError;

public:
	CPlugin(const char *pPath);
	virtual ~CPlugin();

	virtual void *GetSymbol(const char *pSymName);

    virtual bool Error();
    virtual const char *ErrorString();
};

class CPlugins : public IPlugins
{
	array<CPlugin *> m_pPlugins;

public:

	virtual IPlugin *LoadPlugin(const char *pPath);
	virtual void UnloadPlugin(IPlugin *pPlugin);
	virtual void UnloadPlugins();

	virtual void Init();
};

#endif
