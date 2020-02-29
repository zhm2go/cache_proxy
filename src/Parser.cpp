#include "Parser.h"

int Parser::parseRequestHeader(){
  std::string firstLine = this->first_line;
  size_t len_method = firstLine.find(' ');
  std::string new_method = firstLine.substr(0, len_method);
  if(new_method != "GET" && new_method != "CONNECT" && new_method != "POST"){
    std::cerr<<"invalid method"<<std::endl;
    return -1;
  }
  this->request_h.method = new_method;
  
  size_t start_ip = firstLine.find("http://");
  if(start_ip == firstLine.npos){
    size_t end_ipport = firstLine.find(' ', len_method + 1);
    std::string new_ipport = firstLine.substr(len_method + 1, end_ipport - len_method - 1);
    size_t end_ip = new_ipport.find(":");
    std::string new_ip = new_ipport.substr(0, end_ip);
    this->request_h.host = new_ip;
    std::string new_port = new_ipport.substr(end_ip + 1);
    this->request_h.port = new_port;
  }
  else{
    size_t end_ip = firstLine.find('/', start_ip + 7);
    size_t end_url = firstLine.find(' ', start_ip + 7);
    std::string new_ip = firstLine.substr(start_ip + 7, end_ip - start_ip - 7);
    this->request_h.host = new_ip;
    std::string new_url = firstLine.substr(start_ip + 7, end_url - start_ip - 7);
    //std::cout <<"new url is: "<<new_url<<std::endl;
    this->request_h.url = new_url;
    this->request_h.port = "80";
  }
  return 0;
}

int Parser::parseMessage(std::string &message){
  size_t end_first = message.find("\r\n");
  if (end_first == message.npos) {
    std::cerr << "invalid message" <<std::endl;
    return -1;
  }
  std::string new_first_line = message.substr(0, end_first);
  this->first_line = new_first_line;
  size_t end_body = message.find("\r\n\r\n");
  std::string new_body = message.substr(end_first + 2, end_body - end_first);
  this->body = new_body;
  /*
  std::cout <<"----------------------------------------------------------------"<<std::endl;
  std::cout <<"header's key-value pair is:"<<std::endl;
  std::cout <<this->body<<std::endl;
  std::cout <<"----------------------------------------------------------------"<<std::endl;
  */
  if(end_body + 4 < message.size()){
    std::string new_page = message.substr(end_body + 4);
    this->page = new_page;
  }
  return 0;
}

void Parser::setCacheCtrl(){
  std::string cache_ctrl = this->pair.find("Cache-Control")->second;
  ///std::cout <<"cache ctrl elements: "<<cache_ctrl<<std::endl;
  size_t start_ctrl = 0;
  size_t end_ctrl = cache_ctrl.find(',');
  while(true){
    std::string element = cache_ctrl.substr(start_ctrl, end_ctrl - start_ctrl);
    ///std::cout <<element<<std::endl;
    if(element.find('=') != element.npos){
      size_t eq_pos = element.find('=');
      std::string before = element.substr(0, eq_pos);
      std::string after = element.substr(eq_pos + 1);
      this->cache_control[before] = after;
    }
    else{
      this->cache_control[element] = "";
    }
    if(end_ctrl == cache_ctrl.npos){
      break;
    }
    start_ctrl = end_ctrl + 2;
    end_ctrl = cache_ctrl.find(',', start_ctrl);
  }
  /*
  for (std::map<std::string,std::string>::iterator it=this->cache_control.begin(); it!=this->cache_control.end(); ++it\
){
      std::cout << it->first << " => " << it->second << '\n';
    }
    */
}

void Parser::setPair(){
  std::string headerBody = this->body;
  ///std::cout << "header body is" << std::endl;
  ///std::cout << headerBody << std::endl;
  size_t startline = 0;
  size_t endline = headerBody.find("\r\n");
  while(startline + 2 < headerBody.size()){
    std::string line = headerBody.substr(startline, endline - startline);
    size_t colon = line.find(':');
    std::string key = line.substr(0, colon);
    std::string value = line.substr(colon + 2);
    this->pair[key] = value;
    startline = endline + 2;
    endline = headerBody.find("\r\n", startline);
  }
  if(this->pair.find("Cache-Control") != this->pair.end()){
    this->setCacheCtrl();
  }
}

bool Parser::isHeaderExist(std::string header_name) {
  if(this->pair.find(header_name) != this->pair.end()){
    return true;
  }
  return false;
}

bool Parser::checkExistsControlHeader(std::string header_name) {
  if(this->cache_control.find(header_name) != this->pair.end()){
    return true;
  }
  return false;
}

std::string Parser::getCacheControlValue(std::string key) {
  if(this->cache_control.find(key) != this->cache_control.end()){
    return "";
  }
  //std::cout <<"second of "<< key <<" is :"<<std::endl;
  //std::cout <<(this->cache_control.find(key))->second<<std::endl;
  return (this->cache_control.find(key))->second;
}

std::vector<char> Parser::ParserToVector(){
  std::string InCache = this->first_line + "\r\n";
  InCache += this->body + "\r\n";
  InCache += this->page;
  ///std::cout << "In cache exist:" << std::endl;
  ///std::cout << InCache <<std::endl;
  std::vector<char> MsgInCache(InCache.begin(), InCache.end());
  return MsgInCache;
}
