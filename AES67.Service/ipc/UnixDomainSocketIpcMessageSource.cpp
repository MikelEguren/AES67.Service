#include "ipc/UnixDomainSocketIpcMessageSource.hpp"

#if defined(__linux__)
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#endif

namespace aes67::ipc
{
    UnixDomainSocketIpcMessageSource::UnixDomainSocketIpcMessageSource(const std::string& socketPath)
        : _socketPath(socketPath)
    {}

    IpcReceiveResult UnixDomainSocketIpcMessageSource::ReceiveMessages()
    {
        IpcReceiveResult result;

#if defined(__linux__)
        int serverSocket = -1;
        int clientSocket = -1;

        serverSocket = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (serverSocket < 0)
        {
            result.Success = false;
            result.ErrorMessage = std::string("Failed to create unix domain socket: ") + std::strerror(errno);
            return result;
        }

        ::unlink(_socketPath.c_str());

        sockaddr_un address{};
        address.sun_family = AF_UNIX;

        if (_socketPath.size() >= sizeof(address.sun_path))
        {
            result.Success = false;
            result.ErrorMessage = "IPC socket path is too long for unix domain socket.";

            ::close(serverSocket);
            return result;
        }

        std::strncpy(address.sun_path, _socketPath.c_str(), sizeof(address.sun_path) - 1);
        address.sun_path[sizeof(address.sun_path) - 1] = '\0';

        if (::bind(serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
        {
            result.Success = false;
            result.ErrorMessage = std::string("Failed to bind unix domain socket: ") + std::strerror(errno);

            ::close(serverSocket);
            return result;
        }

        if (::listen(serverSocket, 1) < 0)
        {
            result.Success = false;
            result.ErrorMessage = std::string("Failed to listen on unix domain socket: ") + std::strerror(errno);

            ::close(serverSocket);
            ::unlink(_socketPath.c_str());
            return result;
        }

        clientSocket = ::accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0)
        {
            result.Success = false;
            result.ErrorMessage = std::string("Failed to accept unix domain socket connection: ") + std::strerror(errno);

            ::close(serverSocket);
            ::unlink(_socketPath.c_str());
            return result;
        }

        char buffer[1024];
        const ssize_t bytesRead = ::read(clientSocket, buffer, sizeof(buffer) - 1);
        if (bytesRead < 0)
        {
            result.Success = false;
            result.ErrorMessage = std::string("Failed to read from unix domain socket: ") + std::strerror(errno);

            ::close(clientSocket);
            ::close(serverSocket);
            ::unlink(_socketPath.c_str());
            return result;
        }

        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            result.Messages.push_back(std::string(buffer));
        }

        result.Success = true;

        ::close(clientSocket);
        ::close(serverSocket);
        ::unlink(_socketPath.c_str());

#else
        result.Success = false;
        result.ErrorMessage = "Unix domain socket message source is only supported on Linux.";
#endif

        return result;
    }
}
