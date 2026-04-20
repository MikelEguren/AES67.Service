#include "ipc/IpcMessageSerializer.hpp"

#include <sstream>
#include <vector>

namespace aes67::ipc
{
    std::string IpcMessageSerializer::TrimTrailingCarriageReturn(const std::string& value)
    {
        if (!value.empty() && value.back() == '\r')
        {
            return value.substr(0, value.size() - 1);
        }

        return value;
    }

    bool IpcMessageSerializer::TryParseRequest(const std::string& message, IpcRequest& request)
    {
        std::vector<std::string> parts;
        std::stringstream stream(message);
        std::string part;

        while (std::getline(stream, part, '|'))
        {
            parts.push_back(TrimTrailingCarriageReturn(part));
        }

        if (parts.empty())
        {
            return false;
        }

        if (parts[0] == "PREPARE")
        {
            if (parts.size() < 2)
            {
                return false;
            }

            request.CommandType = IpcCommandType::PreparePlayback;
            request.SourcePath = parts[1];
            request.SessionId.clear();
            return true;
        }

        if (parts[0] == "START")
        {
            if (parts.size() < 2)
            {
                return false;
            }

            request.CommandType = IpcCommandType::StartPlayback;
            request.SessionId = parts[1];
            request.SourcePath.clear();
            return true;
        }

        if (parts[0] == "FINISH")
        {
            if (parts.size() < 2)
            {
                return false;
            }

            request.CommandType = IpcCommandType::FinishPlayback;
            request.SessionId = parts[1];
            request.SourcePath.clear();
            return true;
        }

        request.CommandType = IpcCommandType::Unknown;
        return false;
    }

    bool IpcMessageSerializer::TryParseResponse(const std::string& message, IpcResponse& response)
    {
        std::vector<std::string> parts;
        std::stringstream stream(message);
        std::string part;

        while (std::getline(stream, part, '|'))
        {
            parts.push_back(TrimTrailingCarriageReturn(part));
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