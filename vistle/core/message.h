#ifndef MESSAGE_H
#define MESSAGE_H

#include <array>

#include <boost/static_assert.hpp>

#include <util/enum.h>
#include <util/directory.h> // ModuleNameLength
#include "uuid.h"
#include "export.h"

#pragma pack(push)
#pragma pack(1)

namespace vistle {

class Communicator;
class Parameter;
class Port;

namespace message {

struct Id {

   enum Reserved {
      ModuleBase = 1, //< >= ModuleBase: modules
      Invalid = 0,
      Broadcast = -1, //< master is broadcasting
      ForBroadcast = -2, //< to master for broadcasting
      NextHop = -3,
      UI = -4,
      LocalManager = -5,
      LocalHub = -6,
      MasterHub = -7, //< < MasterHub: slave hubs
   };

   static bool isHub(int id) { return id <= Id::LocalHub; }
   static bool isModule(int id) { return id >= Id::ModuleBase; }
};

class V_COREEXPORT DefaultSender {

   public:
      DefaultSender();
      static void init(int id, int rank);
      static const DefaultSender &instance();
      static int id();
      static int rank();

   private:
      int m_id;
      int m_rank;
      static DefaultSender s_instance;
};

typedef std::array<char, ModuleNameLength> module_name_t;
typedef std::array<char, 32> port_name_t;
typedef std::array<char, 32> param_name_t;
typedef std::array<char, 256> param_value_t;
typedef std::array<char, 512> param_desc_t;
typedef std::array<char, 50> param_choice_t;
const int param_num_choices = 18;
typedef std::array<char, 300> shmsegname_t;
typedef std::array<char, 900> text_t;
typedef std::array<char, 300> address_t;

typedef boost::uuids::uuid uuid_t;


class V_COREEXPORT Message {
   // this is POD

   friend class vistle::Communicator;

 public:
   static const size_t MESSAGE_SIZE = 1024; // fixed message size is imposed by boost::interprocess::message_queue

   DEFINE_ENUM_WITH_STRING_CONVERSIONS(Type,
      (INVALID) // keep 1st
      (ANY) //< for Trace: enables tracing of all message types -- keep 2nd
      (IDENTIFY)
      (ADDHUB)
      (REMOVESLAVE)
      (SETID)
      (TRACE)
      (SPAWN)
      (SPAWNPREPARED)
      (KILL)
      (QUIT)
      (STARTED)
      (MODULEEXIT)
      (BUSY)
      (IDLE)
      (EXECUTIONPROGRESS)
      (EXECUTE)
      (ADDOBJECT)
      (ADDOBJECTCOMPLETED)
      (OBJECTRECEIVED)
      (ADDPORT)
      (REMOVEPORT)
      (CONNECT)
      (DISCONNECT)
      (ADDPARAMETER)
      (REMOVEPARAMETER)
      (SETPARAMETER)
      (SETPARAMETERCHOICES)
      (PING)
      (PONG)
      (BARRIER)
      (BARRIERREACHED)
      (SENDTEXT)
      (OBJECTRECEIVEPOLICY)
      (SCHEDULINGPOLICY)
      (REDUCEPOLICY)
      (MODULEAVAILABLE)
      (LOCKUI)
      (REPLAYFINISHED)
      (REQUESTTUNNEL)
      (REQUESTOBJECT)
      (SENDOBJECT)
      (NumMessageTypes) // keep last
   )

   Message(const Type type, const unsigned int size);
   // Message (or its subclasses) may not require destructors

   //! processing flags for messages of a type, composed of RoutingFlags
   unsigned long typeFlags() const;

   //! message uuid - copied to related messages (i.e. responses or errors)
   const uuid_t &uuid() const;
   //! set message uuid
   void setUuid(const uuid_t &uuid);
   //! message type
   Type type() const;
   //! sender ID
   int senderId() const;
   //! set sender ID
   void setSenderId(int id);
   //! sender rank
   int rank() const;
   //! set sender rank
   void setRank(int rank);
   //! messge size
   size_t size() const;
   //! broadacast to all ranks?
   bool isBroadcast() const;
   //! mark message for broadcast to all ranks on destination
   void setBroadcast(bool enable=true);

   //! id of message destination
   int destId() const;
   //! set id of message destination
   void setDestId(int id);

   //! rank of message destination
   int destRank() const;
   //! set rank of destination
   void setDestRank(int r);

 private:
   //! message type
   Type m_type;
   //! message size
   unsigned int m_size;
   //! sender ID
   int m_senderId;
   //! sender rank
   int m_rank;
   //! destination ID
   int m_destId;
   //! destination rank - -1: dependent on message, most often current rank
   int m_destRank;
   //! message uuid
   uuid_t m_uuid;
  protected:
   //! broadcast to all ranks?
   bool m_broadcast;
};
V_ENUM_OUTPUT_OP(Type, Message)

class V_COREEXPORT Buffer: public Message {

  public:
   Buffer(): Message(Message::ANY, Message::MESSAGE_SIZE) {
      memset(payload.data(), 0, payload.size());
   }
   Buffer(const Message &message): Message(message) {
      memset(payload.data(), 0, payload.size());
      memcpy(payload.data(), (char *)&message+sizeof(Message), message.size()-sizeof(Message));
   }
   const Buffer &operator=(const Buffer &rhs) {
      *static_cast<Message *>(this) = rhs;
      memcpy(payload.data(), rhs.payload.data(), payload.size());
      return *this;
   }

   template<class SomeMessage>
   SomeMessage &as() { return *static_cast<SomeMessage *>(static_cast<Message *>(this)); }
   template<class SomeMessage>
   SomeMessage const &as() const { return *static_cast<const SomeMessage *>(static_cast<const Message *>(this)); }

   size_t size() const { return Message::MESSAGE_SIZE; }
   char *data() { return static_cast<char *>(static_cast<void *>(this)); }
   const char *data() const { return static_cast<const char *>(static_cast<const void *>(this)); }

  private:
   std::array<char, Message::MESSAGE_SIZE - sizeof(Message)> payload;

};
BOOST_STATIC_ASSERT(sizeof(Buffer) == Message::MESSAGE_SIZE);

V_COREEXPORT std::ostream &operator<<(std::ostream &s, const Message &msg);

} // namespace message
} // namespace vistle

#pragma pack(pop)
#endif
