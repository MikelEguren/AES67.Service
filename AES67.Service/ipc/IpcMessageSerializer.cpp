#include "ipc/IpcMessageSerializer.hpp"

#include <sstream>
#include <vector>

namespace aes67::ipc
{
    std::string IpcMessageSerializer::TrimToken(const std::string& value)
    {
        std::size_t start = 0;
        std::size_t end = value.size();

        while (start < end && (value[start] == ' ' || value[start] == '\t' || value[start] == '\r' || value[start] == '\n'))
        {
            ++start;
        }

        while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t' || value[end - 1] == '\r' || value[end - 1] == '\n'))
        {
            --end;
        }

        return value.substr(start, end - start);
    }

    bool IpcMessageSerializer::TryParseRequest(const std::string& message, IpcRequest& request, std::string& errorMessage)
    {
        std::vector<std::string> parts;
        std::stringstream stream(message);
        std::string part;

        while (std::getline(stream, part, '|'))
        {
            parts.push_back(TrimToken(part));
        }

        if (parts.empty())
        {
            errorMessage = "IPC request is empty.";
            return false;
        }

        if (parts[0] == "PREPARE")
        {
            if (parts.size() < 2)
            {
                errorMessage = "PREPARE request requires source path.";
                return false;
            }

            request.CommandType = IpcCommandType::PreparePlayback;
            request.SourcePath = parts[1];
            request.SessionId.clear();
            errorMessage.clear();
            return true;
        }

        if (parts[0] == "START")
        {
            if (parts.size() < 2)
            {
                errorMessage = "START request requires session id.";
                return false;
            }

            request.CommandType = IpcCommandType::StartPlayback;
            request.SessionId = parts[1];
            request.SourcePath.clear();
            errorMessage.clear();
            return true;
        }

        if (parts[0] == "FINISH")
        {
            if (parts.size() < 2)
            {
                errorMessage = "FINISH request requires session id.";
                return false;
            }

            request.CommandType = IpcCommandType::FinishPlayback;
            request.SessionId = parts[1];
            request.SourcePath.clear();
            errorMessage.clear();
            return true;
        }

        request.CommandType = IpcCommandType::Unknown;
        errorMessage = "Unknown IPC command.";
        return false;
    }

    bool IpcMessageSerializer::TryParseResponse(const std::string& message, IpcResponse& response)
    {
        std::vector<std::string> parts;
        std::stringstream stream(message);
        std::string part;

        while (std::getline(stream, part, '|'))
        {
            parts.push_back(TrimToken(part));
        }

        if (parts.size() < 3)
        {
            return false;
        }

        if (parts[0] == "OK")
        {
            response.Success = true;
        }
        else if (parts[0] == "ERROR")
        {
            response.Success = false;
        }
        else
        {
            return false;
        }

        response.SessionId = parts[1];

        if (!parts[2].empty())
        {
            response.ChannelNumber = std::stoi(parts[2]);
        }
        else
        {
            response.ChannelNumber = 0;
        }

        if (parts.size() >= 4)
        {
            response.ErrorMessage = parts[3];
        }
        else
        {
            response.ErrorMessage.clear();
        }

        return true;
    }

    std::string IpcMessageSerializer::SerializeResponse(const IpcResponse& response)
    {
        std::string status = response.Success ? "OK" : "ERROR";
        std::string channelNumber = response.Success ? std::to_string(response.ChannelNumber) : "";

        return status + "|" +
            response.SessionId + "|" +
            channelNumber + "|" +
            response.ErrorMessage;
    }
}