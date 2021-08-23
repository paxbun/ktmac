// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_GUI_APP_HH
#define KTMAC_GUI_APP_HH

#include <include/cef_app.h>

namespace ktmac::gui
{

class App :
    public CefApp,
    public CefBrowserProcessHandler,
    public CefRenderProcessHandler,
    public CefV8Handler
{
  public:
    App() = default;

    virtual CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
    {
        return this;
    }

    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
    {
        return this;
    }

    virtual void                 OnContextInitialized() override;
    virtual CefRefPtr<CefClient> GetDefaultClient() override;
    virtual void                 OnContextCreated(CefRefPtr<CefBrowser>   browser,
                                                  CefRefPtr<CefFrame>     frame,
                                                  CefRefPtr<CefV8Context> context) override;
    virtual bool                 Execute(const CefString&       name,
                                         CefRefPtr<CefV8Value>  object,
                                         const CefV8ValueList&  arguments,
                                         CefRefPtr<CefV8Value>& retval,
                                         CefString&             exception) override;
    virtual bool                 OnProcessMessageReceived(CefRefPtr<CefBrowser>        browser,
                                                          CefRefPtr<CefFrame>          frame,
                                                          CefProcessId                 source_process,
                                                          CefRefPtr<CefProcessMessage> message) override;

  private:
    CefRefPtr<CefBrowser> _browser;

  private:
    IMPLEMENT_REFCOUNTING(ktmac::gui::App);
};

}

#endif