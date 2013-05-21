/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <dlfcn.h>

#include <base/system.h>
#include <base/tl/array.h>

#include <engine/plugins.h>
#include <engine/console.h>

#include <engine/shared/config.h>

#include "plugins.h"

CPlugin::CPlugin(const char *pPath)
{
	m_pError = 0;
	m_pHandle = dlopen(pPath, RTLD_LAZY);

	if (!m_pHandle)
		m_pError = dlerror();
}

CPlugin::~CPlugin()
{
	if (m_pHandle)
		dlclose(m_pHandle);
}

void *CPlugin::GetSymbol(const char *pSymName)
{
	void *pSymbol = dlsym(m_pHandle, pSymName);

	if (!pSymbol)
		m_pError = dlerror();

	return pSymbol;
}

bool CPlugin::Error()
{
	return m_pError != 0;
}

const char *CPlugin::ErrorString()
{
	return m_pError;
}

CPlugins::CPlugins(int Type)
{
	m_pType = Type;
}

IPlugin *CPlugins::LoadPlugin(const char *pPath)
{
	CPlugin *pPlugin = new CPlugin(pPath);

	if (pPlugin->Error())
	{
		dbg_msg("plugins", "Plugin load error: %s", pPlugin->ErrorString());
		delete pPlugin;
		return 0;
	}

	if (pPlugin->Get<CPluginInfo>(PLUGIN_INFO_NAME))
		mem_copy(&pPlugin->m_PluginInfo, pPlugin->Get<CPluginInfo>(PLUGIN_INFO_NAME), sizeof(CPluginInfo));
	else if(pPlugin->Error())
	{
		dbg_msg("plugins", "%s", pPlugin->ErrorString());
		delete pPlugin;
		return 0;
	}

	if (!(pPlugin->m_PluginInfo.m_Type & m_pType))
	{
		dbg_msg("plugins", "error: mismatching plugin type: %s", pPath);
		delete pPlugin;
		return 0;
	}

	if (pPlugin->Get<PLUGIN_INIT_SIGNATURE>(PLUGIN_INIT_NAME))
		pPlugin->Get<PLUGIN_INIT_SIGNATURE>(PLUGIN_INIT_NAME)(Kernel(), pPlugin);
	else if(pPlugin->Error())
	{
		dbg_msg("plugins", "%s", pPlugin->ErrorString());
		delete pPlugin;
		return 0;
	}

	dbg_msg("plugins", "Loaded plugin: %s v%s by %s",
	        pPlugin->m_PluginInfo.m_Name,
	        pPlugin->m_PluginInfo.m_Version,
	        pPlugin->m_PluginInfo.m_Author);

	m_pPlugins.add(pPlugin);

	return pPlugin;
}

void CPlugins::UnloadPlugin(IPlugin *pPlugin)
{
	if (pPlugin->Get<PLUGIN_DESTROY_SIGNATURE>(PLUGIN_DESTROY_NAME))
		pPlugin->Get<PLUGIN_DESTROY_SIGNATURE>(PLUGIN_DESTROY_NAME)(Kernel(), pPlugin);
	else if(pPlugin->Error())
		dbg_msg("plugins", "%s", pPlugin->ErrorString());

	delete pPlugin;
}

void CPlugins::UnloadPlugins()
{
	while (m_pPlugins.size() > 0)
	{
		CPlugin *pPlugin = m_pPlugins[0];

		m_pPlugins.remove_index(0);
		UnloadPlugin(pPlugin);
	}
}

void CPlugins::Con_LoadPlugin(IConsole::IResult *pResult, void *pUserData)
{
	IPlugins *pSelf = (IPlugins *)pUserData;
	pSelf->LoadPlugin(pResult->GetString(0));
}

void CPlugins::Init()
{
	IConsole *pConsole = Kernel()->RequestInterface<IConsole>();

	pConsole->Register("loadplugin", "s", CFGFLAG_CLIENT|CFGFLAG_SERVER, Con_LoadPlugin, this, "Load plugin");
}

IPlugins *CreatePlugins(int Type)
{
	return new CPlugins(Type);
}
