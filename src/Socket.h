#ifndef MYSOCKET_H
#define MYSOCKET_H

#include <cstdlib> 
#include <cstring>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <exception>
#include <stdio.h>

class Socket{
 public:
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname;
  const char *port;

  Socket(){
    socket_fd = 0;
    host_info_list = NULL;
    hostname = NULL;
    port = NULL;
  }

  ~Socket(){
    free(host_info_list);
    close(socket_fd);
  }

  int newServerSocket();//getaddrinfo-socket-bind-listen
  int newClientSocket();//getaddrinfo-socket, but do not bind and listen
  int connectSocket();//connect()
  int acceptSocket();//accept()
};
#endif
