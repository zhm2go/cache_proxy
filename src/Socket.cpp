#include "Socket.h"

int Socket::newServerSocket(){
  int status = 0;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;
  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    std::cerr << "no-id: Error \"cannot get address info for host"  << "  (" << hostname << "," << port << ")\""<< std::endl;
    return -1;
  }
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    std::cerr << "no-id: Error \" cannot create socket" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")\"" << std::endl;
    return -1;
  }
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::cerr << "no-id: Error \" cannot bind socket"  << "  (" << hostname << "," << port << ")\""<< std::endl;
    
    return -1;
  }
  status = listen(socket_fd, 100);
  if (status == -1) {
    std::cerr << "no-id: Error \" cannot listen on socket"  << "  (" << hostname << "," << port << ")\"" << std::endl; 
    return -1;
  }
  return status;
}

int Socket::newClientSocket(){
  int status = 0;
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
	host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    std::cerr << "no-id: Error \"cannot get address info for host"  << "  (" << hostname << "," << port << ")\""<< std::endl;
    return -1;
  }
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    std::cerr << "no-id: Error \" cannot create socket"  << "  (" << hostname << "," << port << ")\""<< std::endl;
    return -1;
  }
  return socket_fd;
}

int Socket::connectSocket(){
  int status = 0;
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::cerr << "no-id: Error \" cannot connect to socket"  << "  (" << hostname << "," << port << ")\""<< std::endl;
    return -1;
  }
  return status;
}

int Socket::acceptSocket(){
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);
  int client_connection_fd;
  client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    std::cerr << "no-id: Error \" cannot accept connection on socket\"" << std::endl;
    return -1;
  }
  return client_connection_fd;
}


