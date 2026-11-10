
#define _GNU_SOURCE
#include <dlfcn.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>

static int (*real_bind)(int, const struct sockaddr *, socklen_t) = NULL;

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  if (!real_bind) {
    real_bind = (int (*)(int, const struct sockaddr *, socklen_t))dlsym(
        RTLD_NEXT, "bind");
  }
  if (addr && addr->sa_family == AF_INET) {
    struct sockaddr_in in;
    memcpy(&in, addr, sizeof(in));
    in.sin_addr.s_addr = htonl(INADDR_ANY);
    return real_bind(sockfd, (struct sockaddr *)&in, addrlen);
  }
  return real_bind(sockfd, addr, addrlen);
}
