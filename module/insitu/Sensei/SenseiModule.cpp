#include "SenseiModule.h"

#include <vistle/core/rectilineargrid.h>
#include <vistle/insitu/core/exception.h>
#include <vistle/insitu/message/ShmMessage.h>
#include <vistle/insitu/util/print.h>
#include <vistle/util/directory.h>
#include <vistle/util/hostname.h>
#include <vistle/util/listenv4v6.h>
#include <vistle/util/sleep.h>
#include <sstream>
#include <fstream>

#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/mpi.hpp>

using namespace std;
using namespace vistle;
using namespace vistle::insitu::sensei;
using namespace vistle::insitu::message;

#define CERR cerr << "SenseiModule[" << rank() << "/" << size() << "] "
#define DEBUG_CERR vistle::DoNotPrintInstance

SenseiModule::SenseiModule(const string &name, int moduleID, mpi::communicator comm)
: insitu::InSituModule(name, moduleID, comm)
{
    m_filePath = addStringParameter("path", "path to the connection file written by the simulation",
                                    directory::configHome() + "/sensei.vistle", vistle::Parameter::ExistingFilename);
    setParameterFilters(m_filePath, "simulation Files (*.vistle)");

    m_intOptions.push_back(addIntParameter("frequency", "the pipeline is processed for every nth simulation cycle", 1));
    m_intOptions.push_back(addIntParameter("keep_timesteps",
                                           "if true timesteps are cached and processed as time series", true,
                                           vistle::Parameter::Boolean));
}

std::unique_ptr<insitu::message::MessageHandler> SenseiModule::connectToSim()
{
    CERR << "trying to connect to sim with file " << m_filePath->getValue() << endl;
    std::ifstream infile(m_filePath->getValue());
    if (infile.fail()) {
        CERR << "failed to open file " << m_filePath->getValue() << endl;
        return nullptr;
    }
    std::string key, rankStr;
    while (rankStr != std::to_string(rank())) {
        infile >> rankStr;
        infile >> key;
        if (infile.eof()) {
            CERR << "missing connection key for rank " << rank() << endl;
            return nullptr;
        }
    }
    try {
        return std::make_unique<message::InSituShmMessage>(key, m_rank);
    } catch (const insitu::InsituException &e) {
        std::cerr << e.what() << '\n';
        return nullptr;
    }
}

MODULE_MAIN_THREAD(SenseiModule, boost::mpi::threading::multiple)
