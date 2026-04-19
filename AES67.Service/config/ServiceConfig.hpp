#pragma once

#include <string>

namespace aes67::config
{
    struct ServiceConfig
    {
        int ChannelCount{ 4 };
        int SampleRate{ 48000 };
        int BitsPerSample{ 16 };
        bool Mono{ true };
        std::string IpcSocketPath{ "/run/aes67/aes67.sock" };
    };
}
