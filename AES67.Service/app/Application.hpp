#pragma once

#include <string>

#include "commands/FinishPlaybackCommand.hpp"
#include "commands/PreparePlaybackCommand.hpp"
#include "commands/StartPlaybackCommand.hpp"
#include "config/ServiceConfig.hpp"
#include "domain/FinishPlaybackResult.hpp"
#include "domain/PreparePlaybackResult.hpp"
#include "domain/StartPlaybackResult.hpp"
#include "engine/ChannelManager.hpp"
#include "ipc/IpcRequest.hpp"
#include "ipc/IpcResponse.hpp"
#include "playback/PlaybackSessionManager.hpp"

namespace aes67::app
{
    class Application
    {
    public:
        Application();
        int Run();

        aes67::domain::PreparePlaybackResult Execute(const aes67::commands::PreparePlaybackCommand& command);
        aes67::domain::StartPlaybackResult Execute(const aes67::commands::StartPlaybackCommand& command);
        aes67::domain::FinishPlaybackResult Execute(const aes67::commands::FinishPlaybackCommand& command);

        aes67::ipc::IpcResponse HandleRequest(const aes67::ipc::IpcRequest& request);

    private:
        aes67::config::ServiceConfig _config;
        aes67::engine::ChannelManager _channelManager;
        aes67::playback::PlaybackSessionManager _playbackSessionManager;

        bool ValidateConfig();
        aes67::domain::PreparePlaybackResult PreparePlayback(const std::string& sourcePath);
        aes67::domain::StartPlaybackResult StartPlayback(const std::string& sessionId);
        aes67::domain::FinishPlaybackResult FinishPlayback(const std::string& sessionId);
        void RunSelfTest();
    };
}