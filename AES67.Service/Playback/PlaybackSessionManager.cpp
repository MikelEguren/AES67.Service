#include "PlaybackSessionManager.hpp"

namespace aes67::playback
{
    PlaybackSessionManager::PlaybackSessionManager()
        : _nextSessionId(1)
    {}

    aes67::domain::PlaybackSession PlaybackSessionManager::CreateSession(const std::string& sourcePath, int channelNumber)
    {
        aes67::domain::PlaybackSession session;
        session.SessionId = "S-" + std::to_string(_nextSessionId++);
        session.SourcePath = sourcePath;
        session.ChannelNumber = channelNumber;
        session.State = aes67::domain::PlaybackSessionState::Created;

        _sessions.push_back(session);
        return session;
    }

    const std::vector<aes67::domain::PlaybackSession>& PlaybackSessionManager::GetSessions() const
    {
        return _sessions;
    }
}