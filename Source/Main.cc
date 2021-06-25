// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#include <ktmac/KakaoStateManager.hh>

#include <iostream>
#include <string>

#undef SendMessage

#define HANDLE_CASE(Name)                                                                          \
    case KakaoState::Name:                                                                         \
    {                                                                                              \
        std::cout << "State changed: " << #Name << std::endl;                                      \
        break;                                                                                     \
    }

using namespace ktmac;

int main()
{
    KakaoStateManager manager {
        [](KakaoState state) {
            switch (state)
            {
                HANDLE_CASE(NotRunning)
                HANDLE_CASE(LoggedOut)
                HANDLE_CASE(Background)
                HANDLE_CASE(Locked)
                HANDLE_CASE(ContactListIsVisible)
                HANDLE_CASE(ChatroomListIsVisible)
                HANDLE_CASE(MiscIsVisible)
                HANDLE_CASE(ChatroomIsVisible)
            }
        },
    };

    std::string message;
    while (true)
    {
        std::getline(std::cin, message);
        if (message.empty())
            break;

        if (!manager.SetMessage(message) || !manager.SendMessage())
            std::cout << "Message was not sent." << std::endl;
    }

    std::cout << "Waiting for cleanup..." << std::endl;
}