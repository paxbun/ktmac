// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/ProcessWatcher.hh>

#include <WbemIdl.h>
#include <Windows.h>
#include <comdef.h>
#include <wrl.h>

#include <stdexcept>

using Microsoft::WRL::ComPtr;

namespace
{

ComPtr<IWbemLocator> CreateWbemLocator()
{
    ComPtr<IWbemLocator> wbemLocator;
    if (FAILED(CoCreateInstance(
            CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&wbemLocator)))
        throw std::runtime_error { "Failed to create an IWbemLocator instance." };

    return wbemLocator;
}

ComPtr<IWbemServices> CreateWbemServices(IWbemLocator* wbemLocator)
{
    if (!wbemLocator)
        throw std::invalid_argument { "locator is null." };

    ComPtr<IWbemServices> services;
    if (FAILED(wbemLocator->ConnectServer(
            _bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &services)))
        throw std::runtime_error { "Failed to create an IWbemServices instance." };

    if (FAILED(CoSetProxyBlanket(services.Get(),
                                 RPC_C_AUTHN_WINNT,
                                 RPC_C_AUTHZ_NONE,
                                 NULL,
                                 RPC_C_AUTHN_LEVEL_CALL,
                                 RPC_C_IMP_LEVEL_IMPERSONATE,
                                 NULL,
                                 EOAC_NONE)))
        throw std::runtime_error {
            "Failed to set the authentication information on the IWbemServices instance."
        };

    return services;
}

ComPtr<IUnsecuredApartment> CreateUnsecuredApartment()
{
    ComPtr<IUnsecuredApartment> unsecuredApt;
    if (FAILED(CoCreateInstance(CLSID_UnsecuredApartment,
                                NULL,
                                CLSCTX_LOCAL_SERVER,
                                IID_IUnsecuredApartment,
                                (void**)&unsecuredApt)))
        throw std::runtime_error { "Failed to create an IUnsecuredApartment instance." };

    return unsecuredApt;
}

ComPtr<IWbemObjectSink> CreateObjectSink(ktmac::WmiEventSink* eventSink,
                                         IUnsecuredApartment* unsecuredApt)
{
    ComPtr<IUnknown> stubUnk;
    if (FAILED(unsecuredApt->CreateObjectStub(eventSink, &stubUnk)))
        throw std::runtime_error { "Failed to create an IWbemObjectSink instance." };

    ComPtr<IWbemObjectSink> stubSink;
    if (FAILED(stubUnk->QueryInterface(stubSink.GetAddressOf())))
        throw std::runtime_error { "Failed to query an IWbemObjectSink instance." };

    return stubSink;
}

}

namespace ktmac
{

bool ProcessWatcher::CheckAdministratorPrivilege()
{
    HANDLE token = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
        return false;

    TOKEN_ELEVATION elevation;
    DWORD           size = sizeof elevation;
    if (!GetTokenInformation(token, TokenElevation, &elevation, size, &size))
    {
        CloseHandle(token);
        return false;
    }

    return elevation.TokenIsElevated;
}

bool ProcessWatcher::InitializeCom()
{
    if (FAILED(CoInitializeEx(0, COINIT_MULTITHREADED)))
        return false;

    if (FAILED(CoInitializeSecurity(NULL,
                                    -1,
                                    NULL,
                                    NULL,
                                    RPC_C_AUTHN_LEVEL_DEFAULT,
                                    RPC_C_IMP_LEVEL_IMPERSONATE,
                                    NULL,
                                    EOAC_NONE,
                                    NULL)))
        return false;

    return true;
}

void ProcessWatcher::UninitializeCom()
{
    CoUninitialize();
}

ProcessWatcher::ProcessWatcher(std::string&& processName, ProcessStateHandler&& stateHandler) :
    _wbemLocator { std::move(CreateWbemLocator()) },
    _services { std::move(CreateWbemServices(_wbemLocator.Get())) },
    _unsecuredApt { std::move(CreateUnsecuredApartment()) },
    _processStartDetector {
        new WmiEventSink([this](auto, auto) {
            if (_stateHandler)
                _stateHandler(ProcessState::Running);
        }),
    },
    _processStopDetector {
        new WmiEventSink([this](auto, auto) {
            if (_stateHandler)
                _stateHandler(ProcessState::Stopped);
        }),
    },
    _processStartSink { CreateObjectSink(_processStartDetector, _unsecuredApt.Get()) },
    _processStopSink { CreateObjectSink(_processStopDetector, _unsecuredApt.Get()) },
    _processName { std::move(processName) },
    _stateHandler { std::move(stateHandler) }
{
    std::string processStartQuery
        = "SELECT * FROM win32_ProcessStartTrace where processname LIKE '%" + _processName + "%'";
    if (FAILED(_services->ExecNotificationQueryAsync(_bstr_t("WQL"),
                                                     _bstr_t(processStartQuery.c_str()),
                                                     WBEM_FLAG_SEND_STATUS,
                                                     NULL,
                                                     _processStartSink.Get())))
        throw std::runtime_error { "Failed to execute win32_ProcessStartTrace query." };

    std::string processStopQuery
        = "SELECT * FROM win32_ProcessStopTrace where processname LIKE '%" + _processName + "%'";
    if (FAILED(_services->ExecNotificationQueryAsync(_bstr_t("WQL"),
                                                     _bstr_t(processStopQuery.c_str()),
                                                     WBEM_FLAG_SEND_STATUS,
                                                     NULL,
                                                     _processStopSink.Get())))
        throw std::runtime_error { "Failed to execute win32_ProcessStopTrace query." };
}

ProcessWatcher::~ProcessWatcher()
{
    _services->CancelAsyncCall(_processStartSink.Get());
    _services->CancelAsyncCall(_processStopSink.Get());
}

}