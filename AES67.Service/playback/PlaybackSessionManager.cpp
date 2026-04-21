#include "playback/PlaybackSessionManager.hpp"

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

    bool PlaybackSessionManager::MarkSessionReady(const std::string& sessionId)
    {
        for (auto& session : _sessions)
        {
            if (session.SessionId == sessionId)
            {
                session.State = aes67::domain::PlaybackSessionState::Ready;
                return true;
            }
        }

        return false;
    }

    bool PlaybackSessionManager::MarkSessionPlaying(const std::string& sessionId)
    {
        for (auto& session : _sessions)
        {
            if (session.SessionId == sessionId)
            {
                if (session.State != aes67::domain::PlaybackSessionState::Ready)
                {
                    return false;
                }

                session.State = aes67::domain::PlaybackSessionState::Playing;
                return true;
            }
        }

        return false;
    }

    bool PlaybackSessionManager::MarkSessionFinished(const std::string& sessionId)
    {
        for (auto& session : _sessions)
        {
            if (session.SessionId == sessionId)
            {
                if (session.State != aes67::domain::PlaybackSessionState::Playing)
                {
                    return false;
                }

                session.State = aes67::domain::PlaybackSessionState::Finished;
                return true;
            }
        }

        return false;
    }

    bool PlaybackSessionManager::TryGetSession(const std::string& sessionId, aes67::domain::PlaybackSession& session) const
    {
        for (const auto& currentSession : _sessions)
        {
            if (currentSession.SessionId == sessionId)
            {
                session = currentSession;
                return true;
            }
        }

        return false;
    }

    const std::vector<aes67::domain::PlaybackSession>& PlaybackSessionManager::GetSessions() const
    {
        return _sessions;
    }
}