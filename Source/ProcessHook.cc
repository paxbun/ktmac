// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/ProcessWatcher.hh>
#include <ktmac/ProcessWatcherSocket.hh>

#include <Windows.h>

#include <charconv>
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
        [&socket](ProcessState state) {
            if (state == ProcessState::Running)
                socket.Send(ProcessWatcherMessage::Running);
            else
                socket.Send(ProcessWatcherMessage::Stopped);
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

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR commandLine, int)
{
    if (!ProcessWatcher::CheckAdministratorPrivilege())
        return 1;

    int portNumber = 0;
    if (std::from_chars(commandLine, commandLine + strlen(commandLine), portNumber).ec
        != std::errc {})
        return 1;

    if (!ProcessWatcher::InitializeCom())
        return 1;

    int rtn = Run(portNumber);
    ProcessWatcher::UninitializeCom();

    return rtn;
}
