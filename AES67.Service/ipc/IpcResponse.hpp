#pragma once

#include <string>

namespace aes67::ipc
{
    struct IpcResponse
    {
        bool Success{ false };
        std::string SessionId;
        int ChannelNumber{ 0 };
        std::string ErrorMessage;
    };
}