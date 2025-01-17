#include "Cache.h"
std::mutex mtx_cache;

bool Cache::isRequestInCache(Parser &request){
  if(caches.find(request.request_h.url) == caches.end()){
    return false;
  }
  else{
    return true;}
}

bool Cache::isCacheCtrlExist(Parser &http) {
  return http.isHeaderExist("Cache-Control");
}

bool Cache::isCacheCtrlFieldExist(Parser &http, std::string header) {
  std::string header_str = header;
  return http.checkExistsControlHeader(header_str);
}

Parser Cache::getCache(std::string url) {
  std::lock_guard<std::mutex> lck(mtx_cache);
  return caches[url];
}

void Cache::replaceCache(Parser &request, Parser &response) {
  std::lock_guard<std::mutex> lck(mtx_cache);
  std::string url_name = request.request_h.url;
  if (lru_queue.size() > CACHESIZE){
    std::string last = lru_queue.back();
    lru_queue.pop_back();
    caches.erase(last);
  }
  caches[url_name] = response;
  std::cout << request.uid << ": NOTE " << "replace cache from " << url_name <<std::endl;
  
  if (isRequestInCache(request)) {
    lru_queue.remove(url_name);
    lru_queue.push_front(url_name);
  } else {
    lru_queue.push_front(url_name);
  }
}
/*
ID: not in cache
ID: in cache, but expired at EXPIREDTIME
ID: in cache, requires validation
ID: in cache, valid
*/
std::string Cache::checkCacheInfo(Parser &request){
  Parser cache_response = getCache(request.request_h.url);
  cache_response.ParserToVector();
  int max_age = 0;
  int max_stale = 0;
  int fresh_time = 0;
  
  if (isCacheCtrlExist(request)) {
      if (isCacheCtrlFieldExist(request, "no-cache")) {
        return "in cache, requires validation";
      } 
      if (isCacheCtrlFieldExist(request, "max-age")) {
        max_age = std::stoi(request.getCacheControlValue("max-age"));
        if (max_age == 0) {
          return "in cache, requires validation";
        }
     }
     if (isCacheCtrlFieldExist(request, "min-fresh")) {
          fresh_time = std::stoi(request.getCacheControlValue("min-fresh"));
     }
     if(isCacheCtrlFieldExist(request, "max-stale")){
       if(request.getCacheControlValue("max-stale") != ""){
	 max_stale = std::stoi(request.getCacheControlValue("max-stale"));
       }
     }
  }
  
  if (isCacheCtrlExist(cache_response)) {
      if (isCacheCtrlFieldExist(cache_response, "no-cache")) {
        return "not cacheable because cache control with no-cache";
      }
      if (isCacheCtrlFieldExist(cache_response, "max-age")) {
	if (std::max(max_age, std::stoi(cache_response.getCacheControlValue("max-age"))) == 0) {
          return "in cache, requires validation";
        }
      }
      /*if(isCacheCtrlFieldExist(cache_response, "max-stale")){
       if(cache_response.getCacheControlValue("max-stale") != ""){
	 max_stale = std::stoi(cache_response.getCacheControlValue("max-stale"));
       }
     }*/
  
      std::cout << request.uid << ": Request's max-age is " << max_age << std::endl;
      double fresh_diff = diffExpireTime(cache_response, max_age, fresh_time, max_stale);
      if (fresh_diff < 36000) {
        time_t expire_time = getExpireTime(cache_response, max_age, fresh_time, max_stale);
        std::string expire_msg = "in cache, but expired at " + getTime(expire_time);
        return expire_msg;
        }

      }
  return "in cache, valid";
}


double Cache::diffExpireTime(Parser &cache_response, int max_age, int fresh_time, int max_stale) {

  time_t request_time = time(0);
  tm *request_time_gmt = gmtime(&request_time);
  request_time = mktime(request_time_gmt);
  
  std::map<std::string, std::string>::iterator it = cache_response.pair.find("Date");
  std::string header_date = (it == cache_response.pair.end()) ? "" : it->second;
  time_t cache_time = getUTCTime(header_date);
  time_t expire_time = cache_time + (max_age - fresh_time);

  return difftime(request_time, expire_time);
}

time_t Cache::getExpireTime(Parser &cache_response, int max_age, int fresh_time, int max_stale) {
  std::map<std::string, std::string>::iterator it = cache_response.pair.find("Date");
  std::string header_date = (it == cache_response.pair.end()) ? "" : it->second;
  time_t cache_time = getUTCTime(header_date);
  time_t expire_time = cache_time + (max_age + max_stale - fresh_time);
  return expire_time;
}

time_t Cache::getUTCTime(std::string time_k) {
  std::lock_guard<std::mutex> lck(mtx_cache);
  size_t pos = std::string::npos;
  while ((pos = time_k.find(" GMT")) != std::string::npos) {
    time_k.erase(pos, 4);
  }
  tm tm;
  strptime(time_k.c_str(), "%a, %d %b %Y %H:%M:%S", &tm);
  time_t t = mktime(&tm);
  return t;
}

std::string Cache::getTime(time_t time) {
  tm *gmtm = localtime(&time);
  char *dt = asctime(gmtm);
  std::string str = std::string(dt);
  return str;
}
