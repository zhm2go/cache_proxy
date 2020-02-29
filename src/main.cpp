#include "Socket.h"
#include "Parser.h"
#include "Cache.h"
#include <thread>
#include <ctime>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>
#include <fstream>

#define LEN_MSG 65536

std::mutex mutex0;
int UID = 0;
std::ofstream proxylog("../proxy.log");

int tunnel(Parser request, int send_fd, int recv_fd){
  int len;
  int status;
  std::vector<char> buffer(LEN_MSG, 0);
  len = recv(send_fd, &buffer.data()[0], LEN_MSG, 0);
  if(len == 0){                                                 
    //close tunnel
    std::cout << request.uid << ": Tunnel closed" << std::endl;
    proxylog << request.uid << ": Tunnel closed" << std::endl; 
	  return 1;
  }                                              
  if(len == -1){  
    //recv failed
    std::cout << request.uid << ": ERROR \"recv() failed at tunnel\"" << std::endl;
    proxylog << request.uid << ": ERROR \"recv() failed at tunnel\"" << std::endl;                                          
    return -1;               
	}                                              

  status = send(recv_fd, &buffer.data()[0], len, 0);
  if(status == -1){
    std::cout << request.uid << ": ERROR \"send() failed at tunnel\"" << std::endl;
    proxylog << request.uid << ": ERROR \"send() failed at tunnel\"" << std::endl;  
    return -2;
  }
  return 0;
}

int CONNECTMethod(Parser request, int client_fd, int server_fd){
  int status;
  std::string ok200 = "200 OK";
  send(client_fd, ok200.c_str(), sizeof(ok200), 0);
  fd_set readfds;
  for(;;){
    FD_ZERO(&readfds);
    int fdmax = client_fd;
    if(server_fd > client_fd){fdmax = server_fd;}
    FD_SET(client_fd, &readfds);
    FD_SET(server_fd, &readfds);
    select(fdmax + 1, &readfds, NULL, NULL, NULL);
    if(FD_ISSET(client_fd, &readfds)){
      
      status = tunnel(request, client_fd, server_fd);
      if(status == 1){
        //tunnel closed
        std::cout << request.uid << ": Requesting \"" << request.first_line << "\" from " << request.request_h.host <<std::endl;
        proxylog << request.uid << ": Requesting \"" << request.first_line << "\" from " << request.request_h.host <<std::endl;
        return 1;
      }
    }
    else if(FD_ISSET(server_fd, &readfds)){
      
      status = tunnel(request, server_fd, client_fd);
      
      if(status == 1){
        return 1;
        }
    }
  }
}

