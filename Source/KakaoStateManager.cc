// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/KakaoStateManager.hh>
#include <ktmac/ProcessWatcherSocket.hh>
#include <ktmac/WindowHook.hh>

#include <Windows.h>

#include <TlHelp32.h>

#include <algorithm>
#include <cstring>
#include <unordered_set>
#include <vector>

using namespace std::placeholders;

namespace
{

std::unordered_set<uint32_t> GetKakaoTalkProcessIdList()
{
    PROCESSENTRY32 entry = {};
    entry.dwSize         = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

    std::unordered_set<uint32_t> rtn;
    if (Process32First(snapshot, &entry) == TRUE)
    {
        while (Process32Next(snapshot, &entry) == TRUE)
        {
            if (_stricmp(entry.szExeFile, "KakaoTalk.exe") == 0)
                rtn.insert(entry.th32ProcessID);
        }
    }

    CloseHandle(snapshot);
    return rtn;
}

HWND FindKakaoTalkChatroomWindow()
{
    HWND window = NULL;
    while ((window = FindWindowEx(NULL, window, "#32770", NULL)) != NULL)
    {
        if (FindWindowEx(window, NULL, "RICHEDIT50W", NULL) != NULL)
            return window;
    }

    return NULL;
}

struct MainWindow
{
    HWND mainWindow;
    HWND online;
    HWND contactList;
    HWND chatroomList;
    HWND misc;
    HWND lock;
};

bool FindKakaoTalkMainWindow(MainWindow& rtn, HWND mainWindow)
{
    HWND lockWindow = FindWindowEx(mainWindow, NULL, "EVA_ChildWindow_Dblclk", NULL);
    if (lockWindow != NULL)
    {
        HWND childWindow = FindWindowEx(mainWindow, NULL, "EVA_ChildWindow", NULL);
        if (childWindow != NULL)
        {
            rtn.mainWindow = mainWindow;
            rtn.online     = childWindow;
            rtn.lock       = lockWindow;

            HWND childView = NULL;
            while ((childView = FindWindowEx(childWindow, childView, "EVA_Window", NULL)) != NULL)
            {
                char className[256];
                GetWindowText(childView, className, sizeof className);

                if (strncmp(className, "ContactListView", sizeof "ContactListView" - 1) == 0)
                    rtn.contactList = childView;
                else if (strncmp(className, "ChatRoomListView", sizeof "ChatRoomListView" - 1) == 0)
                    rtn.chatroomList = childView;
                else if (strncmp(className, "MoreView", sizeof "MoreView" - 1) == 0)
                    rtn.misc = childView;
            }

            if (rtn.contactList && rtn.chatroomList && rtn.misc)
                return true;
        }
    }

    return false;
}

bool FindKakaoTalkMainWindow(MainWindow& rtn)
{
    HWND mainWindow = NULL;
    while ((mainWindow = FindWindowEx(NULL, mainWindow, "EVA_Window_Dblclk", NULL)) != NULL)
    {
        if (FindKakaoTalkMainWindow(rtn, mainWindow))
            return true;
    }

    return false;
}

HWND FindKakaoTalkLoginWindow()
{
    HWND window = NULL;
    while ((window = FindWindowEx(NULL, window, "EVA_Window", NULL)) != NULL)
    {
        if (FindWindowEx(window, NULL, "Edit", NULL) != NULL)
            return window;
    }

    return NULL;
}

}

