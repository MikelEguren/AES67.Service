#include "ipc/UnixDomainSocketIpcMessageSource.hpp"

namespace aes67::ipc
{
    UnixDomainSocketIpcMessageSource::UnixDomainSocketIpcMessageSource(const std::string& socketPath)
        : _socketPath(socketPath)
    {}

    IpcReceiveResult UnixDomainSocketIpcMessageSource::ReceiveMessages()
    {
        IpcReceiveResult result;
        result.Success = true;
        return result;
    }
}