int POSTMethod(Parser request, int client_fd, int server_fd, std::vector<char> msg, int len_msg){
  
  std::cout << request.uid << ": Requesting \"" << request.first_line << "\" from " << request.request_h.host <<std::endl;
  proxylog << request.uid << ": Requesting \"" << request.first_line << "\" from " << request.request_h.host <<std::endl;

  int status = send(server_fd, &msg.data()[0], len_msg, 0);
  if(status == -1){
    std::cout << request.uid << ": ERROR \"send to server failed\"" << std::endl;
    proxylog << request.uid << ": ERROR \"send to server failed\"" << std::endl;
    return -1;
  }
  std::vector<char> Info(LEN_MSG, 0);
  int len = recv(server_fd, &Info.data()[0], LEN_MSG, 0);
  if (len < 0){
    std::cout << request.uid << ": Received " << "HTTP/1.1 502 Bad Gateway" << std::endl;
    proxylog << request.uid << ": Received " << "HTTP/1.1 502 Bad Gateway" << std::endl;
    std::string badgateway = "HTTP/1.1 502 Bad Gateway";
    status = send(client_fd, badgateway.c_str(), sizeof(badgateway), 0);
     if (status < 0 ){
     std::cout << request.uid << ": ERROR \"send bad gateway to client failed\"" <<std::endl;
     proxylog << request.uid << ": ERROR \"send bad gateway to client failed\"" <<std::endl;
     }
   return -1;
  }
  std::string resp = "";
  for(int i = 0; i < len; i++){
    resp += Info[i];
  }
  Parser response;
  response.uid = request.uid;
  status = response.parseMessage(resp);
  if(status == -1){
    std::cout << request.uid << ": ERROR \"parseMessage failed\"" << std::endl;
    proxylog << request.uid << ": ERROR \"parseMessage failed\"" << std::endl;
    return -1;
  }
  response.setPair();
  if(response.pair.find("Content-Length") != response.pair.end()){
    std::string length_body = response.pair.at("Content-Length");
    int len_body = atoi(length_body.c_str());
    int len_header = resp.find("\r\n\r\n");
    Info.resize(len_header + len_body + 4,0);
    if(len != len_header + len_body + 4){
      while(len != len_header + len_body + 4){
	int extra_len = recv(server_fd, &Info.data()[len], LEN_MSG, 0);
	len += extra_len;
      }
    }
  }
  else if(response.pair.find("Transfer-Encoding") != response.pair.end() && resp.find("0\r\n\r\n") == resp.npos){
    if(response.pair.at("Transfer-Encoding") == "chunked"){ 
      int start_chunk = 0;
      while(Info[start_chunk] != '0'){
        start_chunk = len;
        if(size_t(len) > Info.size() - 10000){
          Info.resize(Info.size() + LEN_MSG, 0);
        }
        int extra_len = recv(server_fd, &Info.data()[len], LEN_MSG, 0);
        len += extra_len;
      }
    }
  }
  std::cout << request.uid << ": Received \"" << response.first_line << "\" from " << request.request_h.host <<std::endl;
  proxylog << request.uid << ": Received \"" << response.first_line << "\" from " << request.request_h.host <<std::endl;
  
  status = send(client_fd, &Info.data()[0], len, 0);
  if(status == -1){
    std::cout << request.uid << ": ERROR \"send to client failed\"" << std::endl;
    proxylog << request.uid << ": ERROR \"send to client failed\"" << std::endl;
  }
  std::cout << request.uid << ": Responding \"" << response.first_line << "\"" <<std::endl;
  proxylog << request.uid << ": Responding \"" << response.first_line << "\"" <<std::endl;

  close(client_fd);
  close(server_fd);
  return 0;
}

