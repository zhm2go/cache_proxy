#ifndef __CACHE_H__
#define __CACHE_H__

#include "Parser.h"
#include <ctime>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

#define CACHESIZE 256

class Cache {
private:
  std::unordered_map<std::string, Parser> caches;
  std::list<std::string> lru_queue;
  
public:
  Cache(){};
  bool isRequestInCache(Parser &request);//search cache via the request's host and url
  bool isCacheCtrlFieldExist(Parser &http, std::string header);
  std::string checkCacheInfo(Parser &request);//valid, expire, or need validate
  Parser getCache(std::string host);//get cached response
  bool isCacheCtrlExist(Parser &http);
  double diffExpireTime(Parser &cache_response, int max_age, int extra_age, int max_stale);  
  void replaceCache(Parser &request, Parser &response); //update or add cache into cache map 
  std::string getTime(time_t time);
  time_t getExpireTime(Parser &cache_response, int lifetime, int fresh_time, int max_stale);
  time_t getUTCTime(std::string time);//change time to UTC
};

#endif


