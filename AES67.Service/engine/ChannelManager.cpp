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

    bool ChannelManager::TryReserveNextFreeChannel(aes67::domain::ChannelInfo& reservedChannel)
    {
        for (auto& channel : _channels)
        {
            if (channel.State == aes67::domain::ChannelState::Free)
            {
                channel.State = aes67::domain::ChannelState::Reserved;
                reservedChannel = channel;
                return true;
            }
        }

        return false;
    }

    bool ChannelManager::ReleaseChannel(int channelNumber)
    {
        for (auto& channel : _channels)
        {
            if (channel.ChannelNumber == channelNumber)
            {
                channel.State = aes67::domain::ChannelState::Free;
                return true;
            }
        }

        return false;
    }
}