Parser GETMethod(Parser request, int client_fd, int server_fd, std::vector<char> msg, int len_msg){
  
  std::cout << request.uid << ": Requesting \"" << request.first_line << "\" from " << request.request_h.host <<std::endl;
  proxylog << request.uid << ": Requesting \"" << request.first_line << "\" from " << request.request_h.host <<std::endl;
  int status;
  status = send(server_fd, &msg.data()[0], len_msg, 0);
  if(status == -1){
    std::cout << request.uid << ": ERROR \"send to server failed\"" << std::endl;
    proxylog << request.uid << ": ERROR \"send to server failed\"" << std::endl;
  }
  std::vector<char> Info(LEN_MSG, 0);
  int len = recv(server_fd, &Info.data()[0], LEN_MSG, 0);
  if (len < 0){
    std::cout << request.uid << ": Received " << "HTTP/1.1 502 Bad Gateway" << std::endl;
    proxylog << request.uid << ": Received " << "HTTP/1.1 502 Bad Gateway" << std::endl;
    std::string badgateway = "HTTP/1.1 502 Bad Gateway";
    status = send(client_fd, badgateway.c_str(), sizeof(badgateway), 0);
     if (status < 0 ){
     std::cout << request.uid << ": ERROR \"send bad gateway to client failed\"" <<std::endl;
     proxylog << request.uid << ": ERROR \"send bad gateway to client failed\"" <<std::endl;
     }
  }
  std::string resp = "";
  for(int i = 0; i < len; i++){
    resp += Info[i];
  }
  Parser response;
  response.uid = request.uid; 
  status = response.parseMessage(resp);
  if(status == -1){
    std::cout << request.uid << ": ERROR \"parseMessage failed\"" << std::endl;
    proxylog << request.uid << ": ERROR \"parseMessage failed\"" << std::endl;
  }
  response.setPair();
  if(response.pair.find("Content-Length") != response.pair.end()){
    std::string length_body = response.pair.at("Content-Length");
    int len_body = atoi(length_body.c_str());
    int len_header = resp.find("\r\n\r\n");
    Info.resize(len_header + len_body + 4,0);
    if(len != len_header + len_body + 4){
      while(len != len_header + len_body + 4){
	 int extra_len = recv(server_fd, &Info.data()[len], LEN_MSG, 0);
	      len += extra_len;
      }
    }
  }
  else if(response.pair.find("Transfer-Encoding") != response.pair.end() && resp.find("0\r\n\r\n") == resp.npos){
    if(response.pair.at("Transfer-Encoding") == "chunked"){
      int start_chunk = 0;
      while(Info[start_chunk] != '0'){
        start_chunk = len;
        if(size_t(len) > Info.size() - 10000){
          Info.resize(Info.size() + LEN_MSG, 0);
        }
        int extra_len = recv(server_fd, &Info.data()[len], LEN_MSG, 0);
        len += extra_len;
      }
    }
  }
  std::cout << request.uid << ": Received \"" << response.first_line << "\" from " << request.request_h.host <<std::endl;
  proxylog << request.uid << ": Received \"" << response.first_line << "\" from " << request.request_h.host <<std::endl;
  std::string whole_resp = "";
  for(int i = 0; i < len; i++){
    whole_resp += Info[i];
  }
  Parser whole_response;
  whole_response.uid = request.uid;
  status = whole_response.parseMessage(whole_resp);
  if(status == -1){
    std::cout << request.uid << ": ERROR \"parseMessage failed\"" << std::endl;
    proxylog << request.uid << ": ERROR \"parseMessage failed\"" << std::endl;
  }
  whole_response.setPair();
  if(whole_response.first_line.find("304") == whole_response.first_line.npos){
    status = send(client_fd, &Info.data()[0], len, 0);
    if(status == -1){
      std::cout << request.uid << ": ERROR \"send to client failed\"" << std::endl;
      proxylog << request.uid << ": ERROR \"send to client failed\"" << std::endl;
    }

    std::cout << request.uid << ": Responding \"" << response.first_line << "\"" <<std::endl;
    proxylog << request.uid << ": Responding \"" << response.first_line << "\"" <<std::endl;
    close(client_fd);
    close(server_fd);
  }
  return whole_response;
}

