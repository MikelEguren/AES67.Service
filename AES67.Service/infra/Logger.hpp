#pragma once

namespace aes67::infra
{
    class Logger
    {
    public:
        static void Info(const char* message);
        static void Error(const char* message);
    };
}