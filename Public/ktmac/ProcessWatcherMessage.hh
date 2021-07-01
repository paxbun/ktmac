// Copyright (c) 2021 Chanjung Kim (paxbun). All rights reserved.
// Licensed under the MIT License.

#ifndef KTMAC_PROCESS_WATCHER_MESSAGE_HH
#define KTMAC_PROCESS_WATCHER_MESSAGE_HH

#include <cstdint>

namespace ktmac
{

enum class ProcessWatcherMessage : uint8_t
{
    Running,
    Stopped,
    Quit
};

}

#endif