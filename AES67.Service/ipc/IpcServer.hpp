#pragma once

#include <string>
#include <vector>

namespace aes67::app
{
    class Application;
}

namespace aes67::ipc
{
    class IpcServer
    {
    public:
        explicit IpcServer(aes67::app::Application& application);

        std::string ProcessMessage(const std::string& message);
        std::vector<std::string> ProcessMessages(const std::vector<std::string>& messages);

    private:
        aes67::app::Application& _application;
    };
}