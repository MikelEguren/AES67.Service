#pragma once

#include <string>
#include <vector>

namespace aes67::ipc
{
    struct IpcReceiveResult
    {
        bool Success{ true };
        std::vector<std::string> Messages;
        std::string ErrorMessage;
    };
}
