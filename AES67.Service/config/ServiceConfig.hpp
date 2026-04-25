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
        bool EnableLocalMonitor{ false };
        std::string IpcSocketPath{ "/tmp/aes67.sock" };
    };
}
