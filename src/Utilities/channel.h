//
//  channel.hpp
//  Tabla
//
//  Created by Chaim Gingold on 2/24/17.
//  Derived from https://st.xorian.net/blog/2012/08/go-style-channel-in-c/
//
//

#ifndef channel_hpp
#define channel_hpp

#include <list>
#include <thread>

template<class item>
class channel {
private:
  std::list<item> queue;
  std::mutex m;
  std::condition_variable cv;
  bool closed;
public:
  channel() : closed(false) { }
  void close() {
    std::unique_lock<std::mutex> lock(m);
    closed = true;
    cv.notify_all();
  }
  bool is_closed() {
    std::unique_lock<std::mutex> lock(m);
    return closed;
  }
  void put(const item &i, int maxitems=0) {
    std::unique_lock<std::mutex> lock(m);
	if(maxitems>0)
	  cv.wait(lock, [&](){ return closed || queue.size()<maxitems; });
    if(closed)
      throw std::logic_error("put to closed channel");
    queue.push_back(i);
    cv.notify_one();
  }
  bool get(item &out, bool wait = true) {
    std::unique_lock<std::mutex> lock(m);
    if(wait)
      cv.wait(lock, [&](){ return closed || !queue.empty(); });
    if(queue.empty())
      return false;
    out = queue.front();
    queue.pop_front();
    cv.notify_one();
    return true;
  }
};

#endif /* channel_hpp */
