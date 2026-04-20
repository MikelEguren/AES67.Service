#pragma once

#include <string>
#include <vector>

#include "ipc/IIpcMessageSource.hpp"

namespace aes67::ipc
{
    class InMemoryIpcMessageSource : public IIpcMessageSource
    {
    public:
        InMemoryIpcMessageSource();

        std::vector<std::string> ReceiveMessages() override;
    };
}
