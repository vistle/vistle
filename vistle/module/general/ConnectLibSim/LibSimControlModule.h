#ifndef LIBSIM_CONTROL_MODULE_H
#define LIBSIM_CONTROL_MODULE_H
#include <module/reader.h>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_service.hpp>

class ControllModule : public vistle::Reader
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

        void sendComandToSim();
        bool examine(const vistle::Parameter* param) override;

        // Inherited via Reader
        virtual bool read(Token& token, int timestep = -1, int block = -1) override;

};


#endif // !LIBSIM_CONTROL_MODULE_H
