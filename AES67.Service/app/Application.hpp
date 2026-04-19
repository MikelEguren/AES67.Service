#pragma once

#include "config/ServiceConfig.hpp"
#include "engine/ChannelManager.hpp"
#include "playback/PlaybackSessionManager.hpp"

namespace aes67::app
{
    class Application
    {
    public:
        Application();
        int Run();

    private:
        aes67::config::ServiceConfig _config;
        aes67::engine::ChannelManager _channelManager;
        aes67::playback::PlaybackSessionManager _playbackSessionManager;

        bool ValidateConfig();
    };
}