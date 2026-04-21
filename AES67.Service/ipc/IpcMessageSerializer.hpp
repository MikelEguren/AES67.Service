#pragma once

#include <string>

#include "ipc/IpcRequest.hpp"
#include "ipc/IpcResponse.hpp"

namespace aes67::ipc
{
    class IpcMessageSerializer
    {
    public:
        static bool TryParseRequest(const std::string& message, IpcRequest& request, std::string& errorMessage);
        static bool TryParseResponse(const std::string& message, IpcResponse& response);
        static std::string SerializeResponse(const IpcResponse& response);

    private:
        static std::string TrimToken(const std::string& value);
    };
}