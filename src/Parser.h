#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <map>
#include <set>
#include <exception>
#include <fstream>
#include <iostream>
#include <arpa/inet.h>
#include <sstream>
#include <thread>
#include <vector>

struct request {
    std::string method;
    std::string host;
    std::string port;
    std::string url;
};

class Parser{
 public:
  std::string first_line;
  std::string body;
  std::string page;
  request request_h;
  std::map<std::string, std::string> pair;
  std::map<std::string, std::string> cache_control;
  int uid;
  std::string ip;

  Parser(){}
  ~Parser(){}
  Parser(const Parser &rhs){
    uid = rhs.uid;
    first_line = rhs.first_line;
    body = rhs.body;
    page = rhs.page;
    request_h = rhs.request_h;
    pair = rhs.pair;
    cache_control = rhs.cache_control;
  }

  Parser &operator=(const Parser &rhs){
    if (this == &rhs) {return *this;}
    Parser tmp;
    tmp.first_line = rhs.first_line;
    tmp.body = rhs.body;
    tmp.page = rhs.page;
    tmp.uid = rhs.uid;
    tmp.pair = rhs.pair;
    tmp.cache_control = rhs.cache_control;
    tmp.request_h.host = rhs.request_h.host;
    tmp.request_h.method = rhs.request_h.method;
    tmp.request_h.port = rhs.request_h.port;
    tmp.request_h.url = rhs.request_h.url;
    
    
    this->first_line = std::move(tmp.first_line);
    this->body = std::move(tmp.body);
    this->page = std::move(tmp.page);
    this->uid = tmp.uid;
    this->cache_control = std::move(tmp.cache_control);
    this->request_h.host = std::move(tmp.request_h.host);
    this->request_h.method = std::move(tmp.request_h.method);
    this->request_h.port = std::move(tmp.request_h.port);
    this->request_h.url = std::move(tmp.request_h.url);
    return *this;
  }
  
  int parseRequestHeader();//parse header and store them in Parser class and struct
  int parseMessage(std::string &message);//split message into first_line, header, body
  void setCacheCtrl();//split header's cache control content and store into map
  void setPair();//split header's content and store into a std::map<std::string, std::string> pair
  bool isHeaderExist(std::string header_name);//check if the string exists in header
  bool checkExistsControlHeader(std::string header_name);//check if the string exists in cache control
  std::string getCacheControlValue(std::string key);//get value corresponding to key
  std::vector<char> ParserToVector();//construct a header from response
};
#endif
