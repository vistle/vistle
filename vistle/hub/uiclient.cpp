#include "uiclient.h"
#include <core/tcpmessage.h>
#include <util/sysdep.h>
#include "uimanager.h"

namespace asio = boost::asio;

namespace vistle {

UiClient::UiClient(UiManager &manager, int id, boost::shared_ptr<asio::ip::tcp::socket> socket)
: m_id(id)
, m_done(false)
, m_socket(socket)
, m_manager(manager)
{
}

UiClient::~UiClient() {

}

int UiClient::id() const {

   return m_id;
}

UiManager &UiClient::manager() const {

   return m_manager;
}

bool UiClient::done() const {

   return m_done;
}

void UiClient::cancel() {

   m_done = true;
}

boost::shared_ptr<asio::ip::tcp::socket> UiClient::socket() {

   return m_socket;
}

} // namespace vistle
