#pragma once

#include <vector>

#include "domain/ChannelInfo.hpp"

namespace aes67::engine
{
    class ChannelManager
    {
    public:
        explicit ChannelManager(int channelCount);

        const std::vector<aes67::domain::ChannelInfo>& GetChannels() const;

        bool TryReserveNextFreeChannel(aes67::domain::ChannelInfo& reservedChannel);
        bool ReleaseChannel(int channelNumber);

    private:
        std::vector<aes67::domain::ChannelInfo> _channels;
    };
}