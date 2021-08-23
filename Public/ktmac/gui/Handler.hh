// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_GUI_HANDLER_HH
#define KTMAC_GUI_HANDLER_HH

#include <include/cef_client.h>

#include <list>

class SimpleHandler :
    public CefClient,
    public CefDisplayHandler,
    public CefLifeSpanHandler,
    public CefLoadHandler
{
  public:
    explicit SimpleHandler(bool use_views);
    ~SimpleHandler();

    // Provide access to the single global instance of this object.
    static SimpleHandler* GetInstance();

    // CefClient methods:
    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
    {
        return this;
    }
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
    {
        return this;
    }
    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override
    {
        return this;
    }

    // CefDisplayHandler methods:
    virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override;

    // CefLifeSpanHandler methods:
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // CefLoadHandler methods:
    virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame>   frame,
                             ErrorCode             errorCode,
                             const CefString&      errorText,
                             const CefString&      failedUrl) override;
    virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame>   frame,
                           int                   httpStatusCode) override;

    // Request that all existing browser windows close.
    void CloseAllBrowsers(bool force_close);

    bool IsClosing() const
    {
        return is_closing_;
    }

    // Returns true if the Chrome runtime is enabled.
    static bool IsChromeRuntimeEnabled();

    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser>        browser,
                                          CefRefPtr<CefFrame>          frame,
                                          CefProcessId                 source_process,
                                          CefRefPtr<CefProcessMessage> message);

    void NotifyStateChange(const char* state);

  private:
    // Platform-specific implementation.
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);

    // True if the application is using the Views framework.
    const bool use_views_;

    // List of existing browser windows. Only accessed on the CEF UI thread.
    typedef std::list<CefRefPtr<CefBrowser>> BrowserList;
    BrowserList                              browser_list_;

    bool is_closing_;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(SimpleHandler);
};

#endif