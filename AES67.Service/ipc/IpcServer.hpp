#pragma once

#include <string>

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

    private:
        aes67::app::Application& _application;
    };
}