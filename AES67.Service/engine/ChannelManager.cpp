#include "ChannelManager.hpp"

namespace aes67::engine
{
    ChannelManager::ChannelManager(int channelCount)
    {
        _channels.reserve(channelCount);

        for (int i = 1; i <= channelCount; ++i)
        {
            aes67::domain::ChannelInfo channel;
            channel.ChannelNumber = i;
            channel.State = aes67::domain::ChannelState::Free;

            _channels.push_back(channel);
        }
    }

    const std::vector<aes67::domain::ChannelInfo>& ChannelManager::GetChannels() const
    {
        return _channels;
    }
}