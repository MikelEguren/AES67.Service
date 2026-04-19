#pragma once

#include <string>

namespace aes67::commands
{
    struct StartPlaybackCommand
    {
        std::string SessionId;
    };
}