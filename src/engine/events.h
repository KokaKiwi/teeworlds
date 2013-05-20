#ifndef ENGINE_EVENTS_H
#define ENGINE_EVENTS_H

#include "kernel.h"

class IEvents : public IInterface
{
    MACRO_INTERFACE("events", 0)
public:

    typedef const char * EventType;

    typedef void (*FEventCallback)(const char *pEventType, void *pUserData, void *pEventData);

    virtual void Init() = 0;

    virtual void Register(EventType pEventType, FEventCallback pfnFunc, void *pUser) = 0;
    virtual void Unregister(EventType pEventType, FEventCallback pfnFunc) = 0;
    virtual void Emit(EventType pEventType, void *pEventData) = 0;
};

extern IEvents *CreateEvents();

#endif
