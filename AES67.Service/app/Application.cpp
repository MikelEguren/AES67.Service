#include "Application.hpp"

#include <string>

#include "infra/Logger.hpp"

namespace aes67::app
{
    Application::Application()
    {}

    int Application::Run()
    {
        aes67::infra::Logger::Info("AES67 Service starting...");

        std::string channelMessage = "Configured channels: " + std::to_string(_config.ChannelCount);
        aes67::infra::Logger::Info(channelMessage.c_str());

        std::string socketMessage = "IPC socket path: " + _config.IpcSocketPath;
        aes67::infra::Logger::Info(socketMessage.c_str());

        aes67::infra::Logger::Info("Service startup completed.");

        return 0;
    }
}