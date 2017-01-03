#pragma once

#include "PdBase.hpp"
#include "PdReceiver.hpp"
#include "PdTypes.hpp"

#include "cinder/DataSource.h"
#include "cinder/Thread.h"
#include "cinder/audio/Node.h"

#include "concurrentqueue.h"

#include <boost/variant.hpp>
#include <future>

namespace cipd {

typedef std::shared_ptr<pd::Patch> PatchRef;
typedef std::shared_ptr<class PureDataNode> PureDataNodeRef;

class PureDataNode : public ci::audio::Node, public pd::PdReceiver {
protected:
  pd::PdBase mPdBase;
  size_t mNumTicksPerBlock;

  ci::audio::BufferInterleaved mBufferInterleaved;

  // NOTE(ryan): Tasks must unfortunately be wrapped in a unique_ptr here to get around an
  // alignment restruction imposed by ConcurrentQueue. On armv7 max_align_t is 4 byte aligned
  // while std::function is 8, which is disallowed by ConcurrentQueue's implementation.
  using Task = std::function<void(pd::PdBase &)>;
  using TaskPtr = std::unique_ptr<Task>;

  struct BangMessage {
    std::string address;
  };

  struct FloatMessage {
    std::string address;
    float value;
  };

  struct SymbolMessage {
    std::string address;
    std::string symbol;
  };

  using QueueItem = boost::variant<boost::blank, TaskPtr, BangMessage, FloatMessage, SymbolMessage>;

  moodycamel::ConcurrentQueue<QueueItem> mQueueToAudio, mQueueFromAudio;

public:
  PureDataNode(const Format &format = Format());

  void initialize() override;
  void uninitialize() override;

  void print(const std::string &message) override;
  void receiveBang(const std::string &address) override;
  void receiveFloat(const std::string &address, float value) override;
  void receiveSymbol(const std::string &address, const std::string &symbol) override;

  void process(ci::audio::Buffer *buffer) override;

  std::future<PatchRef> loadPatch(const ci::DataSourceRef &dataSource);
  void closePatch(const PatchRef &patch);

  void queueTask(Task &&task);

  template <typename Fn>
  std::future<typename std::result_of<Fn(pd::PdBase &)>::type> queueTaskWithReturn(Fn &&task) {
    using ReturnType = typename std::result_of<Fn(pd::PdBase &)>::type;

    // NOTE(ryan): We need to wrap the promise in a shared_ptr here to get around the fact that
    // C++11 does not have init-capture, so we can't move into the lambda passed to queueTask().
    auto promise = std::make_shared<std::promise<ReturnType>>();

    queueTask([=](pd::PdBase &pd) mutable { promise->set_value(task(pd)); });
    return promise->get_future();
  }

  void subscribe(const std::string &address);

  // thread-safe senders
  void sendBang(const std::string &dest);
  void sendFloat(const std::string &dest, float value);
  void sendSymbol(const std::string &dest, const std::string &symbol);
  void sendList(const std::string &dest, const pd::List &list);
  void sendMessage(const std::string &dest, const std::string &msg,
                   const pd::List &list = pd::List());

  std::future<std::vector<float>> readArray(const std::string &arrayName, int readLen = -1,
                                            int offset = 0);
  void writeArray(const std::string &name, const std::vector<float> &source, int length = -1,
                  int offset = 0);
  void clearArray(const std::string &name, int value = 0);

  void receiveAll(pd::PdReceiver &receiver);

  void setMaxMessageLength(int length);
  void addToSearchPath(const std::string &path);
};

} // namespace cipd
