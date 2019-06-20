/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

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

#include <util/enum.h>

using namespace vistle;

#define tauio_1 tauio_

//-------------------------------------------------------------------------
static const char *colornames[10] = {
    "yellow", "red", "blue", "green",
    "violet", "chocolat", "linen", "white",
    "crimson", "LightGoldenrod"
};

DEFINE_ENUM_WITH_STRING_CONVERSIONS(NodalDataType,
                                    (No_Node_Data)
                                    (Displacements)
                                    (Velocities)
                                    (Accelerations)
                                    )

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ElementDataType,
                                    (No_Element_Data)
                                    (Stress_Tensor)
                                    (Plastic_Strain)
                                    (Thickness)
                                    )

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Component,
                                    (Sx)(Sy)(Sz)(Txy)(Tyz)(Txz)(Pressure)(Von_Mises)
                                    )

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Format, (cadfem)(original))
// format of cadfem
DEFINE_ENUM_WITH_STRING_CONVERSIONS(CadfemFormat, (GERMAN)(US))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ByteSwap, (Off)(On)(Auto))


// Constructor
ReadDyna3D::ReadDyna3D(const std::string &name, int moduleID, mpi::communicator comm)
: vistle::Reader("Read LS-Dyna3D ptf files", name, moduleID, comm)
{

    const char *nodalDataType_label[4] = {
        "None", "Displacements", "Velocities",
        "Accelerations"
    };
    const char *elementDataType_label[4] = { "None", "StressTensor", "PlasticStrain", "Thickness" };
    const char *component_label[8] = {
        "Sx", "Sy", "Sz", "Txy", "Tyz", "Txz",
        "Pressure", "Von Mises"
    };
    const char *format_label[2] = { "cadfem", "original" };

    const char *byteswap_label[3] = { "off", "on", "auto" };

    // Output ports
    p_grid = createOutputPort("grid_out", "grid");
    p_data_1 = createOutputPort("data1_out", "vector data");
    p_data_2 = createOutputPort("data2_out", "scalar data");

    // Parameters
    p_data_path = addStringParameter("filename", "Geometry file path", "/data/general/examples/Dyna3d/aplot", Parameter::Filename);
    p_nodalDataType = addIntParameter("nodalDataType", "Nodal results data to be read", No_Node_Data, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_nodalDataType, NodalDataType);
    p_elementDataType = addIntParameter("elementDataType", "Element results data to be read", No_Element_Data, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_elementDataType, ElementDataType);
    p_component = addIntParameter("component", "stress tensor component", Von_Mises, Parameter::Choice);
    V_ENUM_SET_CHOICES(p_component, Component);
    p_Selection = addStringParameter("Selection", "Number selection for parts", "all");
    // p_State = addIntSliderParam("State","Timestep");
    p_format = addIntParameter("format", "Format of LS-DYNA3D ptf-File", original, Parameter::Choice);
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

ReadDyna3D::~ReadDyna3D() {
}


bool ReadDyna3D::prepareRead() {

    if (!dyna3dReader) {
        dyna3dReader.reset(newReader());
    }

    return dyna3dReader->readStart() == 0;
}

bool ReadDyna3D::read(Reader::Token &token, int timestep, int block) {

    auto &reader = dyna3dReader;
    std::cerr << "reading: token: " << token << ", time=" << timestep << ", block=" << block << std::endl;

    if (p_only_geometry->getValue()) {
        return reader->readOnlyGeo(token, block) == 0;
    } else {
        if (timestep == -1)
            return true;
        return reader->readState(token, timestep+1, block) == 0;
    }

    return true;
}

bool ReadDyna3D::examine(const Parameter *param) {

    std::cerr << "ReadDyna3D::examine(param=" << param->getName() << ")" << std::endl;

    dyna3dReader.reset(newReader());
    assert(dyna3dReader);

    dyna3dReader->setPartSelection(p_Selection->getValue());

    if (!dyna3dReader->examine()) {
        return false;
    }

    if (p_only_geometry->getValue()) {
        setTimesteps(0);
    } else {
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

Dyna3DReaderBase *ReadDyna3D::newReader() {

    auto reader = new Dyna3DReader<4>(this);

    reader->setFilename(p_data_path->getValue());

    switch (p_byteswap->getValue()) {
    case Off:
        reader->setByteswap(Dyna3DReaderBase::BYTESWAP_OFF);
        break;
    case On:
        reader->setByteswap(Dyna3DReaderBase::BYTESWAP_ON);
        break;
    case Auto:
        reader->setByteswap(Dyna3DReaderBase::BYTESWAP_AUTO);
        break;
    }

    switch (p_format->getValue()) {
    case cadfem:
        reader->setFormat(Dyna3DReaderBase::GERMAN);
        break;
    case original:
        reader->setFormat(Dyna3DReaderBase::US);
        break;
    }

    reader->setPorts(p_grid, p_data_1, p_data_2);

    return reader;
}

MODULE_MAIN(ReadDyna3D)
