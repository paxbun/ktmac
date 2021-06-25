// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_WINDOW_HOOK_HH
#define KTMAC_WINDOW_HOOK_HH

#ifdef HOOK_EXPORT
#    define KTMAC_WINDOW_HOOK_PUBLIC __declspec(dllexport)
#else
#    define KTMAC_WINDOW_HOOK_PUBLIC __declspec(dllimport)
#endif

#include <Windows.h>

namespace ktmac
{

using HookEventContext = void*;

using HookEventHandler = void (*)(HookEventContext context, HWND window, DWORD event);

KTMAC_WINDOW_HOOK_PUBLIC HWINEVENTHOOK HookStart(DWORD            processId,
                                                 HookEventHandler handler,
                                                 HookEventContext context);

KTMAC_WINDOW_HOOK_PUBLIC void HookStop(HWINEVENTHOOK hook);

}

#endif