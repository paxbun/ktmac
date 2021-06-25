// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/WmiEventSink.hh>

namespace ktmac
{

ULONG STDMETHODCALLTYPE WmiEventSink::AddRef()
{
    return InterlockedIncrement(&_numRef);
}

ULONG STDMETHODCALLTYPE WmiEventSink::Release()
{
    LONG ref = InterlockedDecrement(&_numRef);
    if (ref == 0)
        delete this;
    return ref;
}

HRESULT STDMETHODCALLTYPE WmiEventSink::QueryInterface(REFIID riid, void** ppv)
{
    if (riid == IID_IUnknown || riid == IID_IWbemObjectSink)
    {
        *ppv = (IWbemObjectSink*)this;
        AddRef();
        return WBEM_S_NO_ERROR;
    }
    else
        return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE WmiEventSink::Indicate(long numObjects, IWbemClassObject** objects)
{
    if (numObjects != 0 && _handler)
        _handler(numObjects, objects);

    return WBEM_S_NO_ERROR;
}

HRESULT STDMETHODCALLTYPE WmiEventSink::SetStatus(LONG              flags,
                                                  HRESULT           result,
                                                  BSTR              param,
                                                  IWbemClassObject* object)
{
    return WBEM_S_NO_ERROR;
}

}