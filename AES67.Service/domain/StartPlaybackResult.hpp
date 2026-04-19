#pragma once

#include <string>

namespace aes67::domain
{
    struct StartPlaybackResult
    {
        bool Success{ false };
        std::string SessionId;
        int ChannelNumber{ 0 };
        std::string ErrorMessage;
    };
}