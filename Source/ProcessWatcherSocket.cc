// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/ProcessWatcherSocket.hh>

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>

#include <cstdio>
#include <stdexcept>

namespace
{

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

}

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
    if (!InitializeWinSock())
        throw std::runtime_error { "Windows Socket initialization failed." };

    HANDLE client = RunClient(portNumber);
    if (client == NULL)
        throw std::runtime_error { "Failed to launch client." };

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

    return ProcessWatcherSocket { (void*)socket, SocketType::Server, client, std::move(handler) };
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

    return ProcessWatcherSocket { (void*)socket, SocketType::Client };
}

ProcessWatcherSocket::ProcessWatcherSocket(void*                         socket,
                                           SocketType                    socketType,
                                           void*                         client,
                                           ProcessWatcherSocketHandler&& handler) :
    _client { client },
    _socket { socket },
    _socketType { socketType },
    _handler { std::move(handler) },
    _recvThread { &ProcessWatcherSocket::HandleIncomingData, this }
{}

ProcessWatcherSocket::ProcessWatcherSocket(ProcessWatcherSocket&& socket) noexcept :
    _client { socket._client },
    _socket { socket._socket },
    _socketType { socket._socketType },
    _handler { std::move(socket._handler) },
    _recvThread { std::move(socket._recvThread) }
{
    socket._client     = nullptr;
    socket._socket     = nullptr;
    socket._socketType = SocketType::Invalid;
    socket._handler    = nullptr;
}

ProcessWatcherSocket::~ProcessWatcherSocket()
{
    SOCKET socket = (SOCKET)_socket;
    if (socket != INVALID_SOCKET)
    {
        if (_socketType == SocketType::Server)
        {
            auto message = ProcessWatcherMessage::Quit;
            send(socket, (const char*)&message, 1, 0);
        }

        if (_recvThread.joinable())
            _recvThread.join();

        shutdown(socket, SD_SEND);
        closesocket(socket);

        if (_socketType == SocketType::Server)
        {
            WaitForSingleObject(_client, INFINITE);
            CloseHandle(_client);
        }

        _client     = nullptr;
        _socket     = (void*)INVALID_SOCKET;
        _socketType = SocketType::Invalid;
        _handler    = nullptr;
    }

    UninitializeWinSock();
}

void ProcessWatcherSocket::WaitUntilQuit()
{
    if (_socketType == SocketType::Client)
        _recvThread.join();
}

void ProcessWatcherSocket::Send(ProcessWatcherMessage message, uint32_t processId)
{
    if (_socketType == SocketType::Client)
    {
        if (message == ProcessWatcherMessage::Running || message == ProcessWatcherMessage::Stopped)
        {
            send((SOCKET)_socket, (const char*)&message, sizeof message, 0);
            send((SOCKET)_socket, (const char*)&processId, sizeof processId, 0);
        }
    }
}

void ProcessWatcherSocket::HandleIncomingData()
{
    ProcessWatcherMessage message   = ProcessWatcherMessage::Quit;
    uint32_t              processId = 0;

    if (_socketType == SocketType::Invalid)
        return;

    while (true)
    {
        int result = recv((SOCKET)_socket, (char*)&message, sizeof message, 0);
        if (result <= 0)
            return;

        if (_socketType == SocketType::Server)
        {
            if (_handler)
            {
                auto pProcessId = (char*)&processId;
                int  lengthRead = 0;
                while (lengthRead < sizeof processId)
                {
                    int result = recv((SOCKET)_socket, (char*)&processId, sizeof processId, 0);
                    if (result <= 0)
                        return;
                    lengthRead += result;
                }

                if (message == ProcessWatcherMessage::Running
                    || message == ProcessWatcherMessage::Stopped)
                    _handler(message, processId);
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