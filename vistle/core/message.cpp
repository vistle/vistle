#include "message.h"
#include "messagerouter.h"
#include "shm.h"
#include "parameter.h"
#include "port.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <algorithm>

namespace vistle {
namespace message {

DefaultSender DefaultSender::s_instance;

DefaultSender::DefaultSender()
: m_id(Id::Invalid)
, m_rank(-1)
{
   Router::the();
}

int DefaultSender::id() {

   return s_instance.m_id;
}

int DefaultSender::rank() {

   return s_instance.m_rank;
}

void DefaultSender::init(int id, int rank) {

   s_instance.m_id = id;
   s_instance.m_rank = rank;
}

const DefaultSender &DefaultSender::instance() {

   return s_instance;
}

static boost::uuids::random_generator s_uuidGenerator = boost::uuids::random_generator();

Message::Message(const Type t, const unsigned int s)
: m_type(t)
, m_size(s)
, m_senderId(DefaultSender::id())
, m_rank(DefaultSender::rank())
, m_destId(Id::NextHop)
, m_destRank(-1)
, m_uuid(t == ANY ? boost::uuids::nil_generator()() : s_uuidGenerator())
, m_referrer(boost::uuids::nil_generator()())
, m_payloadSize(0)
, m_broadcast(false)
{

   vassert(m_size <= MESSAGE_SIZE);
   vassert(m_type > INVALID);
   vassert(m_type < NumMessageTypes);
}

unsigned long Message::typeFlags() const {

   vassert(type() > ANY && type() > INVALID && type() < NumMessageTypes);
   if (type() <= ANY || type() <= INVALID) {
      return 0;
   }
   if (type() >= NumMessageTypes) {
      return 0;
   }

   return Router::rt[type()];
}

const uuid_t &Message::uuid() const {

   return m_uuid;
}

void Message::setUuid(const uuid_t &uuid) {

   m_uuid = uuid;
}

const uuid_t &Message::referrer() const {

   return m_referrer;
}

void Message::setReferrer(const uuid_t &uuid) {

   m_referrer = uuid;
}

int Message::senderId() const {

   return m_senderId;
}

void Message::setSenderId(int id) {

   m_senderId = id;
}

int Message::destId() const {

   return m_destId;
}

void Message::setDestId(int id) {

   m_destId = id;
}

int Message::destRank() const {
   return m_destRank;
}

void Message::setDestRank(int r) {
   m_destRank = r;
}

int Message::rank() const {

   return m_rank;
}

void Message::setRank(int rank) {
   
   m_rank = rank;
}

Type Message::type() const {

   return m_type;
}

size_t Message::size() const {

   return m_size;
}

size_t Message::payloadSize() const {
   return m_payloadSize;
}

bool Message::isBroadcast() const {

    return m_broadcast;
}

void Message::setBroadcast(bool enable) {
    m_broadcast = enable;
}

} // namespace message
} // namespace vistle
