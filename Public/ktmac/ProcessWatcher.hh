// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_PROCESS_WATCHER_HH
#define KTMAC_PROCESS_WATCHER_HH

#include <ktmac/WmiEventSink.hh>

#include <WbemIdl.h>
#include <wrl.h>

#include <functional>

namespace ktmac
{

enum class ProcessState
{
    Running,
    Stopped,
};

using ProcessStateHandler = std::function<void(ProcessState state)>;

class ProcessWatcher
{
    template <typename I>
    using ComPtr = Microsoft::WRL::ComPtr<I>;

  public:
    static bool CheckAdministratorPrivilege();
    static bool InitializeCom();
    static void UninitializeCom();

  private:
    ComPtr<IWbemLocator>        _wbemLocator;
    ComPtr<IWbemServices>       _services;
    ComPtr<IUnsecuredApartment> _unsecuredApt;
    WmiEventSink *              _processStartDetector, *_processStopDetector;
    ComPtr<IWbemObjectSink>     _processStartSink, _processStopSink;
    std::string                 _processName;
    ProcessStateHandler         _stateHandler;

  public:
    ProcessWatcher(std::string&& processName, ProcessStateHandler&& stateHandler);
    ~ProcessWatcher();
};

}

#endif