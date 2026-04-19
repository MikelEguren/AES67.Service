#pragma once

#include <string>
#include "PlaybackSessionState.hpp"

namespace aes67::domain
{
    struct PlaybackSession
    {
        std::string SessionId;
        std::string SourcePath;
        int ChannelNumber{ 0 };
        PlaybackSessionState State{ PlaybackSessionState::Created };
    };
}