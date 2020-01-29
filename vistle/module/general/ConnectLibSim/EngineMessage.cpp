#include "EngineMessage.h"
#include "EngineMessage.h"
#include <boost/asio.hpp>
#include <iostream>

const std::string in_situ::EngineMessage::EoM = "הצü";

in_situ::EngineMessage::EngineMessage(in_situ::EngineMessage::Type t)
    :m_type(t) {
    m_msgIn << static_cast<int>(t);
}
in_situ::EngineMessage::Type in_situ::EngineMessage::type() {
    return m_type;
}
in_situ::EngineMessage::EngineMessage(boost::asio::streambuf& streambuf)
{
    m_msgOut.reset(new std::istream(&streambuf));
    int t = 0;
    *m_msgOut >> t;
    m_type = static_cast<Type>(t);

}

bool in_situ::EngineMessage::sendMessage(std::shared_ptr<boost::asio::ip::tcp::socket> s) {
    m_msgIn << EoM;
    boost::system::error_code err;
    boost::asio::write(*s, boost::asio::buffer(m_msgIn.str()), err);
    if (err) {
        std::cerr << "Engine message write error" << std::endl;
        return false;
    }
    return true;
}

in_situ::EngineMessage&& in_situ::EngineMessage::read(std::shared_ptr<boost::asio::ip::tcp::socket> s) {
    boost::system::error_code ec;
    boost::asio::streambuf streambuf;
    auto n = boost::asio::read_until(*s, streambuf, EoM, ec);
    if (ec) {
        std::cerr << "Engine message read_until error" << std::endl;
        return EngineMessage(invalid);
    }
    streambuf.commit(n);
    return EngineMessage(streambuf);

}

void in_situ::EngineMessage::read(std::shared_ptr<boost::asio::ip::tcp::socket> s, std::function<void(EngineMessage&&)> handler) {
    
    boost::system::error_code ec;
    static boost::asio::streambuf streambuf;
    boost::asio::async_read_until(*s, streambuf, EoM, [handler](boost::system::error_code err, size_t n) {
        if (err) {
            std::cerr << "Engine message async_read_until error" << std::endl;
            handler(EngineMessage(invalid));
            return;
        }
        streambuf.commit(n);
        handler(EngineMessage(streambuf));

        });
}