namespace ktmac
{

void KakaoStateManager::HandleWindowHook(void* context, HWND window, DWORD event)
{
    auto& manager = *(ktmac::KakaoStateManager*)context;
    manager.HandleWindowHook(window, event);
}

KakaoStateManager::KakaoStateManager(std::initializer_list<HandlerType> handlerList) :
    _handlerList(std::move(handlerList)),
    _messageThread {},
    _stateMtx {},
    _messageThreadId { NULL },
    _currentState { KakaoState::NotRunning },
    _currentProcessId { NULL },
    _processIdList(GetKakaoTalkProcessIdList()),
    _loginWindow { NULL },
    _mainWindow { NULL },
    _online { NULL },
    _contactList { NULL },
    _chatroomList { NULL },
    _misc { NULL },
    _lock { NULL },
    _chatroomWindow { NULL },
    _hookHandle { NULL },
    _watcherSocket {
        std::make_unique<ProcessWatcherSocket>(ProcessWatcherSocket::MakeServerSocket(
            23456,
            std::bind(&KakaoStateManager::HandleProcessHook, this, _1, _2))),
    }
{
    FindInitialState();
    CallHandlers();
    RunThread();
}

KakaoStateManager::~KakaoStateManager()
{
    Clean();
}

bool KakaoStateManager::SetMessage(std::wstring const& message)
{
    HWND chatroom = NULL;
    {
        std::lock_guard<std::mutex> mtx { _stateMtx };
        chatroom = _chatroomWindow;
    }
    if (chatroom == NULL)
        return false;

    HWND richEdit = FindWindowExW(chatroom, NULL, L"RICHEDIT50W", NULL);
    if (richEdit == NULL)
        return false;

    SetLastError(NOERROR);
    SendMessageW(richEdit, WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(message.c_str()));

    return GetLastError() == NOERROR;
}

bool KakaoStateManager::SetMessage(std::string const& message)
{
    HWND chatroom = NULL;
    {
        std::lock_guard<std::mutex> mtx { _stateMtx };
        chatroom = _chatroomWindow;
    }
    if (chatroom == NULL)
        return false;

    HWND richEdit = FindWindowExW(chatroom, NULL, L"RICHEDIT50W", NULL);
    if (richEdit == NULL)
        return false;

    SetLastError(NOERROR);
    SendMessageA(richEdit, WM_SETTEXT, NULL, reinterpret_cast<LPARAM>(message.c_str()));

    return GetLastError() == NOERROR;
}

#pragma push_macro("SendMessage")
#undef SendMessage
bool KakaoStateManager::SendMessage()
#pragma pop_macro("SendMessage")
{
    HWND chatroom = NULL;
    {
        std::lock_guard<std::mutex> mtx { _stateMtx };
        chatroom = _chatroomWindow;
    }
    if (chatroom == NULL)
        return false;

    HWND richEdit = FindWindowExW(chatroom, NULL, L"RICHEDIT50W", NULL);
    if (richEdit == NULL)
        return false;

    if (!PostMessage(richEdit, WM_KEYDOWN, VK_RETURN, NULL))
        return false;

    if (!PostMessage(richEdit, WM_KEYUP, VK_RETURN, NULL))
        return false;

    return true;
}

void KakaoStateManager::RunThread()
{
    _messageThread   = std::thread { &KakaoStateManager::HandleMessageLoop, this };
    _messageThreadId = GetThreadId(_messageThread.native_handle());
}

void KakaoStateManager::HandleMessageLoop()
{
    if (_currentProcessId)
        _hookHandle = HookStart(_currentProcessId, HandleWindowHook, this);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (_hookHandle)
        HookStop(_hookHandle);
    _hookHandle = NULL;
}

void KakaoStateManager::Clean(bool clearHandlerList, bool clearProcessIdList)
{
    PostThreadMessage(_messageThreadId, WM_QUIT, NULL, NULL);
    _messageThread.join();

    if (clearHandlerList)
        _handlerList.clear();

    _messageThreadId  = NULL;
    _currentState     = KakaoState::NotRunning;
    _currentProcessId = NULL;

    if (clearProcessIdList)
        _processIdList.clear();

    _loginWindow    = NULL;
    _mainWindow     = NULL;
    _online         = NULL;
    _contactList    = NULL;
    _chatroomList   = NULL;
    _misc           = NULL;
    _lock           = NULL;
    _chatroomWindow = NULL;
    _hookHandle     = NULL;
}

void KakaoStateManager::FindInitialState()
{
    using namespace std::chrono_literals;

    std::lock_guard guard { _stateMtx };
    if (_processIdList.empty())
    {
        _currentState = KakaoState::NotRunning;
        return;
    }

    while (true)
    {
        _loginWindow = FindKakaoTalkLoginWindow();
        if (MainWindow mainWindow; !FindKakaoTalkMainWindow(mainWindow))
        {
            _currentState = KakaoState::NotRunning;
            return;
        }
        else
        {
            _mainWindow   = mainWindow.mainWindow;
            _online       = mainWindow.online;
            _lock         = mainWindow.lock;
            _contactList  = mainWindow.contactList;
            _chatroomList = mainWindow.chatroomList;
            _misc         = mainWindow.misc;
        }
        _chatroomWindow = FindKakaoTalkChatroomWindow();

        if (_mainWindow != NULL)
        {
            DWORD processId = NULL;
            GetWindowThreadProcessId(_mainWindow, &processId);

            _processIdList.insert(processId);
            _currentProcessId = processId;
            break;
        }

        std::this_thread::sleep_for(0.1s);
    }

    if (_loginWindow != NULL)
    {
        _currentState = KakaoState::LoggedOut;
        return;
    }

    if (_chatroomWindow != NULL)
    {
        _currentState = KakaoState::ChatroomIsVisible;
        return;
    }

    // _mainWindow is not null here
    if (!IsWindowVisible(_mainWindow))
    {
        _currentState = KakaoState::Background;
        return;
    }

    if (IsWindowVisible(_lock))
        _currentState = KakaoState::Locked;
    else if (IsWindowVisible(_contactList))
        _currentState = KakaoState::ContactListIsVisible;
    else if (IsWindowVisible(_chatroomList))
        _currentState = KakaoState::ChatroomListIsVisible;
    else
        _currentState = KakaoState::MiscIsVisible;
}

void KakaoStateManager::HandleProcessHook(ProcessWatcherMessage message, uint32_t processId)
{
    using namespace std::chrono_literals;

    if (message == ProcessWatcherMessage::Running)
    {
        bool initialize = false;
        {
            std::lock_guard guard { _stateMtx };
            if (_processIdList.empty())
            {
                _currentProcessId = processId;
                initialize        = true;
            }
            _processIdList.insert(processId);
        }

        if (initialize)
        {
            FindInitialState();
            CallHandlers();
            RunThread();
        }
    }
    else if (message == ProcessWatcherMessage::Stopped)
    {
        std::lock_guard guard { _stateMtx };

        auto it = _processIdList.find(processId);
        if (it != _processIdList.end())
            _processIdList.erase(it);

        if (_processIdList.empty())
        {
            _currentProcessId = NULL;
            Clean(false, false);
            _currentState = KakaoState::NotRunning;
            CallHandlers();
        }
    }
}

void KakaoStateManager::HandleWindowHook(HWND window, DWORD event)
{
    if (_currentState != KakaoState::NotRunning)
    {
        KakaoState newState          = _currentState;
        HWND       newLoginWindow    = _loginWindow;
        HWND       newMainWindow     = _mainWindow;
        HWND       newOnline         = _online;
        HWND       newContactList    = _contactList;
        HWND       newChatroomList   = _chatroomList;
        HWND       newMisc           = _misc;
        HWND       newLock           = _lock;
        HWND       newChatroomWindow = _chatroomWindow;
        bool       stateChanged      = false;

        auto getMainWindowState = [&](bool preferOnline = false) {
            if (!preferOnline && IsWindowVisible(_lock))
                newState = KakaoState::Locked;
            else if (IsWindowVisible(_contactList))
                newState = KakaoState::ContactListIsVisible;
            else if (IsWindowVisible(_chatroomList))
                newState = KakaoState::ChatroomListIsVisible;
            else
                newState = KakaoState::MiscIsVisible;
        };

        switch (event)
        {
        case EVENT_OBJECT_CREATE:
        {
            char className[128];
            if (!GetClassName(window, className, sizeof className))
                break;

            switch (_currentState)
            {
            case KakaoState::Background:
            {
                if (strcmp(className, "#32770") == 0)
                {
                    newChatroomWindow = window;
                    newState          = KakaoState::ChatroomIsVisible;
                    stateChanged      = true;
                }
                else if (strcmp(className, "EVA_Window") == 0
                         && FindWindowEx(window, NULL, "Edit", NULL) != NULL)
                {
                    newLoginWindow = window;
                    newState       = KakaoState::LoggedOut;
                    stateChanged   = true;
                }
                break;
            }
            case KakaoState::ContactListIsVisible:
            case KakaoState::ChatroomListIsVisible:
            case KakaoState::MiscIsVisible:
            {
                if (strcmp(className, "#32770") == 0)
                {
                    newChatroomWindow = window;
                    newState          = KakaoState::ChatroomIsVisible;
                    stateChanged      = true;
                }
                break;
            }
            }
            break;
        }
        case EVENT_OBJECT_DESTROY:
        {
            switch (_currentState)
            {
            case KakaoState::ChatroomIsVisible:
            {
                if (window == _chatroomWindow)
                {
                    newChatroomWindow = NULL;

                    if (IsWindowVisible(_mainWindow))
                        getMainWindowState();
                    else
                        newState = KakaoState::Background;

                    stateChanged = true;
                }
                break;
            }
            }
            break;
        }
        case EVENT_OBJECT_SHOW:
        {
            switch (_currentState)
            {
            case KakaoState::LoggedOut:
            case KakaoState::Background:
            {
                if (window == _mainWindow)
                {
                    getMainWindowState();
                    stateChanged = true;
                }
                break;
            }
            case KakaoState::Locked:
            {
                if (window == _online)
                {
                    getMainWindowState(true);
                    stateChanged = true;
                }
                break;
            }
            case KakaoState::ContactListIsVisible:
            case KakaoState::ChatroomListIsVisible:
            case KakaoState::MiscIsVisible:
            {
                if (_chatroomWindow != NULL && IsWindowVisible(_chatroomWindow))
                {
                    newState     = KakaoState::ChatroomIsVisible;
                    stateChanged = true;
                }
                else if (window == _lock || window == _contactList || window == _chatroomList
                         || window == _misc)
                {
                    getMainWindowState();
                    stateChanged = true;
                }
                break;
            }
            }
            break;
        }
        case EVENT_OBJECT_HIDE:
        {
            switch (_currentState)
            {
            case KakaoState::Locked:
            case KakaoState::ContactListIsVisible:
            case KakaoState::ChatroomListIsVisible:
            case KakaoState::MiscIsVisible:
            {
                if (window == _mainWindow)
                {
                    newState     = KakaoState::Background;
                    stateChanged = true;
                }
                break;
            }
            case KakaoState::ChatroomIsVisible:
            {
                if (window == _chatroomWindow)
                {
                    if (IsWindowVisible(_mainWindow))
                    {
                        getMainWindowState();
                        stateChanged = true;
                    }
                    else
                    {
                        newState     = KakaoState::Background;
                        stateChanged = true;
                    }
                }
                break;
            }
            }
            break;
        }
        }

        if (stateChanged)
        {
            std::lock_guard<std::mutex> guard { _stateMtx };

            _currentState   = newState;
            _loginWindow    = newLoginWindow;
            _mainWindow     = newMainWindow;
            _online         = newOnline;
            _contactList    = newContactList;
            _chatroomList   = newChatroomList;
            _misc           = newMisc;
            _lock           = newLock;
            _chatroomWindow = newChatroomWindow;

            CallHandlers();
        }
    }
}

}