void checkReplaceCache(Parser request, Parser response, Cache *cache){
  if((request.cache_control.find("no-store") == request.cache_control.end()) && (response.cache_control.find("no-store") == response.cache_control.end())){                                                                                                        
        if (response.first_line.compare("HTTP/1.1 200 OK") == 0){                                                                   
          cache->replaceCache(request, response);                                                                                   
          std::string cache_info = cache->checkCacheInfo(request);                                                                  
          if ((cache_info.compare("in cache, requires validation")) == 0){                                                          
            std::cout << request.uid << ": cached, but requires re-validation " <<std::endl;                                        
            proxylog << request.uid << ": cached, but requires re-validation " <<std::endl;                                         
          }                                                                                                                         
          if ((cache_info.compare(0, 25, "in cache, but expired at ")) == 0) {                                                      
            std::cout << request.uid << ": cached, expires at " << cache_info.substr(26) <<std::endl;                               
            std::cout << request.uid << ": cached, expires at " << cache_info.substr(26) <<std::endl;                               
          }                                                                                                                         
        }                                                                                                                           
      }else{                                                                                                                       
        if(response.first_line.find("200 OK") != response.first_line.npos){                                                         
          std::cout << request.uid << ": not cacheable because cache-control: no-store" <<std::endl;                                
          proxylog << request.uid << ": not cacheable because cache-control: no-store" <<std::endl;                                 
        }                                                                                                                           
  }
}
void realMain(int client_fd, Cache *cache) {
  Parser request;
  try {
    std::lock_guard<std::mutex> lck(mutex0);
    request.uid = UID;
    UID++;
  } catch (...) {
    std::cout << request.uid << ": ERROR \"locking uid failed\"" << std::endl;
    proxylog << request.uid << ": ERROR \"locking uid failed\"" << std::endl;
    close(client_fd);
    return;
  }
  
  std::vector<char> message(LEN_MSG, 0);
  int len_msg = recv(client_fd, &message.data()[0], LEN_MSG, 0);
  if(len_msg == -1){
    std::cout << request.uid << ": ERROR \"recv() failed on client " << client_fd << "\"" <<std::endl;
    proxylog << request.uid << ": ERROR \"recv() failed on client " << client_fd << "\"" <<std::endl;
  }
  std::string str_msg = "";
  for(int i = 0; i < len_msg; i++){
    str_msg += message[i];
  } 
  if(str_msg == ""){    
    close(client_fd);
    return;
  }
  
  request.parseMessage(str_msg);
  request.parseRequestHeader();
  request.setPair();
  struct sockaddr_in client_ip;
  socklen_t len = sizeof(client_ip);
  getpeername(client_fd, (struct sockaddr *)&client_ip, &len);
  request.ip = std::string(inet_ntoa(*(struct in_addr *)&client_ip.sin_addr.s_addr));
  time_t current_time = time(0);
  tm *tm = localtime(&current_time);
  char *dt = asctime(tm);
  std::cout << request.uid << ": \"" << request.first_line << "\" from " << request.ip << " @ " << std::string(dt) <<std::endl;
  proxylog << request.uid << ": \"" << request.first_line << "\" from " << request.ip << " @ " << std::string(dt) <<std::endl;
  
  Socket server_socket;
  server_socket.hostname = request.request_h.host.c_str();
  server_socket.port = request.request_h.port.c_str();
  int status = server_socket.newClientSocket();
  if (status < 0){
    std::cout << request.uid << ": ERROR \"creating client socket failed\"" <<std::endl;
    proxylog << request.uid << ": ERROR \"creating client socket failed\"" <<std::endl;
    std::cout << request.uid << ": Responding " << "HTTP/1.1 400 Bad Request" << std::endl;
    proxylog << request.uid << ": Responding " << "HTTP/1.1 400 Bad Request" << std::endl;
    std::string badrequst = "HTTP/1.1 400 Bad Request";
    status = send(client_fd, badrequst.c_str(), sizeof(badrequst), 0);
   if (status < 0 ){
     std::cout << request.uid << ": ERROR \"send bad request to client failed\"" <<std::endl;
     proxylog << request.uid << ": ERROR \"send bad request to client failed\"" <<std::endl;
   }
    close(client_fd);
    return;
  }
  status = server_socket.connectSocket();
  if (status < 0){
    std::cout << request.uid << ": ERROR \"connecting client socket failed\"" <<std::endl;
    proxylog << request.uid << ": ERROR \"connecting client socket failed\"" <<std::endl;
    close(client_fd);
    return;
  }
  if (request.request_h.method == "CONNECT") {
    status = CONNECTMethod(request, client_fd, server_socket.socket_fd);
    if(status == 1){
      close(client_fd);
      return;
    }
    else {
      std::cout << request.uid << ": ERROR \"CONNECT failed\"" <<std::endl;
      proxylog << request.uid << ": ERROR \"CONNECT failed\"" <<std::endl;
      close(client_fd);
      return;
    }
  }
  if(request.request_h.method == "GET"){
    Parser response;
    response.uid = request.uid;
    bool inCache = cache->isRequestInCache(request);
    
    if (inCache == false){
    //cache not found, directly request server and update cache
      std::cout << request.uid << ": not in cache" <<std::endl;
      proxylog << request.uid << ": not in cache" <<std::endl;
      response = GETMethod(request, client_fd, server_socket.socket_fd, message, len_msg);
      //add to cache
      std::cout << request.uid << ": NOTE cache's first line: " << response.first_line<<std::endl;
      checkReplaceCache(request, response, cache);
      return;  
    } else {
      //cache found, but first check its validity
      std::string cache_info = cache->checkCacheInfo(request);
      std::cout << request.uid << ": " << cache_info <<std::endl;
      proxylog << request.uid << ": " << cache_info <<std::endl;
      if (cache_info=="in cache, valid"){
        //cache is valid, get cache and send to client
	Parser cache_response = cache->getCache(request.request_h.url);
	std::vector<char> MsgInCache = cache_response.ParserToVector();
      	status = send(client_fd, &MsgInCache.data()[0], MsgInCache.size(), 0);
        if (status < 0 ){
          std::cout << request.uid << ": ERROR \"send cache to client failed\"" <<std::endl;
          proxylog << request.uid << ": ERROR \"send cache to client failed\"" <<std::endl;
        }
          std::cout << request.uid << ": Responding \"" << cache_response.first_line << "\"" <<std::endl;
          proxylog << request.uid << ": Responding \"" << cache_response.first_line << "\"" <<std::endl;
          
      }else if(cache_info == "in cache, requires validation"){//found cache that need to be re-validated
	Parser cache_response = cache->getCache(request.request_h.url);
	if(response.pair.find("ETag") != response.pair.end()){
	  std::string ETag = response.pair.find("ETag")->second;
	  int size_b = request.body.size();
	  request.body = request.body.substr(0, size_b - 2) + "If-None-Match: " + ETag + "\r\n\r\n";
	}
	if(response.pair.find("Last-Modified") != response.pair.end()){
	  std::string lm = response.pair.find("Last-Modified")->second;
	  int size_b = request.body.size();
	  request.body = request.body.substr(0, size_b - 2) + "If-Modified-Since: "+ lm + "\r\n\r\n";
	}
	std::vector<char> new_req = request.ParserToVector();
	Parser new_response = GETMethod(request, client_fd, server_socket.socket_fd, new_req, new_req.size());
	if(new_response.first_line.find("304") != new_response.first_line.npos){
	  std::vector<char> cache_res = cache_response.ParserToVector();
	  send(client_fd, &cache_res.data()[0], cache_res.size(), 0);
	  close(client_fd);
	  close(server_socket.socket_fd);
	}
	else{
	  checkReplaceCache(request, new_response, cache);
	}
      }else {
       //request new response from server and update cache
          response = GETMethod(request, client_fd, server_socket.socket_fd, message, len_msg);
          std::cout << request.uid << ": NOTE cache's first line: " << response.first_line<<std::endl;
	  checkReplaceCache(request, response, cache);
       }
       close(client_fd);
    }
  }
  if(request.request_h.method == "POST"){// POST method
    status = POSTMethod(request, client_fd, server_socket.socket_fd, message,len_msg);
    if (status < 0 ){
      close(client_fd);
    }
  }
}

int main(int argc, char **argv) {  
  Socket proxy_socket;
  proxy_socket.hostname = NULL;
  proxy_socket.port = "12345";
  int status = proxy_socket.newServerSocket();
  if (status < 0){
    std::cout << "no-id: \"" << "ERROR \"creating proxy server socket failed\"" <<std::endl;
    proxylog << "no-id: \"" << "ERROR \"creating proxy server socket failed\"" <<std::endl;
  }
  Cache cache;
  for(;;){
    int client_fd = proxy_socket.acceptSocket();
    if (client_fd > 0) {
        std::thread client_request(realMain, client_fd, &cache);
        client_request.detach();
    }
    else {
      std::cout << "no-id: \"" << "ERROR \"accepting client socket failed\"" <<std::endl;
      proxylog << "no-id: \"" << "ERROR \"accepting client socket failed\"" <<std::endl;
    }
  }
  return 0;
  }
