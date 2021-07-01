// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/ProcessWatcherSocket.hh>

#include <Windows.h>

#include <cstdint>
#include <cstdio>
#include <iostream>

using namespace ktmac;

int main(int argc, char* argv[])
{
    int portNumber = 23654;

    auto handler = [](ProcessWatcherMessage message, uint32_t processId) {
        if (message == ProcessWatcherMessage::Running)
            std::cout << "Running(" << processId << ")..." << std::endl;
        else
            std::cout << "Stopped(" << processId << ")..." << std::endl;
    };

    ProcessWatcherSocket socket { ProcessWatcherSocket::MakeServerSocket(portNumber, handler) };
    std::this_thread::sleep_for(std::chrono::seconds { 10 });
}