#include "SenseiModule.h"


#include <vistle/util/listenv4v6.h>
#include <vistle/util/hostname.h>


#include <vistle/core/rectilineargrid.h>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>


#include <vistle/insitu/util/print.h>


using namespace std;
using namespace vistle::insitu::sensei;
using namespace vistle::insitu::message;

#define CERR cerr << "SenseiModule["<< rank() << "/" << size() << "] "
#define DEBUG_CERR vistle::DoNotPrintInstance



SenseiModule::SenseiModule(const string& name, int moduleID, mpi::communicator comm)
    : InSituReader("Observe simulations via SENSEI", name, moduleID, comm)
 {

    m_filePath = addStringParameter("path", "path to a .vistle file", "", vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "simulation Files (*.vistle)");

    m_timeout = addIntParameter("timeout time in second", "time in seconds in which simulation must respond", 10);
    setParameterMinimum(m_timeout, vistle::Integer{ 0 });

    m_deleteShm = addIntParameter("deleteShm", "delete the shm potentially used for communication with sensei", false, vistle::Parameter::Presentation::Boolean);

    m_intOptions[InSituMessageType::NthTimestep] = std::unique_ptr< IntParam<NthTimestep>>(new IntParam<NthTimestep>{ addIntParameter("frequency", "frequency in whic data is retrieved from the simulation", 1) , m_messageHandler });
    m_intOptions[InSituMessageType::CombineGrids] = std::unique_ptr< IntParam<CombineGrids>>(new IntParam<CombineGrids>{ addIntParameter("combine grids", "combine all structure grids on a rank to a single unstructured grid", false, vistle::Parameter::Boolean) , m_messageHandler });
    m_intOptions[InSituMessageType::KeepTimesteps] = std::unique_ptr< IntParam<KeepTimesteps>>(new IntParam<KeepTimesteps>{ addIntParameter("keep timesteps", "keep data of processed timestep of this execution", true, vistle::Parameter::Boolean) , m_messageHandler });

}

SenseiModule::~SenseiModule() {
    if (m_connectedToSim)
    {
        m_messageHandler.send(ConnectionClosed{ true });
    }
}

bool SenseiModule::beginExecute() {

    if (!m_connectedToSim)
    {
        return true;
    }
    SetPorts::value_type connectedPorts;
    std::vector<string> p;
    for (const auto port : m_outputPorts)         {
        if (port.second->isConnected()) {
            p.push_back(port.first);
        }
    }
    connectedPorts.push_back(p);
    m_messageHandler.send(SetPorts{ connectedPorts });
    m_messageHandler.send(Ready{ true });
    return true;
}

bool SenseiModule::endExecute() {

    if (!m_connectedToSim)
    {
        return true;
    }
    operate(); //catch newest state
    m_messageHandler.send(Ready{ false });
    std::unique_ptr<Message> msg;
    while (true) //wait until we receive the confirmation that the sim stopped making vistle objects or timeout
    {
        auto msg = m_messageHandler.timedRecv(m_timeout->getValue());
        if (msg.type() == InSituMessageType::Invalid)
        {
            CERR << "sensei simulation timed out" << endl;
            disconnectSim();
            return false;
        }
        else if (msg.type() == InSituMessageType::Ready && !msg.unpackOrCast<Ready>().value)
        {
            return true;
        }
        else
        {
            handleMessage(msg);
        }
        if (!m_connectedToSim)
        {
            return true;
        }
    }
    
  

    return true;
}

bool SenseiModule::changeParameter(const vistle::Parameter* param) {
    Module::changeParameter(param);
    if (!param) {
        return true;
    }
    if (param == m_deleteShm)
    {
        m_messageHandler.removeShm();
    }
    else if (param == m_filePath) {
        connectToSim();
    } else if (std::find(m_commandParameter.begin(), m_commandParameter.end(), param) != m_commandParameter.end()) {
        m_messageHandler.send(ExecuteCommand(param->getName()));
    } else {
        for (const auto &option : m_intOptions) {
            if (option.second->param() == param) {
                option.second->send();
                continue;
            }
        }
    }

    return InSituReader::changeParameter(param);
}

bool vistle::insitu::sensei::SenseiModule::operate()
{
    bool didWork = false;
    while (recvAndhandleMessage())
    {
        didWork = true;
    };
    return didWork;
}



