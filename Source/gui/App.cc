// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/gui/App.hh>
#include <ktmac/gui/Handler.hh>
#include <ktmac/gui/ResourceUtils.hh>
#include <ktmac/gui/Resources.h>

#include <include/base/cef_bind.h>
#include <include/cef_command_line.h>
#include <include/cef_parser.h>
#include <include/views/cef_browser_view.h>
#include <include/views/cef_window.h>
#include <include/wrapper/cef_closure_task.h>
#include <include/wrapper/cef_helpers.h>

#include <string>

namespace
{

class WindowDelegate : public CefWindowDelegate
{
  private:
    CefRefPtr<CefBrowserView> _browserView;

  public:
    WindowDelegate(CefRefPtr<CefBrowserView> browserView) : _browserView { browserView } {}

  public:
    virtual void OnWindowCreated(CefRefPtr<CefWindow> window) override
    {
        window->AddChildView(_browserView);
        window->Show();

        _browserView->RequestFocus();
    }

    virtual void OnWindowDestroyed(CefRefPtr<CefWindow> window) override
    {
        _browserView = nullptr;
    }

    virtual bool CanResize(CefRefPtr<CefWindow>) override
    {
        return false;
    }

    virtual bool CanMaximize(CefRefPtr<CefWindow>) override
    {
        return false;
    }

    virtual bool CanClose(CefRefPtr<CefWindow>) override
    {
        CefRefPtr<CefBrowser> browser { _browserView->GetBrowser() };
        if (browser)
            return browser->GetHost()->TryCloseBrowser();
        return true;
    }

    virtual CefSize GetPreferredSize(CefRefPtr<CefView>) override
    {
        return CefSize { 300, 150 };
    }

  private:
    IMPLEMENT_REFCOUNTING(WindowDelegate);
    DISALLOW_COPY_AND_ASSIGN(WindowDelegate);
};

class BrowserViewDelegate : public CefBrowserViewDelegate
{
  public:
    BrowserViewDelegate() = default;

  public:
    virtual bool OnPopupBrowserViewCreated(CefRefPtr<CefBrowserView>,
                                           CefRefPtr<CefBrowserView> popupBrowserView,
                                           bool                      isDevtools) override
    {
        if (isDevtools)
            return false;

        CefWindow::CreateTopLevelWindow(new WindowDelegate { popupBrowserView });
        return true;
    }

  private:
    IMPLEMENT_REFCOUNTING(BrowserViewDelegate);
    DISALLOW_COPY_AND_ASSIGN(BrowserViewDelegate);
};

std::string GetDataURI(void const* data, size_t size, char const* mimeType)
{
    using namespace std::string_literals;
    return "data:"s + mimeType + ";base64,"
           + CefURIEncode(CefBase64Encode(data, size), false).ToString();
}

}

namespace ktmac::gui
{

void App::OnContextInitialized()
{
    CEF_REQUIRE_UI_THREAD();

    CefRefPtr<SimpleHandler> handler { new SimpleHandler(true) };
    CefBrowserSettings       browserSettings;
    CefWindowInfo            windowInfo;

    std::string_view html { LoadResource(KTMAC_RESTYPE_HTML, KTMAC_RES_VIEW_HTML) };
    std::string      url { GetDataURI(html.data(), html.size(), "text/html") };

    CefRefPtr<CefBrowserView> browserView = CefBrowserView::CreateBrowserView(
        handler, url, browserSettings, nullptr, nullptr, new BrowserViewDelegate());

    // Create the Window. It will show itself after creation.
    CefWindow::CreateTopLevelWindow(new WindowDelegate(browserView));
}

CefRefPtr<CefClient> App::GetDefaultClient()
{
    return SimpleHandler::GetInstance();
}

void App::OnContextCreated(CefRefPtr<CefBrowser>   browser,
                           CefRefPtr<CefFrame>     frame,
                           CefRefPtr<CefV8Context> context)
{
    _browser = browser;

    CefRefPtr<CefV8Value> object = context->GetGlobal();
    object->SetValue("ktmacSendMessage",
                     CefV8Value::CreateFunction("ktmacSendMessage", this),
                     V8_PROPERTY_ATTRIBUTE_NONE);
    object->SetValue("ktmacGetCurrentState",
                     CefV8Value::CreateFunction("ktmacGetCurrentState", this),
                     V8_PROPERTY_ATTRIBUTE_NONE);
}

bool App::Execute(const CefString&       name,
                  CefRefPtr<CefV8Value>  object,
                  const CefV8ValueList&  arguments,
                  CefRefPtr<CefV8Value>& retval,
                  CefString&             exception)
{
    if (name == "ktmacSendMessage")
    {
        if (arguments.size() != 1)
        {
            exception.FromString("Invalid number of arguments");
            return true;
        }

        auto content = arguments[0];

        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("ktmac-send-message");
        CefRefPtr<CefListValue>      args    = message->GetArgumentList();

        args->SetString(0, content->GetStringValue().c_str());

        auto frame = _browser->GetMainFrame();
        frame->SendProcessMessage(PID_BROWSER, message);

        return true;
    }
    else if (name == "ktmacGetCurrentState")
    {
        CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("ktmac-get-current-state");

        auto frame = _browser->GetMainFrame();
        frame->SendProcessMessage(PID_BROWSER, message);
        return true;
    }

    return false;
}

bool App::OnProcessMessageReceived(CefRefPtr<CefBrowser>        browser,
                                   CefRefPtr<CefFrame>          frame,
                                   CefProcessId                 source_process,
                                   CefRefPtr<CefProcessMessage> message)
{
    using namespace std::string_literals;

    if (message->GetName() == "ktmac-state-change")
    {
        auto arguments = message->GetArgumentList();
        if (arguments->GetSize() != 1)
            return false;

        std::wstring script
            = L"window.dispatchEvent(new CustomEvent('ktmac-state-changed',{'detail':'"s;

        script += arguments->GetString(0);
        script += L"'}));";

        frame->ExecuteJavaScript(script, frame->GetURL(), 0);
        return true;
    }
    return false;
}

}