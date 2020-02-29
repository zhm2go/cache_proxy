SOURCES = main.cpp Socket.cpp Parser.cpp Cache.cpp
OBJS = $(patsubst %.cpp, %.o, $(SOURCES))
CPPFLAGS = -ggdb3 -Wall -Werror -pedantic -std=c++11

http_caching_proxy: $(OBJS)
	g++ $(CPPFLAGS) -o http_caching_proxy $(OBJS) -lpthread
%.o: %.cpp Socket.h Parser.cpp Cache.h
	g++ $(CPPFLAGS) -c $< -lpthread
  
clean:
	rm -rf *~ http_caching_proxy *.o *.txt
