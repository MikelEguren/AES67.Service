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
        int ServiceLoopIterationCount{ 30 };        
        int SampleRate{ 48000 };
        int BitsPerSample{ 16 };
        bool Mono{ true };
        bool EnableLocalMonitor{ false };
        int Aes67PacketTimeMs{ 5 };
        std::string IpcSocketPath{ "/tmp/aes67.sock" };

        std::string Aes67DestinationIp{ "127.0.0.1" };
        int Aes67DestinationPort{ 5004 };
    };
}
