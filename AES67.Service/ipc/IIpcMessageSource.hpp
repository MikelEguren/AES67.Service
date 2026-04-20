#pragma once

#include <string>
#include <vector>

namespace aes67::ipc
{
    class IIpcMessageSource
    {
    public:
        virtual ~IIpcMessageSource() = default;

        virtual std::vector<std::string> ReceiveMessages() = 0;
    };
}