void SenseiModule::connectToSim() {

    reconnect();
    CERR << "trying to connect to sim with file " << m_filePath->getValue() << endl;
    std::ifstream infile(m_filePath->getValue());
    std::string key, rankStr;
    while (rankStr != std::to_string(rank()))
    {
        infile >> rankStr;
        infile >> key;
        if (infile.eof())
        {
            CERR << "missing conncection key for rank " << rank() << endl;
            break;
        }
    }

    infile.close();
    
    CERR << " key = " << key << endl;
    m_messageHandler.initialize(key, m_rank);
    vector<string> args{ to_string(size()), vistle::Shm::the().instanceName(), name(), to_string(id()), vistle::hostname(), to_string(InstanceNum()) };
    m_messageHandler.send(ShmInit{ args });
    m_connectedToSim = true;
   }



void SenseiModule::disconnectSim() {
    m_connectedToSim = false;
    m_messageHandler.reset();
}




bool SenseiModule::recvAndhandleMessage()     {

    auto msg = m_messageHandler.tryRecv();
    if (msg.type() != message::InSituMessageType::Invalid)
    {
        std::cerr << "received message of type " << static_cast<int>(msg.type()) << std::endl;
    }
    return handleMessage(msg);

}

bool SenseiModule::handleMessage(Message& msg) {

    DEBUG_CERR << "handleMessage " << (int)msg.type() << endl;
    switch (msg.type()) {
    case InSituMessageType::Invalid:
        return false;
        break;
    case InSituMessageType::ShmInit:
        break;
    case InSituMessageType::AddObject:
        break;
    case InSituMessageType::SetPorts: //list of ports, last entry is the type description (e.g mesh or variable)
    {
        auto em = msg.unpackOrCast< SetPorts>();
        for (auto i = m_outputPorts.begin(); i != m_outputPorts.end(); ++i) {//destoy unnecessary ports
            if (std::find_if(em.value.begin(), em.value.end(), [i](const std::vector<string>& ports) {return std::find(ports.begin(), ports.end(), i->first) != ports.end(); }) == em.value.end()) {
                destroyPort(i->second);
                i = m_outputPorts.erase(i);
            }
        }
        for (auto portList : em.value) {
            for (size_t i = 0; i < portList.size() - 1; i++) {
                auto lb = m_outputPorts.lower_bound(portList[i]);
                if (!(lb != m_outputPorts.end() && !(m_outputPorts.key_comp()(portList[i], lb->first)))) {
                    m_outputPorts.insert(lb, std::make_pair(portList[i], createOutputPort(portList[i], portList[portList.size() - 1])));
                }
            }
        }

    }
    break;
    case InSituMessageType::SetCommands:
    {
        auto em = msg.unpackOrCast< SetCommands>();
        for (auto i = m_commandParameter.begin(); i != m_commandParameter.end(); ++i) {
            if (std::find(em.value.begin(), em.value.end(), (*i)->getName()) == em.value.end()) {
                removeParameter(*i);
                i = m_commandParameter.erase(i);

            }
        }
        for (auto portName : em.value) {
            auto lb = std::find_if(m_commandParameter.begin(), m_commandParameter.end(), [portName](const auto& val) {return val->getName() == portName; });
            if (lb == m_commandParameter.end()) {
                m_commandParameter.insert(addIntParameter(portName, "trigger command on change", false, vistle::Parameter::Presentation::Boolean));
            }
        }
    }
    break;
    case InSituMessageType::Ready:
        break;
    case InSituMessageType::ExecuteCommand:
        break;
    case InSituMessageType::GoOn:
    {
        m_messageHandler.send(GoOn{});
    }
    break;
    case InSituMessageType::ConstGrids:
        break;
    case InSituMessageType::NthTimestep:
        break;
    case InSituMessageType::ConnectionClosed:
    {
        auto state = msg.unpackOrCast<ConnectionClosed>();
        if (state.value)
        {
            CERR << "the simulation disconnected properly" << endl;;
        }
        else
        {
            CERR << " tcp connection closed...disconnecting." << endl;
        }
#ifndef MODULE_THREAD
        disconnectSim();
#endif
    }
    break;
    default:
        break;
    }
    return true;
}



MODULE_MAIN_THREAD(SenseiModule, MPI_THREAD_MULTIPLE)
