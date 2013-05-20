/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_PLUGINS_H
#define ENGINE_PLUGINS_H

#include "kernel.h"

#define PLUGIN_EXPORT extern "C"

#define PLUGIN_INFO_NAME "PluginInfo"
#define PLUGIN_INIT_NAME "PluginInit"
#define PLUGIN_DESTROY_NAME "PluginDestroy"

#define PLUGIN_INIT_SIGNATURE void (IKernel *, IPlugin *)
#define PLUGIN_DESTROY_SIGNATURE void (IKernel *, IPlugin *)

#define PLUGIN_INFO(Name, Version, Author, Description)\
	CPluginInfo PluginInfo = {Name, Version, Author, Description};

struct CPluginInfo
{
	const char *m_Name;
	const char *m_Version;
	const char *m_Author;
	const char *m_Description;
};

class IPlugin
{
public:
	CPluginInfo m_PluginInfo;

	virtual ~IPlugin() {};

	virtual void *GetSymbol(const char *pSymName) = 0;

	template<typename T>
	T *Get(const char *pSymName)
	{
		return (T *) GetSymbol(pSymName);
	}

    virtual bool Error() = 0;
    virtual const char *ErrorString() = 0;
};

class IPlugins : public IInterface
{
	MACRO_INTERFACE("plugins", 0)
public:

	virtual IPlugin *LoadPlugin(const char *pPath) = 0;
	virtual void UnloadPlugin(IPlugin *pPlugin) = 0;
	virtual void UnloadPlugins() = 0;

	virtual void Init() = 0;
};

extern IPlugins *CreatePlugins();

#endif
