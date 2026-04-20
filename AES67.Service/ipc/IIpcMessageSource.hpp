#pragma once

#include "ipc/IpcReceiveResult.hpp"

namespace aes67::ipc
{
    class IIpcMessageSource
    {
    public:
        virtual ~IIpcMessageSource() = default;

        virtual IpcReceiveResult ReceiveMessages() = 0;
    };
}