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

        aes67::domain::ChannelInfo reservedChannel;
        if (_channelManager.TryReserveNextFreeChannel(reservedChannel))
        {
            std::string reservedMessage = "Reserved channel: " + std::to_string(reservedChannel.ChannelNumber);
            aes67::infra::Logger::Info(reservedMessage.c_str());

            aes67::domain::PlaybackSession session =
                _playbackSessionManager.CreateSession("demo-audio.wav", reservedChannel.ChannelNumber);

            std::string createdSessionMessage =
                "Created session " + session.SessionId +
                " for channel " + std::to_string(session.ChannelNumber) +
                " with source " + session.SourcePath;
            aes67::infra::Logger::Info(createdSessionMessage.c_str());

            if (_playbackSessionManager.MarkSessionReady(session.SessionId))
            {
                std::string readyMessage =
                    "Session " + session.SessionId +
                    " is ready on channel " + std::to_string(session.ChannelNumber);
                aes67::infra::Logger::Info(readyMessage.c_str());

                aes67::domain::ChannelInfo currentChannel;
                if (_channelManager.TryGetChannel(session.ChannelNumber, currentChannel))
                {
                    if (currentChannel.State == aes67::domain::ChannelState::Reserved)
                    {
                        std::string stillReservedMessage =
                            "Channel " + std::to_string(currentChannel.ChannelNumber) + " remains reserved for ready session.";
                        aes67::infra::Logger::Info(stillReservedMessage.c_str());
                    }
                    else
                    {
                        aes67::infra::Logger::Error("Ready session channel is not reserved as expected.");
                    }
                }
                else
                {
                    aes67::infra::Logger::Error("Failed to retrieve reserved channel.");
                }
            }
            else
            {
                aes67::infra::Logger::Error("Failed to mark session as ready.");
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