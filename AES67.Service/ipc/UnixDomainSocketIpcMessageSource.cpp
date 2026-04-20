#include "ipc/UnixDomainSocketIpcMessageSource.hpp"

namespace aes67::ipc
{
    UnixDomainSocketIpcMessageSource::UnixDomainSocketIpcMessageSource(const std::string& socketPath)
        : _socketPath(socketPath)
    {}

    std::vector<std::string> UnixDomainSocketIpcMessageSource::ReceiveMessages()
    {
        return {};
    }
}
