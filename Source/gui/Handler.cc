// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/KakaoStateManager.hh>
#include <ktmac/gui/Handler.hh>

#include <include/base/cef_callback.h>
#include <include/cef_app.h>
#include <include/cef_parser.h>
#include <include/views/cef_browser_view.h>
#include <include/views/cef_window.h>
#include <include/wrapper/cef_closure_task.h>
#include <include/wrapper/cef_helpers.h>

#include <sstream>
#include <string>

namespace
{

const char*               g_lastState = "";
SimpleHandler*            g_instance  = nullptr;
ktmac::KakaoStateManager* g_manager   = nullptr;

// Returns a data: URI with the specified contents.
std::string GetDataURI(const std::string& data, const std::string& mime_type)
{
    return "data:" + mime_type + ";base64,"
           + CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
}

} // namespace

#define HANDLE_CASE(Name, RealName)                                                                \
    case ktmac::KakaoState::Name:                                                                  \
    {                                                                                              \
        g_lastState = RealName;                                                                    \
        g_instance->NotifyStateChange(RealName);                                                   \
        break;                                                                                     \
    }

SimpleHandler::SimpleHandler(bool use_views) : use_views_(use_views), is_closing_(false)
{
    DCHECK(!g_instance);
    g_instance = this;
    g_manager  = new ktmac::KakaoStateManager {
        {
            this,
            [](void* browser, ktmac::KakaoState state) {
                switch (state)
                {
                    HANDLE_CASE(NotRunning, "not-running")
                    HANDLE_CASE(LoggedOut, "logged-out")
                    HANDLE_CASE(Background, "background")
                    HANDLE_CASE(Locked, "locked")
                    HANDLE_CASE(ContactListIsVisible, "contact-list-is-visible")
                    HANDLE_CASE(ChatroomListIsVisible, "chatroom-list-is-visible")
                    HANDLE_CASE(MiscIsVisible, "misc-is-visible")
                    HANDLE_CASE(ChatroomIsVisible, "chatroom-is-visible")
                }
            },
        },
    };
}

SimpleHandler::~SimpleHandler()
{
    g_instance = nullptr;
    delete g_manager;
    g_manager = nullptr;
}

// static
SimpleHandler* SimpleHandler::GetInstance()
{
    return g_instance;
}

void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    CEF_REQUIRE_UI_THREAD();

    if (use_views_)
    {
        // Set the title of the window using the Views framework.
        CefRefPtr<CefBrowserView> browser_view = CefBrowserView::GetForBrowser(browser);
        if (browser_view)
        {
            CefRefPtr<CefWindow> window = browser_view->GetWindow();
            if (window)
                window->SetTitle(title);
        }
    }
    else if (!IsChromeRuntimeEnabled())
    {
        // Set the title of the window using platform APIs.
        PlatformTitleChange(browser, title);
    }
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    // Add to the list of existing browsers.
    browser_list_.push_back(browser);
}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed destription of this
    // process.
    if (browser_list_.size() == 1)
    {
        // Set a flag to indicate that the window close should be allowed.
        is_closing_ = true;
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser)
{
    CEF_REQUIRE_UI_THREAD();

    // Remove from the list of existing browsers.
    BrowserList::iterator bit = browser_list_.begin();
    for (; bit != browser_list_.end(); ++bit)
    {
        if ((*bit)->IsSame(browser))
        {
            browser_list_.erase(bit);
            break;
        }
    }

    if (browser_list_.empty())
    {
        // All browser windows have closed. Quit the application message loop.
        CefQuitMessageLoop();
    }
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame>   frame,
                                ErrorCode             errorCode,
                                const CefString&      errorText,
                                const CefString&      failedUrl)
{
    CEF_REQUIRE_UI_THREAD();

    // Allow Chrome to show the error page.
    if (IsChromeRuntimeEnabled())
        return;

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    // Display a load error message using a data: URI.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
          "<h2>Failed to load URL "
       << std::string(failedUrl) << " with error " << std::string(errorText) << " (" << errorCode
       << ").</h2></body></html>";

    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void SimpleHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame>   frame,
                              int                   httpStatusCode)
{
    NotifyStateChange(g_lastState);
}

void SimpleHandler::CloseAllBrowsers(bool force_close)
{
    if (!CefCurrentlyOn(TID_UI))
    {
        // Execute on the UI thread.
        CefPostTask(TID_UI, base::Bind(&SimpleHandler::CloseAllBrowsers, this, force_close));
        return;
    }

    if (browser_list_.empty())
        return;

    BrowserList::const_iterator it = browser_list_.begin();
    for (; it != browser_list_.end(); ++it) (*it)->GetHost()->CloseBrowser(force_close);
}

// static
bool SimpleHandler::IsChromeRuntimeEnabled()
{
    static int value = -1;
    if (value == -1)
    {
        CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();
        value = command_line->HasSwitch("enable-chrome-runtime") ? 1 : 0;
    }
    return value == 1;
}

#undef SendMessage

bool SimpleHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser>        browser,
                                             CefRefPtr<CefFrame>          frame,
                                             CefProcessId                 source_process,
                                             CefRefPtr<CefProcessMessage> message)
{
    if (message->GetName() == "ktmac-send-message")
    {
        if (g_manager)
        {
            auto arguments = message->GetArgumentList();
            g_manager->SetMessage(arguments->GetString(0).c_str());
            g_manager->SendMessage();
        }
        return true;
    }
    return false;
}

void SimpleHandler::NotifyStateChange(const char* state)
{
    CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("ktmac-state-change");
    CefRefPtr<CefListValue>      args    = message->GetArgumentList();
    args->SetString(0, state);

    for (auto& browser : browser_list_)
    {
        browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, message);
    }
}

void SimpleHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title)
{
    CefWindowHandle hwnd = browser->GetHost()->GetWindowHandle();
    if (hwnd)
        SetWindowText(hwnd, std::wstring(title).c_str());
}
