/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <dlfcn.h>

#include <base/tl/array.h>

#include <engine/plugins.h>
#include <engine/console.h>

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

IPlugin *CPlugins::LoadPlugin(const char *pPath)
{
	CPlugin *pPlugin = new CPlugin(pPath);

	if (pPlugin->Error())
	{
		dbg_msg("plugins", "Plugin load error: %s", pPlugin->ErrorString());
		delete pPlugin;
		return 0;
	}

	if (pPlugin->Get<PLUGIN_INIT_SIGNATURE>("PluginInit"))
		pPlugin->Get<PLUGIN_INIT_SIGNATURE>("PluginInit")(Kernel(), this, pPlugin);
	else if(pPlugin->Error())
		dbg_msg("plugins", "Plugin init error: %s", pPlugin->ErrorString());

	m_pPlugins.add(pPlugin);

	return pPlugin;
}

void CPlugins::UnloadPlugin(IPlugin *pPlugin)
{
	if (pPlugin->Get<PLUGIN_DESTROY_SIGNATURE>("PluginDestroy"))
		pPlugin->Get<PLUGIN_DESTROY_SIGNATURE>("PluginDestroy")(this, pPlugin);

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

IPlugins *CreatePlugins()
{
	return new CPlugins();
}

void CPlugins::Init()
{

}
