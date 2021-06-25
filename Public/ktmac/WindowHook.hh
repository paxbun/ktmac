// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef HOOK_HH
#define HOOK_HH

#ifdef HOOK_EXPORT
#    define HOOK_PUBLIC __declspec(dllexport)
#else
#    define HOOK_PUBLIC __declspec(dllimport)
#endif

#include <Windows.h>

namespace ktmac
{

using HookEventContext = void*;

using HookEventHandler = void (*)(HookEventContext context, HWND window, DWORD event);

HOOK_PUBLIC HWINEVENTHOOK HookStart(DWORD            processId,
                                    HookEventHandler handler,
                                    HookEventContext context);

HOOK_PUBLIC void HookStop(HWINEVENTHOOK hook);

}

#endif