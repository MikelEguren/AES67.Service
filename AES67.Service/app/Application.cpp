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

        aes67::domain::PreparePlaybackResult prepareResult = PreparePlayback("demo-audio.wav");

        if (prepareResult.Success)
        {
            std::string preparedMessage =
                "Prepared playback session " + prepareResult.SessionId +
                " on channel " + std::to_string(prepareResult.ChannelNumber);
            aes67::infra::Logger::Info(preparedMessage.c_str());

            aes67::domain::ChannelInfo currentChannel;
            if (_channelManager.TryGetChannel(prepareResult.ChannelNumber, currentChannel))
            {
                if (currentChannel.State == aes67::domain::ChannelState::Reserved)
                {
                    std::string stillReservedMessage =
                        "Channel " + std::to_string(currentChannel.ChannelNumber) + " remains reserved for ready session.";
                    aes67::infra::Logger::Info(stillReservedMessage.c_str());
                }
                else
                {
                    aes67::infra::Logger::Error("Prepared session channel is not reserved as expected.");
                }
            }
            else
            {
                aes67::infra::Logger::Error("Failed to retrieve reserved channel.");
            }
        }
        else
        {
            std::string errorMessage = "Prepare playback failed: " + prepareResult.ErrorMessage;
            aes67::infra::Logger::Error(errorMessage.c_str());
        }

        aes67::infra::Logger::Info("Service startup completed.");

        return 0;
    }
}