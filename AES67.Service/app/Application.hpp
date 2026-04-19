#pragma once

#include "config/ServiceConfig.hpp"

namespace aes67::app
{
    class Application
    {
    public:
        Application();
        int Run();

    private:
        aes67::config::ServiceConfig _config;
    };
}