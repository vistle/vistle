#ifndef INSITU_MESSAGE_H
#define INSITU_MESSAGE_H

#include "export.h"
#include "moduleInfo.h"
#include "sharedOption.h"

#include <array>
#include <string>
#include <utility>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/streambuf.hpp>

#include <boost/mpi.hpp>
#include <vistle/core/archives.h>
#include <vistle/core/archives_config.h>
#include <vistle/core/message.h>
#include <vistle/core/messagepayload.h>
#include <vistle/core/messages.h>
#include <vistle/core/shm.h>
#include <vistle/core/tcpmessage.h>

#include <vistle/util/vecstreambuf.h>

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

namespace vistle {
namespace insitu {

namespace message {

enum class V_INSITUMESSAGEEXPORT InSituMessageType {
  Invalid,
  AddObject,
  ConnectionClosed,
  ExecuteCommand,
  GoOn,
  LibSimIntOption,
  Ready,
  SenseiIntOption,
  SetCommands,
  SetPorts // detected ports from sim to module <--> connected ports from module
           // to Engine
  ,
  ShmInfo
#ifdef MODULE_THREAD
  ,
  ModuleID
#endif
};

class Message;
struct V_INSITUMESSAGEEXPORT InSituMessageBase
{
  InSituMessageBase(InSituMessageType t)
      : m_type(t){};
  InSituMessageType type() const;

protected:
  InSituMessageType m_type;
};

template <InSituMessageType Type, typename T>
struct WhatEverToCallThis : InSituMessageBase
{
  typedef T value_type;
  friend class insitu::message::Message;
  WhatEverToCallThis(const T &payload)
      : InSituMessageBase(Type)
      , value(payload)
  {
  }
  static const InSituMessageType type = Type;
  T value{};
  ARCHIVE_ACCESS
  template <class Archive>
  void serialize(Archive &ar)
  {
    ar &value;
  }

private:
  WhatEverToCallThis()
      : InSituMessageBase(Type)
  {
  }
};

#define COMMA ,

#define DECLARE_ENGINE_MESSAGE_WITH_PARAM(messageType, payloadType)                                                    \
  typedef V_INSITUMESSAGEEXPORT WhatEverToCallThis<InSituMessageType::messageType, payloadType> messageType;

#define DECLARE_ENGINE_MESSAGE(messageType)                                                                            \
  struct V_INSITUMESSAGEEXPORT messageType : public InSituMessageBase                                                  \
  {                                                                                                                    \
    const InSituMessageType type = InSituMessageType::messageType;                                                     \
    messageType()                                                                                                      \
        : InSituMessageBase(InSituMessageType::messageType)                                                            \
    {                                                                                                                  \
    }                                                                                                                  \
    ARCHIVE_ACCESS                                                                                                     \
    template <class Archive>                                                                                           \
    void serialize(Archive &ar)                                                                                        \
    {                                                                                                                  \
    }                                                                                                                  \
  };

DECLARE_ENGINE_MESSAGE(Invalid)
DECLARE_ENGINE_MESSAGE(GoOn)

DECLARE_ENGINE_MESSAGE_WITH_PARAM(ConnectionClosed, bool) // true -> disconnected on purpose
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ShmInfo, ModuleInfo::ShmInfo)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(AddObject, std::string)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(SetPorts, std::vector<std::vector<std::string>>)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(SetCommands, std::vector<std::string>)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(Ready, bool)
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ExecuteCommand, std::string)

#ifdef MODULE_THREAD
DECLARE_ENGINE_MESSAGE_WITH_PARAM(ModuleID, int)
#endif

struct V_INSITUMESSAGEEXPORT InSituMessage : public vistle::message::MessageBase<InSituMessage, vistle::message::INSITU>
{
  InSituMessage(InSituMessageType t)
      : m_ismType(t)
  {
  }
  InSituMessageType ismType() const { return m_ismType; }

private:
  InSituMessageType m_ismType;
};
static_assert(sizeof(InSituMessage) <= vistle::message::Message::MESSAGE_SIZE, "message too large");

class V_INSITUMESSAGEEXPORT Message
{

public:
  InSituMessageType type() const;

  template <typename SomeMessage>
  SomeMessage &unpackOrCast() const
  {
    assert(SomeMessage::type == type());
    if (!m_msg) {
      vistle::vecistreambuf<vistle::buffer> buf(m_payload);
      m_msg.reset(new SomeMessage{});
      try {
        vistle::iarchive ar(buf);
        ar &*static_cast<SomeMessage *>(m_msg.get());
#ifdef USE_YAS
      } catch (yas::io_exception &ex) {
        std::cerr << "ERROR: failed to get InSituTcp::Message of type " << static_cast<int>(type())
                  << " with payload size " << m_payload.size() << " bytes: " << ex.what() << std::endl;
#endif
      } catch (...) {
        throw;
      }
    }
    return *static_cast<SomeMessage *>(m_msg.get());
  }
  static Message errorMessage();
  Message(InSituMessageType type, vistle::buffer &&payload);

private:
  Message();
  InSituMessageType m_type;
  vistle::buffer m_payload;
  mutable std::unique_ptr<InSituMessageBase> m_msg;
};

} // namespace message
} // namespace insitu
} // namespace vistle

#endif // !ENGINE_MESSAGE_H
