#include "Application.hpp"

#include <string>

#include "infra/Logger.hpp"
#include "playback/PlaybackSessionManager.hpp"

namespace aes67::app
{
    Application::Application()
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

        aes67::engine::ChannelManager channelManager(_config.ChannelCount);

        std::string createdChannelsMessage = "Channel manager initialized with " +
            std::to_string(channelManager.GetChannels().size()) +
            " channels.";
        aes67::infra::Logger::Info(createdChannelsMessage.c_str());

        aes67::domain::ChannelInfo reservedChannel;
        if (channelManager.TryReserveNextFreeChannel(reservedChannel))
        {
            std::string reservedMessage = "Reserved channel: " + std::to_string(reservedChannel.ChannelNumber);
            aes67::infra::Logger::Info(reservedMessage.c_str());

            aes67::playback::PlaybackSessionManager playbackSessionManager;
            aes67::domain::PlaybackSession session =
                playbackSessionManager.CreateSession("demo-audio.wav", reservedChannel.ChannelNumber);

            std::string sessionMessage =
                "Created session " + session.SessionId +
                " for channel " + std::to_string(session.ChannelNumber) +
                " with source " + session.SourcePath;

            aes67::infra::Logger::Info(sessionMessage.c_str());

            if (channelManager.ReleaseChannel(reservedChannel.ChannelNumber))
            {
                std::string releasedMessage = "Released channel: " + std::to_string(reservedChannel.ChannelNumber);
                aes67::infra::Logger::Info(releasedMessage.c_str());
            }
            else
            {
                aes67::infra::Logger::Error("Failed to release reserved channel.");
            }
        }
        else
        {
            aes67::infra::Logger::Error("No free channel available.");
        }

        aes67::infra::Logger::Info("Service startup completed.");

        return 0;
    }
}