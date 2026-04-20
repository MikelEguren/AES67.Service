#pragma once

#include <string>

#include "ipc/IpcCommandType.hpp"

namespace aes67::ipc
{
    struct IpcRequest
    {
        IpcCommandType CommandType{ IpcCommandType::Unknown };
        std::string SourcePath;
        std::string SessionId;
    };
}