// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_SOCKET_HH
#define KTMAC_SOCKET_HH

#include <cstdint>
#include <functional>
#include <thread>

namespace ktmac
{

enum class ProcessWatcherMessage : uint8_t
{
    Running,
    Stopped,
    Quit
};

using ProcessWatcherSocketHandler = std::function<void(ProcessWatcherMessage message)>;

class ProcessWatcherSocket
{
  private:
    enum class SocketType
    {
        Invalid,
        Server,
        Client
    };

  public:
    static bool                 InitializeWinSock();
    static void                 UninitializeWinSock();
    static ProcessWatcherSocket MakeServerSocket(int                           portNumber,
                                                 ProcessWatcherSocketHandler&& handler);
    static ProcessWatcherSocket MakeClientSocket(int portNumber);

  private:
    void*                       _client;
    void*                       _socket;
    SocketType                  _socketType;
    ProcessWatcherSocketHandler _handler;
    std::thread                 _recvThread;

  public:
    ~ProcessWatcherSocket();

  private:
    ProcessWatcherSocket(void*                         socket,
                         SocketType                    socketType,
                         void*                         client  = nullptr,
                         ProcessWatcherSocketHandler&& handler = nullptr);

  public:
    void WaitUntilQuit();
    void Send(ProcessWatcherMessage message);

  private:
    void HandleIncomingData();
};

}

#endif