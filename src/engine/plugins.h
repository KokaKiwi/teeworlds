/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_PLUGINS_H
#define ENGINE_PLUGINS_H

#include "kernel.h"

#define PLUGIN_EXPORT extern "C"

#define PLUGIN_INIT_SIGNATURE void (IKernel *, IPlugins *, IPlugin *)
#define PLUGIN_DESTROY_SIGNATURE void (IPlugins *, IPlugin *)

class IPlugin
{
public:
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
