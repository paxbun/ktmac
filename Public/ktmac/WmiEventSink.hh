// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_WMI_HANDLER_HH
#define KTMAC_WMI_HANDLER_HH

#include <WbemIdl.h>
#include <Windows.h>

#include <functional>

namespace ktmac
{

using WmiHandler = std::function<void(long numObjects, IWbemClassObject** objects)>;

class WmiEventSink : public ::IWbemObjectSink
{
  private:
    LONG       _numRef;
    bool       _done;
    WmiHandler _handler;

  public:
    WmiEventSink(WmiHandler&& handler) :
        _numRef { 0 },
        _done { false },
        _handler { std::move(handler) }
    {}

    ~WmiEventSink()
    {
        _done = true;
    }

    virtual ULONG STDMETHODCALLTYPE   AddRef();
    virtual ULONG STDMETHODCALLTYPE   Release();
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv);
    virtual HRESULT STDMETHODCALLTYPE Indicate(long numObjects, IWbemClassObject** objects);
    virtual HRESULT STDMETHODCALLTYPE SetStatus(LONG              flags,
                                                HRESULT           result,
                                                BSTR              param,
                                                IWbemClassObject* object);
};

}

#endif