#include <string>

#include <boost/format.hpp>

#include <vistle/core/points.h>
#include <vistle/util/filesystem.h>

#include "LoadCover.h"

MODULE_MAIN(LoadCover)

using namespace vistle;

LoadCover::LoadCover(const std::string &name, int moduleID, mpi::communicator comm): Module(name, moduleID, comm)
{
    createOutputPort("grid_out");
    addStringParameter("filename", "name of file (%1%: rank)", "", Parameter::ExistingFilename);
    setParameterFilters("filename", "VRML Files (*.wrl *.WRL *.wrl.gz *.wrz *.vrml)/X3D Files (*.x3d)/All Files (*)");
    addIntParameter("rank", "rank of node where to load (-1: all nodes)", 0);
}

LoadCover::~LoadCover()
{}

bool LoadCover::compute()
{
    return true;
}

bool LoadCover::prepare()
{
    std::string filename = getStringParameter("filename");
    int loadRank = getIntParameter("rank");

    if (rank() == loadRank || loadRank == -1) {
        std::string f;
        try {
            using namespace boost::io;
            boost::format fmter(filename);
            fmter.exceptions(all_error_bits ^ (too_many_args_bit | too_few_args_bit));
            fmter % rank();
            f = fmter.str();
        } catch (boost::io::bad_format_string &ex) {
            sendError("bad format string in filename");
            return true;
        }

        if (!vistle::filesystem::is_regular_file(f)) {
            sendInfo("not a regular file: %s", f.c_str());
        }

        Points::ptr points(new Points(Index(0)));
        points->addAttribute("_model_file", f);
        updateMeta(points);
        addObject("grid_out", points);
    }

    return true;
}
