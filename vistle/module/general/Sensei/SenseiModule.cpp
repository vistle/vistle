#include "SenseiModule.h"


#include <util/listenv4v6.h>
#include <util/hostname.h>


#include <core/rectilineargrid.h>
#include <sstream>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>


#include <insitu/util/print.h>


using namespace std;
using insitu::message::SyncShmIDs;
using insitu::message::InSituMessageType;

#define CERR cerr << "SenseiModule["<< rank() << "/" << size() << "] "
#define DEBUG_CERR vistle::DoNotPrintInstance



SenseiModule::SenseiModule(const string& name, int moduleID, mpi::communicator comm)
    : InSituReader("Observe simulations via SENSEI", name, moduleID, comm)
 {
    m_filePath = addStringParameter("path", "path to a .sim2 file or directory containing these files", "", vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "simulation Files (*.sim2)");
    m_simName = addStringParameter("simulation name", "the name of the simulation as used in the filename of the sim2 file ", "");
 


    m_intOptions[InSituMessageType::VTKVariables] = std::unique_ptr< IntParam<insitu::message::VTKVariables>>(new IntParam<insitu::message::VTKVariables>{ addIntParameter("VTKVariables", "sort the variable data on the grid from VTK ordering to Vistles", false, vistle::Parameter::Boolean), m_messageHandler });
    m_intOptions[InSituMessageType::ConstGrids] = std::unique_ptr< IntParam<insitu::message::ConstGrids>>(new IntParam<insitu::message::ConstGrids>{ addIntParameter("contant grids", "are the grids the same for every timestep?", false, vistle::Parameter::Boolean) , m_messageHandler });
    m_intOptions[InSituMessageType::NthTimestep] = std::unique_ptr< IntParam<insitu::message::NthTimestep>>(new IntParam<insitu::message::NthTimestep>{ addIntParameter("frequency", "frequency in whic data is retrieved from the simulation", 1) , m_messageHandler });
    m_intOptions[InSituMessageType::CombineGrids] = std::unique_ptr< IntParam<insitu::message::CombineGrids>>(new IntParam<insitu::message::CombineGrids>{ addIntParameter("combine grids", "combine all structure grids on a rank to a single unstructured grid", false, vistle::Parameter::Boolean) , m_messageHandler });
    m_intOptions[InSituMessageType::KeepTimesteps] = std::unique_ptr< IntParam<insitu::message::KeepTimesteps>>(new IntParam<insitu::message::KeepTimesteps>{ addIntParameter("keep timesteps", "keep data of processed timestep of this execution", true, vistle::Parameter::Boolean) , m_messageHandler });

}

SenseiModule::~SenseiModule() {

}

bool SenseiModule::beginExecute() {

    if (!m_connectedToEngine)
    {
        return true;
    }
    insitu::message::SetPorts::value_type connectedPorts;
    std::vector<string> p;
    for (const auto port : m_outputPorts)         {
        if (port.second->isConnected()) {
            p.push_back(port.first);
        }
    }
    connectedPorts.push_back(p);
    m_messageHandler.send(insitu::message::SetPorts{ connectedPorts });
    m_messageHandler.send(insitu::message::Ready{ true });
    return true;
}

bool SenseiModule::endExecute() {

    m_messageHandler.send(insitu::message::Ready{ false });


    return true;
}

bool SenseiModule::changeParameter(const vistle::Parameter* param) {
    Module::changeParameter(param);
    if (!param) {
        return true;
    }
    if (param == m_filePath || param == m_simName) {
        connectToSim();
    } else


     if (std::find(m_commandParameter.begin(), m_commandParameter.end(), param) != m_commandParameter.end()) {

        m_messageHandler.send(insitu::message::ExecuteCommand(param->getName()));
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



void SenseiModule::connectToSim() {
    
   }



void SenseiModule::disconnectSim() {
   
}


void SenseiModule::recvAndhandleMessage()     {

    auto msg = m_messageHandler.recv();
    
    DEBUG_CERR << "handleMessage " << (int)msg.type() << endl;
    using namespace insitu;
    switch (msg.type()) {
    case InSituMessageType::Invalid:
        break;
    case InSituMessageType::ShmInit:
        break;
    case InSituMessageType::AddObject:
        break;
    case InSituMessageType::SetPorts: //list of ports, last entry is the type description (e.g mesh or variable)
    {
        auto em = msg.unpackOrCast< message::SetPorts>();
        for (auto i = m_outputPorts.begin(); i != m_outputPorts.end(); ++i) {//destoy unnecessary ports
            if (std::find_if(em.value.begin(), em.value.end(), [i](const std::vector<string>& ports) {return std::find(ports.begin(), ports.end(), i->first) != ports.end();}) == em.value.end()) {
                destroyPort(i->second);
                i = m_outputPorts.erase(i);
            }
        }
        for (auto portList : em.value)             {
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
        auto em = msg.unpackOrCast< message::SetCommands>();
        for (auto i = m_commandParameter.begin(); i != m_commandParameter.end(); ++i) {
            if (std::find(em.value.begin(), em.value.end(), (*i)->getName()) == em.value.end()) {
                removeParameter(*i);
                i = m_commandParameter.erase(i);

            }
        }
        for (auto portName : em.value)             {
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
        m_messageHandler.send(message::GoOn{});
    }
        break;
    case InSituMessageType::ConstGrids:
        break;
    case InSituMessageType::NthTimestep:
        break;
    case InSituMessageType::ConnectionClosed:
    {
        auto state = msg.unpackOrCast<insitu::message::ConnectionClosed>();
        if (state.value)
        {
            sendInfo("the simulation disconnected properly");
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
}



MODULE_MAIN_THREAD(SenseiModule, MPI_THREAD_MULTIPLE)
