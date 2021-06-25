// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/WindowHook.hh>

#include <TlHelp32.h>

#include <mutex>
#include <unordered_map>

using namespace ktmac;

struct HookListEntry
{
    HWINEVENTHOOK    hook;
    HookEventHandler handler;
    HookEventContext context;
};

namespace
{

HINSTANCE _instance = NULL;

std::unordered_map<HWINEVENTHOOK, HookListEntry> _hookList;
std::mutex                                       _hookListMtx;

void CALLBACK WinEventProc(HWINEVENTHOOK hookHandle,
                           DWORD         event,
                           HWND          window,
                           LONG          objectId,
                           LONG          childId,
                           DWORD         eventThread,
                           DWORD         eventTimeMs)
{
    if (objectId == OBJID_WINDOW && childId == CHILDID_SELF)
    {
        HookEventHandler handler = nullptr;
        HookEventContext context = nullptr;
        {
            std::lock_guard<std::mutex> guard { _hookListMtx };
            if (auto it = _hookList.find(hookHandle); it != _hookList.end())
            {
                handler = it->second.handler;
                context = it->second.context;
            }
        }

        if (handler)
            handler(context, window, event);
    }
}

}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH: _instance = instance; break;
    }
    return TRUE;
}

HOOK_PUBLIC HWINEVENTHOOK ktmac::HookStart(DWORD            processId,
                                           HookEventHandler handler,
                                           HookEventContext context)
{
    if (processId == NULL)
        return NULL;

    HWINEVENTHOOK rtn = SetWinEventHook(EVENT_OBJECT_CREATE,
                                        EVENT_OBJECT_HIDE,
                                        _instance,
                                        WinEventProc,
                                        processId,
                                        NULL,
                                        WINEVENT_OUTOFCONTEXT);
    if (rtn == NULL)
        return NULL;

    {
        std::lock_guard<std::mutex> guard { _hookListMtx };
        _hookList.insert(std::make_pair(rtn, HookListEntry { rtn, handler, context }));
    }

    return rtn;
}

HOOK_PUBLIC void ktmac::HookStop(HWINEVENTHOOK hook)
{
    std::lock_guard<std::mutex> guard { _hookListMtx };

    if (auto it = _hookList.find(hook); it != _hookList.end())
    {
        UnhookWinEvent(hook);
        _hookList.erase(it);
    }
}
