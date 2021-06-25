// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/ProcessWatcherSocket.hh>

#include <Windows.h>

#include <cstdio>
#include <iostream>

using namespace ktmac;

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

    {
        ProcessWatcherSocket socket { ProcessWatcherSocket::MakeServerSocket(portNumber, handler) };
        std::this_thread::sleep_for(std::chrono::seconds { 10 });
    }

    ProcessWatcherSocket::UninitializeWinSock();
}