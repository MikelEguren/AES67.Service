#include "ipc/UnixDomainSocketIpcMessageSource.hpp"

namespace aes67::ipc
{
    UnixDomainSocketIpcMessageSource::UnixDomainSocketIpcMessageSource(const std::string& socketPath)
        : _socketPath(socketPath)
    {}

    IpcReceiveResult UnixDomainSocketIpcMessageSource::ReceiveMessages()
    {
        IpcReceiveResult result;

#if defined(__linux__)
        result.Success = false;
        result.ErrorMessage = "Unix domain socket message source is not implemented yet on Linux.";
#else
        result.Success = false;
        result.ErrorMessage = "Unix domain socket message source is only supported on Linux.";
#endif

        return result;
    }
}
