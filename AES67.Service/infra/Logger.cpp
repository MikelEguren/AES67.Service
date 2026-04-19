#include "Logger.hpp"

#include <iostream>

namespace aes67::infra
{
    void Logger::Info(const char* message)
    {
        std::cout << "[INFO] " << message << std::endl;
    }

    void Logger::Error(const char* message)
    {
        std::cerr << "[ERROR] " << message << std::endl;
    }
}