#pragma once

#include <string>

namespace aes67::commands
{
    struct PreparePlaybackCommand
    {
        std::string SourcePath;
    };
}