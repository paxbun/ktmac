// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_KAKAO_STATE_MANAGER_HH
#define KTMAC_KAKAO_STATE_MANAGER_HH

#include <ktmac/ProcessWatcherSocket.hh>

#include <Windows.h>

#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

namespace ktmac
{

enum class KakaoState
{
    NotRunning,
    LoggedOut,
    Background,
    Locked,
    ContactListIsVisible,
    ChatroomListIsVisible,
    MiscIsVisible,
    ChatroomIsVisible,
};

class KakaoStateManager
{
  public:
    using HandlerType = std::function<void(KakaoState state)>;

  private:
    std::vector<HandlerType> _handlerList;

    std::thread _messageThread;
    std::mutex  _stateMtx;
    DWORD       _messageThreadId;
    KakaoState  _currentState;

    size_t                       _numProcesses;
    uint32_t                     _currentProcessId;
    std::unordered_set<uint32_t> _processIdList;

    HWND _loginWindow;
    HWND _mainWindow, _online, _contactList, _chatroomList, _misc, _lock;
    HWND _chatroomWindow;

    HWINEVENTHOOK _hookHandle;

    ProcessWatcherSocket _watcherSocket;

  public:
    KakaoState GetCurrentState()
    {
        return _currentState;
    }

  public:
    inline KakaoStateManager() : KakaoStateManager(std::initializer_list<HandlerType> {}) {}
    KakaoStateManager(std::initializer_list<HandlerType> init);
    ~KakaoStateManager();

  public:
    inline void AddHandler(HandlerType&& newHandler)
    {
        _handlerList.push_back(std::move(newHandler));
    }

    inline void operator+=(HandlerType&& newHandler)
    {
        AddHandler(std::move(newHandler));
    }

    bool SetMessage(std::wstring const& message);

    bool SetMessage(std::string const& message);

#pragma push_macro("SendMessage")
#undef SendMessage
    bool SendMessage();
#pragma pop_macro("SendMessage")

  private:
    inline void CallHandlers()
    {
        for (auto&& handler : _handlerList)
        {
            if (handler)
                handler(_currentState);
        }
    }

    void        RunThread();
    void        HandleMessageLoop();
    void        Clean(bool clearHandlerList = true, bool clearProcessIdList = true);
    void        FindInitialState();
    void        HandleProcessHook(ProcessWatcherMessage message, uint32_t processId);
    void        HandleWindowHook(HWND window, DWORD event);
    friend void HandleWindowHook(void* context, HWND window, DWORD event);
};

}

#endif