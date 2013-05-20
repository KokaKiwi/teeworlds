#ifndef ENGINE_SHARED_EVENTS_H
#define ENGINE_SHARED_EVENTS_H

#include <base/tl/array.h>

#include <engine/events.h>

struct CEventListener
{
    IEvents::FEventCallback m_fnFunc;
    void *m_User;

    CEventListener(IEvents::FEventCallback pfnFunc, void *pUser): m_fnFunc(pfnFunc), m_User(pUser) {}
};

struct CEventListeners
{
    IEvents::EventType m_EventType;
    array<CEventListener *> m_Listeners;

    CEventListeners(IEvents::EventType pEventType): m_EventType(pEventType) {}
};

class CEvents : public IEvents
{
    array<CEventListeners *> m_pListeners;
protected:
    array<CEventListener *> *Find(EventType pEventType, bool pCreate);
public:

    virtual void Init();

    virtual void Register(EventType pEventType, FEventCallback pfnFunc, void *pUser);
    virtual void Unregister(EventType pEventType, FEventCallback pfnFunc);
    virtual void Emit(EventType pEventType, void *pEventData);
};

#endif
