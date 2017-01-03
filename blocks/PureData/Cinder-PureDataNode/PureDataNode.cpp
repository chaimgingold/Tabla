#include "PureDataNode.h"

#include "cinder/Log.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/dsp/Converter.h"

using namespace ci;

namespace cipd {

PureDataNode::PureDataNode(const Format &format)
: Node{ format }, mQueueToAudio{ 64 }, mQueueFromAudio{ 64 } {
  if (getChannelMode() != ChannelMode::SPECIFIED) {
    setChannelMode(ChannelMode::SPECIFIED);
    setNumChannels(2);
  }
}

void PureDataNode::initialize() {
  mPdBase.setReceiver(this);
  mNumTicksPerBlock = getFramesPerBlock() / pd::PdBase::blockSize();

  auto numChannels = getNumChannels();
  auto sampleRate = getSampleRate();

  if (numChannels > 1) {
    mBufferInterleaved = audio::BufferInterleaved(getFramesPerBlock(), numChannels);
  }

  queueTask([=](pd::PdBase &pd) {
    __unused bool success = pd.init(int(numChannels), int(numChannels), int(sampleRate));
    CI_ASSERT(success);

    // in libpd world, dsp computation is controlled through the process methods,
    // so computeAudio is enabled until uninitialize.
    pd.computeAudio(true);

  });
}

void PureDataNode::uninitialize() {
  queueTask([](pd::PdBase &pd) { pd.computeAudio(false); });
}

void PureDataNode::process(audio::Buffer *buffer) {
  {
    QueueItem item;
    for (bool success = true; success;) {
      success = mQueueToAudio.try_dequeue(item);
      if (success) {
        switch (item.which()) {
          case 0:
            break;

          case 1: {
            (*boost::get<TaskPtr>(item))(mPdBase);
          } break;

          case 2: {
            const auto &msg = boost::get<BangMessage>(item);
            mPdBase.sendBang(msg.address);
          } break;

          case 3: {
            const auto &msg = boost::get<FloatMessage>(item);
            mPdBase.sendFloat(msg.address, msg.value);
          } break;

          case 4: {
            const auto &msg = boost::get<SymbolMessage>(item);
            mPdBase.sendSymbol(msg.address, msg.symbol);
          } break;
        }
      }
    }
  }

  if (getNumChannels() > 1) {
    if (getNumConnectedInputs() > 0) {
      audio::dsp::interleaveBuffer(buffer, &mBufferInterleaved);
    }

    mPdBase.processFloat(int(mNumTicksPerBlock), mBufferInterleaved.getData(),
                         mBufferInterleaved.getData());

    audio::dsp::deinterleaveBuffer(&mBufferInterleaved, buffer);
  } else {
    mPdBase.processFloat(int(mNumTicksPerBlock), buffer->getData(), buffer->getData());
  }

  mPdBase.receiveMessages();
}

std::future<PatchRef> PureDataNode::loadPatch(const ci::DataSourceRef &dataSource) {
  if (!isInitialized()) {
    getContext()->initializeNode(shared_from_this());
  }

  const fs::path &path = dataSource->getFilePath();

  return queueTaskWithReturn([path](pd::PdBase &pd) {
    auto patch = pd.openPatch(path.filename().string(), path.parent_path().string());
    if (!patch.isValid()) {
      CI_LOG_E("Could not open patch from dataSource: " << path);
      return PatchRef();
    }
    return std::make_shared<pd::Patch>(std::move(patch));
  });
}

void PureDataNode::closePatch(const PatchRef &patch) {
  if (patch) {
    queueTask([=](pd::PdBase &pd) { pd.closePatch(*patch); });
  }
}

void PureDataNode::setMaxMessageLength(int length) {
	queueTask([=](pd::PdBase &pd) { pd.setMaxMessageLen(length); });
}

void PureDataNode::addToSearchPath(const std::string &path) {
	queueTask([=](pd::PdBase &pd) { pd.addToSearchPath(path); });
}

	
void PureDataNode::queueTask(Task &&task) {
  mQueueToAudio.enqueue(QueueItem(std::make_unique<Task>(std::move(task))));
}

void PureDataNode::subscribe(const std::string &address) {
  queueTask([=](pd::PdBase &pd) { pd.subscribe(address); });
}

void PureDataNode::sendBang(const std::string &dest) {
  mQueueToAudio.enqueue(QueueItem(BangMessage{ dest }));
}

void PureDataNode::sendFloat(const std::string &dest, float value) {
  mQueueToAudio.enqueue(QueueItem(FloatMessage{ dest, value }));
}

void PureDataNode::sendSymbol(const std::string &dest, const std::string &symbol) {
  mQueueToAudio.enqueue(QueueItem(SymbolMessage{ dest, symbol }));
}

void PureDataNode::sendList(const std::string &dest, const pd::List &list) {
  queueTask([=](pd::PdBase &pd) { pd.sendList(dest, list); });
}

void PureDataNode::sendMessage(const std::string &dest, const std::string &msg,
                               const pd::List &list) {
  queueTask([=](pd::PdBase &pd) { pd.sendMessage(dest, msg, list); });
}

std::future<std::vector<float>> PureDataNode::readArray(const std::string &arrayName, int readLen,
                                                        int offset) {
  return queueTaskWithReturn([=](pd::PdBase &pd) {
    std::vector<float> dest;
    pd.readArray(arrayName, dest, readLen, offset);
    return dest;
  });
}

void PureDataNode::writeArray(const std::string &name, const std::vector<float> &source, int length,
                              int offset) {
  queueTask([=](pd::PdBase &pd) {
    // NOTE(ryan): source is internal to this lambda, so it's okay to cast away the const-ness.
    pd.writeArray(name, const_cast<std::vector<float> &>(source), length, offset);
  });
}

void PureDataNode::clearArray(const std::string &name, int value) {
  queueTask([=](pd::PdBase &pd) { pd.clearArray(name, value); });
}


void PureDataNode::print(const std::string &message) {
  CI_LOG_I(message);
}

void PureDataNode::receiveBang(const std::string &address) {
  mQueueFromAudio.enqueue(QueueItem(BangMessage{ address }));
}

void PureDataNode::receiveFloat(const std::string &address, float value) {
  mQueueFromAudio.enqueue(QueueItem(FloatMessage{ address, value }));
}

void PureDataNode::receiveSymbol(const std::string &address, const std::string &symbol) {
  mQueueFromAudio.enqueue(QueueItem(SymbolMessage{ address, symbol }));
}


void PureDataNode::receiveAll(pd::PdReceiver &receiver) {
  QueueItem item;
  for (bool success = true; success;) {
    success = mQueueFromAudio.try_dequeue(item);
    if (success) {
      switch (item.which()) {
        case 0:
          break;

        case 1:
          break;

        case 2: {
          const auto &msg = boost::get<BangMessage>(item);
          receiver.receiveBang(msg.address);
        } break;

        case 3: {
          const auto &msg = boost::get<FloatMessage>(item);
          receiver.receiveFloat(msg.address, msg.value);
        } break;

        case 4: {
          const auto &msg = boost::get<SymbolMessage>(item);
          receiver.receiveSymbol(msg.address, msg.symbol);
        } break;
      }
    }
  }
}

} // namespace cipd
