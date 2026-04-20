#include "Application.hpp"

#include <string>

#include "infra/Logger.hpp"

namespace aes67::app
{
    Application::Application()
        : _config(),
        _channelManager(_config.ChannelCount),
        _playbackSessionManager()
    {}

    bool Application::ValidateConfig()
    {
        if (_config.ChannelCount != 4 &&
            _config.ChannelCount != 8 &&
            _config.ChannelCount != 16)
        {
            aes67::infra::Logger::Error("Invalid channel count. Only 4, 8 or 16 are allowed.");
            return false;
        }

        return true;
    }

    aes67::domain::PreparePlaybackResult Application::Execute(const aes67::commands::PreparePlaybackCommand& command)
    {
        return PreparePlayback(command.SourcePath);
    }

    aes67::domain::StartPlaybackResult Application::Execute(const aes67::commands::StartPlaybackCommand& command)
    {
        return StartPlayback(command.SessionId);
    }

    aes67::domain::FinishPlaybackResult Application::Execute(const aes67::commands::FinishPlaybackCommand& command)
    {
        return FinishPlayback(command.SessionId);
    }

    aes67::domain::PreparePlaybackResult Application::PreparePlayback(const std::string& sourcePath)
    {
        aes67::domain::PreparePlaybackResult result;

        aes67::domain::ChannelInfo reservedChannel;
        if (!_channelManager.TryReserveNextFreeChannel(reservedChannel))
        {
            result.Success = false;
            result.ErrorMessage = "No free channel available.";
            return result;
        }

        aes67::domain::PlaybackSession session =
            _playbackSessionManager.CreateSession(sourcePath, reservedChannel.ChannelNumber);

        if (!_playbackSessionManager.MarkSessionReady(session.SessionId))
        {
            _channelManager.ReleaseChannel(reservedChannel.ChannelNumber);

            result.Success = false;
            result.ErrorMessage = "Failed to mark session as ready.";
            return result;
        }

        result.Success = true;
        result.SessionId = session.SessionId;
        result.ChannelNumber = session.ChannelNumber;
        return result;
    }

    aes67::domain::StartPlaybackResult Application::StartPlayback(const std::string& sessionId)
    {
        aes67::domain::StartPlaybackResult result;

        aes67::domain::PlaybackSession session;
        if (!_playbackSessionManager.TryGetSession(sessionId, session))
        {
            result.Success = false;
            result.ErrorMessage = "Session not found.";
            return result;
        }

        if (!_playbackSessionManager.MarkSessionPlaying(sessionId))
        {
            result.Success = false;
            result.ErrorMessage = "Session is not in ready state.";
            return result;
        }

        if (!_channelManager.MarkChannelPlaying(session.ChannelNumber))
        {
            result.Success = false;
            result.ErrorMessage = "Channel is not reserved or cannot transition to playing.";
            return result;
        }

        result.Success = true;
        result.SessionId = session.SessionId;
        result.ChannelNumber = session.ChannelNumber;
        return result;
    }

    aes67::domain::FinishPlaybackResult Application::FinishPlayback(const std::string& sessionId)
    {
        aes67::domain::FinishPlaybackResult result;

        aes67::domain::PlaybackSession session;
        if (!_playbackSessionManager.TryGetSession(sessionId, session))
        {
            result.Success = false;
            result.ErrorMessage = "Session not found.";
            return result;
        }

        if (!_playbackSessionManager.MarkSessionFinished(sessionId))
        {
            result.Success = false;
            result.ErrorMessage = "Session is not in playing state.";
            return result;
        }

        if (!_channelManager.ReleaseChannel(session.ChannelNumber))
        {
            result.Success = false;
            result.ErrorMessage = "Failed to release channel.";
            return result;
        }

        result.Success = true;
        result.SessionId = session.SessionId;
        result.ChannelNumber = session.ChannelNumber;
        return result;
    }

    aes67::ipc::IpcResponse Application::HandleRequest(const aes67::ipc::IpcRequest& request)
    {
        aes67::ipc::IpcResponse response;

        switch (request.CommandType)
        {
        case aes67::ipc::IpcCommandType::PreparePlayback:
        {
            aes67::commands::PreparePlaybackCommand command;
            command.SourcePath = request.SourcePath;

            aes67::domain::PreparePlaybackResult result = Execute(command);
            response.Success = result.Success;
            response.SessionId = result.SessionId;
            response.ChannelNumber = result.ChannelNumber;
            response.ErrorMessage = result.ErrorMessage;
            return response;
        }

        case aes67::ipc::IpcCommandType::StartPlayback:
        {
            aes67::commands::StartPlaybackCommand command;
            command.SessionId = request.SessionId;

            aes67::domain::StartPlaybackResult result = Execute(command);
            response.Success = result.Success;
            response.SessionId = result.SessionId;
            response.ChannelNumber = result.ChannelNumber;
            response.ErrorMessage = result.ErrorMessage;
            return response;
        }

        case aes67::ipc::IpcCommandType::FinishPlayback:
        {
            aes67::commands::FinishPlaybackCommand command;
            command.SessionId = request.SessionId;

            aes67::domain::FinishPlaybackResult result = Execute(command);
            response.Success = result.Success;
            response.SessionId = result.SessionId;
            response.ChannelNumber = result.ChannelNumber;
            response.ErrorMessage = result.ErrorMessage;
            return response;
        }

        default:
            response.Success = false;
            response.ErrorMessage = "Unknown IPC command.";
            return response;
        }
    }

