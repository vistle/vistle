#include "LibSimModule.h"

#include <vistle/util/hostname.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/sleep.h>
#include <vistle/util/filesystem.h>

#include <vistle/insitu/libsim/connectLibsim/connect.h>

#include <sstream>
#include <vistle/core/rectilineargrid.h>

#include <boost/bind.hpp>
#include <boost/mpi.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <vistle/core/message.h>
#include <vistle/core/tcpmessage.h>
#include <vistle/insitu/core/slowMpi.h>
#include <vistle/insitu/util/print.h>

#ifdef MODULE_THREAD
#include <vistle/insitu/libsim/engineInterface/EngineInterface.h>
#endif
using namespace std;
namespace libsim = vistle::insitu::libsim;
using vistle::insitu::message::InSituMessageType;

#define CERR cerr << "LibSimModule[" << rank() << "/" << size() << "] "
#define DEBUG_CERR vistle::DoNotPrintInstance

LibSimModule::LibSimModule(const string &name, int moduleID, mpi::communicator comm): InSituModule(name, moduleID, comm)
{
#ifndef MODULE_THREAD
    m_filePath = addStringParameter("path", "path to a .sim2 file or directory containing these files", "",
                                    vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "simulation Files (*.sim2)");
    m_simName = addStringParameter("simulation_name",
                                   "the name of the simulation as used in the filename of the sim2 file ", "");
#else
    initializeCommunication();
#endif // !MODULE_THREAD

    m_intOptions.push_back(addIntParameter("vtk_variables",
                                           "sort the variable data on the grid from VTK ordering to Vistles", false,
                                           vistle::Parameter::Boolean));
    m_intOptions.push_back(addIntParameter("constant_grids", "are the grids the same for every timestep?", false,
                                           vistle::Parameter::Boolean));
    m_intOptions.push_back(addIntParameter("frequency", "frequency in which data is retrieved from the simulation", 1));
    m_intOptions.push_back(addIntParameter("combine_grids",
                                           "combine all structure grids on a rank to a single unstructured grid", true,
                                           vistle::Parameter::Boolean));
    m_intOptions.push_back(addIntParameter("keep_timesteps", "keep data of processed timestep of this execution", true,
                                           vistle::Parameter::Boolean));
}

LibSimModule::~LibSimModule()
{}

std::unique_ptr<vistle::insitu::message::MessageHandler> LibSimModule::connectToSim()
{
#ifdef MODULE_THREAD
    CERR << "connectToSim" << std::endl;
    if (!libsim::EngineInterface::getHandler()) {
        CERR << "can not start without running simulation" << endl;
    }
    CERR << "port " << libsim::EngineInterface::getHandler()->port() << std::endl;
    return libsim::EngineInterface::extractHandler();
#else
    auto handler = std::make_unique<vistle::insitu::message::InSituTcp>(comm());
    bool simInitSent = false;
    if (rank() == 0) {
        using namespace vistle::filesystem;
        path p{m_filePath->getValue()};
        if (is_directory(p)) {
            auto simName = m_simName->getValue();
            path lastEditedFile;
            std::time_t lastEdit{};
            for (auto &entry: boost::make_iterator_range(vistle::filesystem::directory_iterator(p), {})) {
                if (simName.size() == 0 ||
                    entry.path().filename().generic_string().find(simName + ".sim2") != std::string::npos) {
                    auto editTime = last_write_time(entry.path());
                    if (editTime > lastEdit) {
                        lastEdit = editTime;
                        lastEditedFile = entry.path();
                    }
                }
            }
            p = lastEditedFile;
        }
        DEBUG_CERR << "opening file: " << p.string() << endl;
        vector<string> args{to_string(handler->port())};
        if (vistle::insitu::libsim::attemptLibSimConnection(p.string(), args)) {
            simInitSent = true;
        } else {
            CERR << "attemptLibSimConnection failed " << endl;
        }
    }
    boost::mpi::broadcast(comm(), simInitSent, 0);
    return simInitSent ? std::move(handler) : nullptr;
#endif // !MODULE_THREAD
}


MODULE_MAIN_THREAD(LibSimModule, boost::mpi::threading::multiple)
