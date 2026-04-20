#include "ipc/IpcServer.hpp"

#include "app/Application.hpp"
#include "ipc/IpcMessageSerializer.hpp"

namespace aes67::ipc
{
    IpcServer::IpcServer(aes67::app::Application& application)
        : _application(application)
    {}

    std::string IpcServer::ProcessMessage(const std::string& message)
    {
        aes67::ipc::IpcRequest request;
        if (!aes67::ipc::IpcMessageSerializer::TryParseRequest(message, request))
        {
            aes67::ipc::IpcResponse errorResponse;
            errorResponse.Success = false;
            errorResponse.ErrorMessage = "Invalid IPC request format.";
            return aes67::ipc::IpcMessageSerializer::SerializeResponse(errorResponse);
        }

        aes67::ipc::IpcResponse response = _application.HandleRequest(request);
        return aes67::ipc::IpcMessageSerializer::SerializeResponse(response);
    }
}