    void Application::RunSelfTest()
    {
        aes67::infra::Logger::Info("Running in-memory playback self-test...");

        aes67::ipc::IpcRequest prepareRequest;
        prepareRequest.CommandType = aes67::ipc::IpcCommandType::PreparePlayback;
        prepareRequest.SourcePath = "demo-audio.wav";

        aes67::ipc::IpcResponse prepareResponse = HandleRequest(prepareRequest);

        if (!prepareResponse.Success)
        {
            std::string errorMessage = "Prepare playback failed: " + prepareResponse.ErrorMessage;
            aes67::infra::Logger::Error(errorMessage.c_str());
            return;
        }

        std::string preparedMessage =
            "Prepared playback session " + prepareResponse.SessionId +
            " on channel " + std::to_string(prepareResponse.ChannelNumber);
        aes67::infra::Logger::Info(preparedMessage.c_str());

        aes67::ipc::IpcRequest startRequest;
        startRequest.CommandType = aes67::ipc::IpcCommandType::StartPlayback;
        startRequest.SessionId = prepareResponse.SessionId;

        aes67::ipc::IpcResponse startResponse = HandleRequest(startRequest);

        if (!startResponse.Success)
        {
            std::string startErrorMessage = "Start playback failed: " + startResponse.ErrorMessage;
            aes67::infra::Logger::Error(startErrorMessage.c_str());
            return;
        }

        std::string startedMessage =
            "Started playback session " + startResponse.SessionId +
            " on channel " + std::to_string(startResponse.ChannelNumber);
        aes67::infra::Logger::Info(startedMessage.c_str());

        aes67::domain::ChannelInfo currentChannel;
        if (_channelManager.TryGetChannel(startResponse.ChannelNumber, currentChannel))
        {
            if (currentChannel.State == aes67::domain::ChannelState::Playing)
            {
                std::string playingChannelMessage =
                    "Channel " + std::to_string(currentChannel.ChannelNumber) + " is now in playing state.";
                aes67::infra::Logger::Info(playingChannelMessage.c_str());
            }
            else
            {
                aes67::infra::Logger::Error("Started session channel is not in playing state as expected.");
            }
        }
        else
        {
            aes67::infra::Logger::Error("Failed to retrieve playing channel.");
        }

        aes67::ipc::IpcRequest finishRequest;
        finishRequest.CommandType = aes67::ipc::IpcCommandType::FinishPlayback;
        finishRequest.SessionId = startResponse.SessionId;

        aes67::ipc::IpcResponse finishResponse = HandleRequest(finishRequest);

        if (!finishResponse.Success)
        {
            std::string finishErrorMessage = "Finish playback failed: " + finishResponse.ErrorMessage;
            aes67::infra::Logger::Error(finishErrorMessage.c_str());
            return;
        }

        std::string finishedMessage =
            "Finished playback session " + finishResponse.SessionId +
            " on channel " + std::to_string(finishResponse.ChannelNumber);
        aes67::infra::Logger::Info(finishedMessage.c_str());

        aes67::domain::ChannelInfo releasedChannel;
        if (_channelManager.TryGetChannel(finishResponse.ChannelNumber, releasedChannel))
        {
            if (releasedChannel.State == aes67::domain::ChannelState::Free)
            {
                std::string freeChannelMessage =
                    "Channel " + std::to_string(releasedChannel.ChannelNumber) + " is now free.";
                aes67::infra::Logger::Info(freeChannelMessage.c_str());
            }
            else
            {
                aes67::infra::Logger::Error("Finished session channel is not free as expected.");
            }
        }
        else
        {
            aes67::infra::Logger::Error("Failed to retrieve released channel.");
        }
    }

    int Application::Run()
    {
        aes67::infra::Logger::Info("AES67 Service starting...");

        if (!ValidateConfig())
        {
            aes67::infra::Logger::Error("Service startup failed due to invalid configuration.");
            return -1;
        }

        std::string channelMessage = "Configured channels: " + std::to_string(_config.ChannelCount);
        aes67::infra::Logger::Info(channelMessage.c_str());

        std::string socketMessage = "IPC socket path: " + _config.IpcSocketPath;
        aes67::infra::Logger::Info(socketMessage.c_str());

        std::string createdChannelsMessage = "Channel manager initialized with " +
            std::to_string(_channelManager.GetChannels().size()) +
            " channels.";
        aes67::infra::Logger::Info(createdChannelsMessage.c_str());

        RunSelfTest();

        aes67::infra::Logger::Info("Service startup completed.");

        return 0;
    }
}