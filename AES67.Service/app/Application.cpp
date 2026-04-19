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

    void Application::RunSelfTest()
    {
        aes67::infra::Logger::Info("Running in-memory playback self-test...");

        aes67::commands::PreparePlaybackCommand prepareCommand;
        prepareCommand.SourcePath = "demo-audio.wav";

        aes67::domain::PreparePlaybackResult prepareResult = Execute(prepareCommand);

        if (!prepareResult.Success)
        {
            std::string errorMessage = "Prepare playback failed: " + prepareResult.ErrorMessage;
            aes67::infra::Logger::Error(errorMessage.c_str());
            return;
        }

        std::string preparedMessage =
            "Prepared playback session " + prepareResult.SessionId +
            " on channel " + std::to_string(prepareResult.ChannelNumber);
        aes67::infra::Logger::Info(preparedMessage.c_str());

        aes67::commands::StartPlaybackCommand startCommand;
        startCommand.SessionId = prepareResult.SessionId;

        aes67::domain::StartPlaybackResult startResult = Execute(startCommand);

        if (!startResult.Success)
        {
            std::string startErrorMessage = "Start playback failed: " + startResult.ErrorMessage;
            aes67::infra::Logger::Error(startErrorMessage.c_str());
            return;
        }

        std::string startedMessage =
            "Started playback session " + startResult.SessionId +
            " on channel " + std::to_string(startResult.ChannelNumber);
        aes67::infra::Logger::Info(startedMessage.c_str());

        aes67::domain::ChannelInfo currentChannel;
        if (_channelManager.TryGetChannel(startResult.ChannelNumber, currentChannel))
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

        aes67::commands::FinishPlaybackCommand finishCommand;
        finishCommand.SessionId = startResult.SessionId;

        aes67::domain::FinishPlaybackResult finishResult = Execute(finishCommand);

        if (!finishResult.Success)
        {
            std::string finishErrorMessage = "Finish playback failed: " + finishResult.ErrorMessage;
            aes67::infra::Logger::Error(finishErrorMessage.c_str());
            return;
        }

        std::string finishedMessage =
            "Finished playback session " + finishResult.SessionId +
            " on channel " + std::to_string(finishResult.ChannelNumber);
        aes67::infra::Logger::Info(finishedMessage.c_str());

        aes67::domain::ChannelInfo releasedChannel;
        if (_channelManager.TryGetChannel(finishResult.ChannelNumber, releasedChannel))
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