// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/ProcessWatcherSocket.hh>

#include <Windows.h>

#include <cstdio>
#include <iostream>

using namespace ktmac;

HANDLE RunClient(int portNumber)
{
    char argument[12];
    sprintf_s(argument, "%d", portNumber);

    SHELLEXECUTEINFO shellExecuteInfo = {};
    shellExecuteInfo.cbSize           = sizeof shellExecuteInfo;
    shellExecuteInfo.fMask            = SEE_MASK_NOCLOSEPROCESS;
    shellExecuteInfo.hwnd             = NULL;
    shellExecuteInfo.lpVerb           = "runas";
    shellExecuteInfo.lpFile           = "ktmac-process-hook.exe";
    shellExecuteInfo.lpParameters     = argument;
    shellExecuteInfo.lpDirectory      = NULL;
    shellExecuteInfo.nShow            = SW_SHOW;
    shellExecuteInfo.hInstApp         = NULL;

    if (!ShellExecuteEx(&shellExecuteInfo))
        return NULL;

    return shellExecuteInfo.hProcess;
}

int main(int argc, char* argv[])
{
    int portNumber = 23654;

    if (!ProcessWatcherSocket::InitializeWinSock())
        return 1;

    auto handler = [](ProcessWatcherMessage message) {
        if (message == ProcessWatcherMessage::Running)
            std::cout << "Running..." << std::endl;
        else
            std::cout << "Stopped..." << std::endl;
    };

    HANDLE client = RunClient(portNumber);
    {
        ProcessWatcherSocket socket { ProcessWatcherSocket::MakeServerSocket(portNumber, handler) };
        std::this_thread::sleep_for(std::chrono::seconds { 10 });
    }

    WaitForSingleObject(client, INFINITE);
    CloseHandle(client);

    ProcessWatcherSocket::UninitializeWinSock();
}