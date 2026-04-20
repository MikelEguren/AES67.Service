#pragma once

#include <string>

#include "config/ExecutionMode.hpp"

namespace aes67::config
{
    struct ServiceConfig
    {
        ExecutionMode Mode{ ExecutionMode::ServiceLoop };
        //ExecutionMode Mode{ ExecutionMode::RunOnce };
        int ChannelCount{ 4 };
        int ServiceLoopIterationCount{ 3 };
        int SampleRate{ 48000 };
        int BitsPerSample{ 16 };
        bool Mono{ true };
        std::string IpcSocketPath{ "/run/aes67/aes67.sock" };
    };
}
