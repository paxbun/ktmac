// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/ProcessWatcher.hh>
#include <ktmac/ProcessWatcherSocket.hh>

#include <Windows.h>

#include <charconv>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

using namespace ktmac;

int Run(int portNumber)
try
{
    ProcessWatcherSocket socket { ProcessWatcherSocket::MakeClientSocket(portNumber) };

    ProcessWatcher watcher {
        "KakaoTalk.exe",
        [&socket](ProcessState state, uint32_t processId) {
            if (state == ProcessState::Running)
                socket.Send(ProcessWatcherMessage::Running, processId);
            else
                socket.Send(ProcessWatcherMessage::Stopped, processId);
        },
    };

    socket.WaitUntilQuit();

    return 0;
}
catch (std::exception const& ex)
{
    MessageBox(NULL, ex.what(), "ktmac-process-hook error", MB_OK | MB_ICONERROR);
    return 1;
}

int WINAPI WinMain(__in HINSTANCE, __in_opt HINSTANCE, __in LPSTR commandLine, __in int)
{
    if (!ProcessWatcher::CheckAdministratorPrivilege())
        return 1;

    int portNumber = 0;
    if (std::from_chars(commandLine, commandLine + strlen(commandLine), portNumber).ec
        != std::errc {})
        return 1;

    if (!ProcessWatcher::InitializeCom())
        return 1;

    if (!ProcessWatcherSocket::InitializeWinSock())
        return 1;

    int rtn = Run(portNumber);

    ProcessWatcher::UninitializeCom();
    ProcessWatcherSocket::UninitializeWinSock();

    return rtn;
}
