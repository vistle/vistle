/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see LICENSE.txt.

 * License: LGPL 2+ */

/**************************************************************************\ 
 **                                                           (C)1995 RUS  **
 **                                                                        **
 ** Description: Read module for Dyna3D data         	                  **
 **              (Attention: Thick shell elements omitted !)               **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 **                                                                        **
 ** Author:                                                                **
 **                                                                        **
 **                             Uwe Woessner                               **
 **                Computer Center University of Stuttgart                 **
 **                            Allmandring 30                              **
 **                            70550 Stuttgart                             **
 **                                                                        **
 ** Date:  17.03.95  V1.0                                                  **
 ** Revision R. Beller 08.97 & 02.99                                       **
\**************************************************************************/


#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "ReadDyna3D.h"

#include <vistle/util/enum.h>

using namespace vistle;


// Constructor
ReadDyna3D::ReadDyna3D(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader(name, moduleID, comm)
{
    // Output ports
    p_grid = createOutputPort("grid_out", "grid");
    p_data_1 = createOutputPort("data1_out", "vector data");
    p_data_2 = createOutputPort("data2_out", "scalar data");

    // Parameters
    p_data_path = addStringParameter("filename", "Geometry file path", "/data/general/examples/Dyna3d/aplot",
                                     Parameter::ExistingFilename);
    p_nodalDataType =
        addIntParameter("nodalDataType", "Nodal results data to be read", No_Node_Data, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_nodalDataType, NodalDataType);
    p_elementDataType =
        addIntParameter("elementDataType", "Element results data to be read", No_Element_Data, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_elementDataType, ElementDataType);
    p_component = addIntParameter("component", "stress tensor component", Von_Mises, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_component, Component);
    p_Selection = addStringParameter("Selection", "Number selection for parts", "all");
    // p_State = addIntSliderParam("State","Timestep");
    p_format = addIntParameter("format", "Format of LS-DYNA3D ptf-File", Guess, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_format, Format);

    p_byteswap = addIntParameter("byteswap", "Perform Byteswapping", Auto, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_byteswap, ByteSwap);

    p_only_geometry = addIntParameter("OnlyGeometry", "Reading only Geometry? yes|no", false, Parameter::Boolean);

    // Default setting
    //p_Selection->setValue("0-9999999");
    // p_State->setValue(1,1,1);
    // p_Step->setValue(1,1,1);
    //long init_Step[2] = { 1, 0 };
    //p_Step->setValue(2, init_Step);
    //p_format->setValue(2, format_label, 1);
    //p_only_geometry->setValue(0);

    //setParallelizationMode(ParallelizeTimeAndBlocks);
    //setParallelizationMode(ParallelizeTimesteps);
    setParallelizationMode(ParallelizeBlocks);

    observeParameter(p_data_path);
    observeParameter(p_format);
    observeParameter(p_byteswap);
    observeParameter(p_only_geometry);
    observeParameter(p_Selection);
}

ReadDyna3D::~ReadDyna3D()
{}


bool ReadDyna3D::prepareRead()
{
    if (!dyna3dReader) {
        dyna3dReader.reset(newReader());
    }

    dyna3dReader->setNodalDataType(NodalDataType(p_nodalDataType->getValue()));
    dyna3dReader->setElementDataType(ElementDataType(p_elementDataType->getValue()));
    dyna3dReader->setComponent(Component(p_component->getValue()));

    return dyna3dReader->readStart() == 0;
}

bool ReadDyna3D::read(Reader::Token &token, int timestep, int block)
{
    auto &reader = dyna3dReader;
    //std::cerr << "reading: token: " << token << ", time=" << timestep << ", block=" << block << std::endl;

    if (p_only_geometry->getValue()) {
        return reader->readOnlyGeo(token, block) == 0;
    } else {
        if (timestep == -1)
            return true;
        return reader->readState(token, timestep + 1, block) == 0;
    }

    return true;
}

bool ReadDyna3D::examine(const Parameter *param)
{
    //std::cerr << "ReadDyna3D::examine(param=" << (param ? param->getName() : "(null)") << ")" << std::endl;

    bool rescan = false;
    if (!param)
        rescan = true;
    if (!dyna3dReader) {
        dyna3dReader.reset(newReader());
        rescan = true;
    }
    assert(dyna3dReader);

    dyna3dReader->setFilename(p_data_path->getValue());
    if (param == p_data_path) {
        rescan = true;
    }

    dyna3dReader->setByteswap(ByteSwap(p_byteswap->getValue()));
    if (param == p_byteswap) {
        rescan = true;
    }
    dyna3dReader->setFormat(Format(p_format->getValue()));
    dyna3dReader->setPartSelection(p_Selection->getValue());

    if (!dyna3dReader->examine(rescan)) {
        return false;
    }

    if (p_only_geometry->getValue()) {
        setTimesteps(0);
    } else if (dyna3dReader->numTimesteps() >= 0) {
        setTimesteps(dyna3dReader->numTimesteps());
    }
    setPartitions(dyna3dReader->numBlocks());

    return true;
}

bool ReadDyna3D::finishRead()
{
    assert(dyna3dReader);

    return dyna3dReader->readFinish() == 0;
}

Dyna3DReaderBase *ReadDyna3D::newReader()
{
    auto reader = new Dyna3DReader<4>(this);

    reader->setFilename(p_data_path->getValue());

    reader->setByteswap(ByteSwap(p_byteswap->getValue()));
    reader->setFormat(Format(p_format->getValue()));

    reader->setPartSelection(p_Selection->getValue());

    reader->setPorts(p_grid, p_data_1, p_data_2);

    return reader;
}

MODULE_MAIN(ReadDyna3D)
