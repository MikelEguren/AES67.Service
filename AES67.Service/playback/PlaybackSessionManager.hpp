#pragma once

#include <string>
#include <vector>

#include "domain/PlaybackSession.hpp"

namespace aes67::playback
{
    class PlaybackSessionManager
    {
    public:
        PlaybackSessionManager();

        aes67::domain::PlaybackSession CreateSession(const std::string& sourcePath, int channelNumber);
        bool MarkSessionReady(const std::string& sessionId);
        bool MarkSessionPlaying(const std::string& sessionId);
        bool MarkSessionFinished(const std::string& sessionId);
        bool TryGetSession(const std::string& sessionId, aes67::domain::PlaybackSession& session) const;
        const std::vector<aes67::domain::PlaybackSession>& GetSessions() const;

    private:
        int _nextSessionId;
        std::vector<aes67::domain::PlaybackSession> _sessions;
    };
}