/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>

#include <engine/events.h>

#include "events.h"

array<CEventListener *> *CEvents::Find(EventType pEventType, bool pCreate)
{
    for (int i = 0; i < m_pListeners.size(); i++)
    {
        CEventListeners *pListeners = m_pListeners[i];
        if (str_comp(pListeners->m_EventType, pEventType) == 0)
            return &(pListeners->m_Listeners);
    }

    if (pCreate)
    {
        CEventListeners *pListeners = new CEventListeners(pEventType);
        m_pListeners.add(pListeners);

        return &(pListeners->m_Listeners);
    }

    return 0;
}

void CEvents::Init()
{

}

void CEvents::Register(EventType pEventType, FEventCallback pfnFunc, void *pUser)
{
    array<CEventListener *> *pListeners = Find(pEventType, true);

    pListeners->add(new CEventListener(pfnFunc, pUser));
}

void CEvents::Unregister(EventType pEventType, FEventCallback pfnFunc)
{
    array<CEventListener *> *pListeners = Find(pEventType, false);

    if (pListeners)
    {
        for (int i = 0; i < pListeners->size();)
        {
            CEventListener *pListener = (*pListeners)[i];
            if (pListener->m_fnFunc == pfnFunc)
            {
                pListeners->remove_index(i);
                delete pListener;
            }
            else
                i++;
        }
    }
}

void CEvents::Emit(EventType pEventType, void *pEventData)
{
    array<CEventListener *> *pListeners = Find(pEventType, false);

    if (pListeners)
    {
        for (int i = 0; i < pListeners->size(); i++)
        {
            CEventListener *pListener = (*pListeners)[i];

            pListener->m_fnFunc(pEventType, pListener->m_User, pEventData);
        }
    }
}

IEvents *CreateEvents()
{
    return new CEvents();
}
