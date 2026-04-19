#pragma once

#include <string>

#include "config/ServiceConfig.hpp"
#include "domain/PreparePlaybackResult.hpp"
#include "domain/StartPlaybackResult.hpp"
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
        aes67::domain::PreparePlaybackResult PreparePlayback(const std::string& sourcePath);
        aes67::domain::StartPlaybackResult StartPlayback(const std::string& sessionId);
    };
}