#pragma once

#include <string>
#include <vector>

#include "ipc/IIpcMessageSource.hpp"

namespace aes67::ipc
{
    class UnixDomainSocketIpcMessageSource : public IIpcMessageSource
    {
    public:
        explicit UnixDomainSocketIpcMessageSource(const std::string& socketPath);

        std::vector<std::string> ReceiveMessages() override;

    private:
        std::string _socketPath;
    };
}
