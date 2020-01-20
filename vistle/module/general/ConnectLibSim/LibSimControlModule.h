#ifndef LIBSIM_CONTROL_MODULE_H
#define LIBSIM_CONTROL_MODULE_H
#include <module/module.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

class ControllModule : public vistle::Module
{
public:

    typedef boost::asio::ip::tcp::socket socket;
    typedef boost::asio::ip::tcp::acceptor acceptor;

    ControllModule(const std::string& name, int moduleID, mpi::communicator comm);
    vistle::Port* port = nullptr;
    vistle::StringParameter* command = nullptr;
    vistle::IntParameter* sendCommand = nullptr;
    bool sendCommandChanged = false;
private:


        // Inherited via Reader
        virtual bool prepare() override;

};


#endif // !LIBSIM_CONTROL_MODULE_H
