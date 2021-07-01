// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_KAKAO_STATE_MANAGER_HH
#define KTMAC_KAKAO_STATE_MANAGER_HH

#include <ktmac/ProcessWatcherMessage.hh>

#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <utility>
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

#ifdef KTMAC_CORE_SHARED
#    ifdef KTMAC_CORE_EXPORT
class __declspec(dllexport) KakaoStateManager
#    else
class __declspec(dllimport) KakaoStateManager
#    endif
#else
class KakaoStateManager
#endif
{
  private:
    struct Impl;

  public:
    using HandlerContextType = void*;
    using HandlerType        = void (*)(HandlerContextType context, KakaoState state);
    using HandlerPairType    = std::pair<HandlerContextType, HandlerType>;

  private:
    Impl* _impl;

  public:
    KakaoState GetCurrentState();

  public:
    inline KakaoStateManager() : KakaoStateManager(std::initializer_list<HandlerPairType> {}) {}
    inline KakaoStateManager(KakaoStateManager&& manager) noexcept : _impl { manager._impl }
    {
        manager._impl = nullptr;
    }

    KakaoStateManager& operator=(KakaoStateManager&& manager) noexcept;
    KakaoStateManager(std::initializer_list<HandlerPairType> init);
    ~KakaoStateManager();

  public:
    void AddHandler(HandlerPairType newHandler);

    inline void AddHandler(HandlerContextType context, HandlerType handler)
    {
        AddHandler(std::make_pair(context, handler));
    }

    inline void operator+=(HandlerPairType newHandler)
    {
        AddHandler(newHandler);
    }

    bool SetMessage(wchar_t const* message);

    inline bool SetMessage(std::wstring const& message)
    {
        return SetMessage(message.c_str());
    }

    bool SetMessage(char const* message);

    inline bool SetMessage(std::string const& message)
    {
        return SetMessage(message.c_str());
    }

#pragma push_macro("SendMessage")
#undef SendMessage
    bool SendMessage();
#pragma pop_macro("SendMessage")
};

}

#endif