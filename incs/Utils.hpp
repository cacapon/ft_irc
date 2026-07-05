#pragma once

#include <sys/socket.h>

#include <stdexcept>

#ifdef __APPLE__
#include <fcntl.h>
#endif

int ft_socket(int domain, int type, int protocol);
