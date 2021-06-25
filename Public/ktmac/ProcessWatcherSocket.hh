// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_SOCKET_HH
#define KTMAC_SOCKET_HH

#include <WinSock2.h>

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
    SOCKET                      _socket;
    SocketType                  _socketType;
    ProcessWatcherSocketHandler _handler;
    std::thread                 _recvThread;

  private:
    ProcessWatcherSocket(SOCKET                        socket,
                         SocketType                    socketType,
                         ProcessWatcherSocketHandler&& handler = nullptr);
    ~ProcessWatcherSocket();

  public:
    void Send(ProcessWatcherMessage message);

  private:
    void HandleIncomingData();
};

}

#endif