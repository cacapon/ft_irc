#include "Utils.hpp"

int ft_socket(int domain, int type, int protocol)
{
#ifdef __APPLE__
    int fd = socket(domain, type, protocol);
    if (fd < 0)
        throw std::runtime_error("socket failed");
    fcntl(fd, F_SETFL, O_NONBLOCK);
    return fd;
#else
    int fd = socket(domain, type | SOCK_NONBLOCK, protocol);
    if (fd < 0)
        throw std::runtime_error("socket failed");
    return fd;
#endif
}