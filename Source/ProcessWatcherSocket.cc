// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/ProcessWatcherSocket.hh>

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>

#include <cstdio>
#include <stdexcept>

namespace ktmac
{

bool ProcessWatcherSocket::InitializeWinSock()
{
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}

void ProcessWatcherSocket::UninitializeWinSock()
{
    WSACleanup();
}

ProcessWatcherSocket ProcessWatcherSocket::MakeServerSocket(int portNumber,
                                                            ProcessWatcherSocketHandler&& handler)
{
    addrinfo hints    = {};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;

    char portNumberStr[12];
    sprintf_s(portNumberStr, sizeof portNumberStr - 1, "%d", portNumber);

    addrinfo* result = nullptr;
    if (getaddrinfo(NULL, portNumberStr, &hints, &result) != 0)
        throw std::runtime_error { "Failed to getaddrinfo()." };

    SOCKET listenSocket = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listenSocket == INVALID_SOCKET)
    {
        freeaddrinfo(result);
        throw std::runtime_error { "Failed to create a listen socket." };
    }

    if (bind(listenSocket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR)
    {
        freeaddrinfo(result);
        closesocket(listenSocket);
        throw std::runtime_error { "Failed to bind the listen socket." };
    }

    freeaddrinfo(result);

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        closesocket(listenSocket);
        throw std::runtime_error { "Failed to listen()." };
    }

    SOCKET socket = accept(listenSocket, NULL, NULL);
    if (socket == INVALID_SOCKET)
    {
        closesocket(listenSocket);
        throw std::runtime_error { "Failed to accept()." };
    }

    closesocket(listenSocket);

    return ProcessWatcherSocket { socket, SocketType::Server, std::move(handler) };
}

ProcessWatcherSocket ProcessWatcherSocket::MakeClientSocket(int portNumber)
{
    addrinfo hints    = {};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    char portNumberStr[12];
    sprintf_s(portNumberStr, sizeof portNumberStr - 1, "%d", portNumber);

    addrinfo* result = nullptr;
    if (getaddrinfo("127.0.0.1", portNumberStr, &hints, &result) != 0)
        throw std::runtime_error { "Failed to getaddrinfo()." };

    SOCKET socket = INVALID_SOCKET;
    for (addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next)
    {
        socket = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (socket == INVALID_SOCKET)
            throw std::runtime_error { "Failed to create a socket." };

        if (connect(socket, ptr->ai_addr, (int)ptr->ai_addrlen) != SOCKET_ERROR)
            break;

        socket = INVALID_SOCKET;
    }

    freeaddrinfo(result);

    if (socket == INVALID_SOCKET)
        throw std::runtime_error { "Failed to create a socket." };

    return ProcessWatcherSocket { socket, SocketType::Client };
}

ProcessWatcherSocket::ProcessWatcherSocket(SOCKET                        socket,
                                           SocketType                    socketType,
                                           ProcessWatcherSocketHandler&& handler) :
    _socket { socket },
    _socketType { socketType },
    _handler { std::move(handler) },
    _recvThread { &ProcessWatcherSocket::HandleIncomingData, this }
{}

ProcessWatcherSocket::~ProcessWatcherSocket()
{
    if (_socket != INVALID_SOCKET)
    {
        if (_socketType == SocketType::Server)
        {
            auto message = ProcessWatcherMessage::Quit;
            send(_socket, (const char*)&message, 1, 0);
        }
        _recvThread.join();

        shutdown(_socket, SD_SEND);
        closesocket(_socket);
        _socket     = INVALID_SOCKET;
        _socketType = SocketType::Invalid;
    }
}

void ProcessWatcherSocket::Send(ProcessWatcherMessage message)
{
    if (_socketType == SocketType::Client)
    {
        if (message == ProcessWatcherMessage::Running || message == ProcessWatcherMessage::Stopped)
            send(_socket, (const char*)&message, 1, 0);
    }
}

void ProcessWatcherSocket::HandleIncomingData()
{
    ProcessWatcherMessage message = ProcessWatcherMessage::Quit;

    if (_socketType == SocketType::Invalid)
        return;

    while (true)
    {
        int result = recv(_socket, (char*)&message, 1, 0);
        if (result <= 0)
            return;

        if (_socketType == SocketType::Server)
        {
            if (_handler)
            {
                if (message == ProcessWatcherMessage::Running
                    || message == ProcessWatcherMessage::Stopped)
                    _handler(message);
            }
        }
        else
        {
            if (message == ProcessWatcherMessage::Quit)
                return;
        }
    }
}

}