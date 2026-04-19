#pragma once

#include <string>

namespace aes67::commands
{
    struct FinishPlaybackCommand
    {
        std::string SessionId;
    };
}