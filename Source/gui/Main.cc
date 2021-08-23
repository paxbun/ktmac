// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/gui/App.hh>

#include <include/cef_app.h>
#include <include/cef_command_line.h>
#include <include/cef_sandbox_win.h>

#include <Windows.h>

using namespace ktmac;

int WINAPI wWinMain(__in HINSTANCE hInstance, __in_opt HINSTANCE, __in LPTSTR, __in int nCmdShow)
{
    CefEnableHighDPISupport();

    CefScopedSandboxInfo scopedSandbox;
    void*                sandboxInfo { scopedSandbox.sandbox_info() };

    CefRefPtr<gui::App> app { new gui::App };

    CefMainArgs mainArgs { hInstance };
    if (int exitCode = CefExecuteProcess(mainArgs, app, sandboxInfo); exitCode >= 0)
        return exitCode;

    CefRefPtr<CefCommandLine> commandLine { CefCommandLine::CreateCommandLine() };
    commandLine->InitFromString(::GetCommandLineW());

    CefSettings settings;

    CefInitialize(mainArgs, settings, app, sandboxInfo);
    CefRunMessageLoop();
    CefShutdown();

    return 0;
}