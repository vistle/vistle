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

//#include <appl/ApplInterface.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include "Dyna3DReader.h"
#include <util/byteswap.h>
#include <core/unstr.h>
#include <iostream>
#include <util/filesystem.h>

namespace fs = vistle::filesystem;
using namespace vistle;

#ifdef _WIN32
const int OpenFlags = O_RDONLY | O_BINARY;
#else
const int OpenFlags = O_RDONLY;
#endif


#if 0
#define tauio_1 tauio_

//-------------------------------------------------------------------------
static const char *colornames[10] = {
    "yellow", "red", "blue", "green",
    "violet", "chocolat", "linen", "white",
    "crimson", "LightGoldenrod"
};
#endif
/*
// GLOBAL VARIABLES
char InfoBuf[1000];

int NumIDs;
int nodalDataType;
int elementDataType;
int component;
int ExistingStates = 0;
int MinState = 1;
int MaxState = 1;
int State    = 1;

int IDLISTE[MAXID];
int *numcoo, *numcon, *numelem;

int *delElem = NULL;
int *delCon  = NULL;

Element** solidTab  = NULL;
Element** tshellTab = NULL;
Element** shellTab  = NULL;
Element** beamTab   = NULL;

// storing coordinate positions for all time steps
int **coordLookup = NULL;

// element and connection list for time steps (considering the deletion table)
int **My_elemList = NULL;
int **conList  = NULL;

int *DelTab = NULL;  // node/element deletion table

coDoUnstructuredGrid*	  grid_out    = NULL;
coDoVec3* Vertex_out  = NULL;
coDoFloat* Scalar_out = NULL;

coDoUnstructuredGrid**  grids_out        = NULL;
coDoVec3** Vertexs_out  = NULL;
coDoFloat** Scalars_out = NULL;

coDoSet* grid_set_out,    *grid_timesteps;
coDoSet*	Vertex_set_out,  *Vertex_timesteps;
coDoSet*	Scalar_set_out, *Scalar_timesteps;

coDoSet*	grid_sets_out[MAXTIMESTEPS];
coDoSet*	Vertex_sets_out[MAXTIMESTEPS];
coDoSet*	Scalar_sets_out[MAXTIMESTEPS];
*/

/* kern.f -- translated by f2c (version 19950110).*/

/*
struct {
  int itrecin, nrin, nrzin, ifilin, itrecout, nrout, nrzout, ifilout, irl, iwpr;
  float tau[512];
} tauio_;

int infile;
int numcoord;
int *NodeIds=NULL;
int *SolidNodes=NULL;
int *SolidMatIds=NULL;
int *BeamNodes=NULL;
int *BeamMatIds=NULL;
int *ShellNodes=NULL;
int *ShellMatIds=NULL;
int *TShellNodes=NULL;
int *TShellMatIds=NULL;
int *SolidElemNumbers=NULL;
int *BeamElemNumbers=NULL;
int *ShellElemNumbers=NULL;
int *TShellElemNumbers=NULL;
int *Materials=NULL;

// Common Block Declarations
// global scope !!! attention to side-effects !!!
float *Coord      = NULL;           // Coordinates
float *DisCo      = NULL;           // displaced coordinates
float *NodeData   = NULL;
float *SolidData    = NULL;
float *TShellData = NULL;
float *ShellData  = NULL;
float *BeamData   = NULL;

char CTauin[300];
char CTrace, CEndin, COpenin;
float TimestepTime    = 0.0;
int NumSolidElements  = 0;
int NumBeamElements   = 0;
int NumShellElements  = 0;
int NumTShellElements = 0;
int NumMaterials=0;
int NumDim=0;
int NumGlobVar=0;
int NumWords=0;
int ITFlag=0;
int IUFlag=0;
int IVFlag=0;
int IAFlag=0;
int MDLOpt=0;
int IStrn=0;
int NumSolidMat=0;
int NumSolidVar=0;
int NumAddSolidVar=0;
int NumBeamMat=0;
int NumBeamVar=0;
int NumAddBeamVar=0;
int NumShellMat=0;
int NumShellVar=0;
int NumAddShellVar=0;
int NumTShellMat=0;
int NumTShellVar=0;
int NumShellInterpol=0;
int NumInterpol=0;
int NodeAndElementNumbering=0;
int SomeFlags[5];
int NumBRS=0;
int NodeSort=0;
int NumberOfWords=0;
int NumNodalPoints=0;
int IZControll=0;
int IZElements=0;
int IZArb=0;
int IZStat0=0;

// GLOBAL STATIC VARIABLES
// Table of "constant" values
static int c__1 = 1;
static int c__0 = 0;
static int c__13 = 13;
static int c__8 = 8;
static int c__5 = 5;
static int c__4 = 4;
static int c__10 = 10;
static int c__16 = 16;
*/

// Constructor
template<int wordsize, class INTEGER, class REAL>
Dyna3DReader<wordsize, INTEGER, REAL>::Dyna3DReader(vistle::Reader *module)
: Dyna3DReaderBase(module)
{
    m_wordsize = wordsize;
    postInst();
}

template<int wordsize, class INTEGER, class REAL>
Dyna3DReader<wordsize, INTEGER, REAL>::~Dyna3DReader() {

    if (infile >= 0)
        close(infile);
    infile = -1;

    deleteStateData();

    deleteElementLUT();
}

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::postInst()
{
    ExistingStates = 0;
    MinState = 1;
    State = 100;

    delElem = delCon = 0;
    solidTab.clear();
    tshellTab.clear();
    shellTab.clear();
    beamTab.clear();

    coordLookup = 0;

    My_elemList = conList = 0;
    DelTab = 0;

#if 0
    grid_out = 0;
    Vertex_out = 0;
    Scalar_out = 0;

    grids_out = 0;
    Vertexs_out = 0;
    Scalars_out = 0;
#endif

    NodeIds = SolidNodes = SolidMatIds = BeamNodes = BeamMatIds = 0;
    ShellNodes = ShellMatIds = TShellNodes = TShellMatIds = 0;
    SolidElemNumbers = BeamElemNumbers = ShellElemNumbers = 0;
    TShellElemNumbers = Materials = 0;

    Coord = DisCo = NodeData = SolidData = TShellData = ShellData = 0;
    BeamData = 0;

    TimestepTime = 0.0;
    version = 0.0;
    NumSolidElements = NumBeamElements = NumShellElements = 0;
    NumTShellElements = NumMaterials = NumDim = NumGlobVar = NumWords = 0;
    ITFlag = IUFlag = IVFlag = IAFlag = IDTDTFlag = NumTemperature = MDLOpt = IStrn = 0;
    NumSolidMat = 0;
    NumSolidVar = 0;
    NumAddSolidVar = 0;
    NumBeamMat = 0;
    NumBeamVar = 0;
    NumAddBeamVar = 0;
    NumShellMat = 0;
    NumShellVar = 0;
    NumAddShellVar = 0;
    NumTShellMat = 0;
    NumTShellVar = 0;
    NumShellInterpol = 0;
    NumInterpol = 0;
    NodeAndElementNumbering = 0;
    NumBRS = 0;
    NodeSort = 0;
    NumberOfWords = 0;
    NumNodalPoints = 0;
    IZControll = 0;
    IZElements = 0;
    IZArb = 0;
    IZStat0 = 0;
    NumFluidMat = 0;
    NCFDV1 = 0;
    NCFDV2 = 0;
    NADAPT = 0;

    c__1 = 1;
    c__0 = 0;
    c__13 = 13;
    c__8 = 8;
    c__5 = 5;
    c__4 = 4;
    c__10 = 10;
    c__16 = 16;
    numcon = NULL;
    numelem = NULL;
    numcoo = NULL;
}

template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::readStart()
{
    //
    // ...... do work here ........
    //

    numcon = NULL;
    numelem = NULL;
    numcoo = NULL;

    ExistingStates = 0;
    MinState = 1;
    State = 100;

    delElem = delCon = 0;
    solidTab.clear();
    tshellTab.clear();
    shellTab.clear();
    beamTab.clear();

    coordLookup = 0;

    My_elemList = conList = 0;
    DelTab = 0;

#if 0
    grid_out = 0;
    Vertex_out = 0;
    Scalar_out = 0;

    grids_out = 0;
    Vertexs_out = 0;
    Scalars_out = 0;
#endif

    NodeIds = SolidNodes = SolidMatIds = BeamNodes = BeamMatIds = 0;
    ShellNodes = ShellMatIds = TShellNodes = TShellMatIds = 0;
    SolidElemNumbers = BeamElemNumbers = ShellElemNumbers = 0;
    TShellElemNumbers = Materials = 0;

    Coord = DisCo = NodeData = SolidData = TShellData = ShellData = 0;
    BeamData = 0;

    TimestepTime = 0.0;
    NumSolidElements = NumBeamElements = NumShellElements = 0;
    NumTShellElements = NumMaterials = NumDim = NumGlobVar = NumWords = 0;
    ITFlag = IUFlag = IVFlag = IAFlag = IDTDTFlag = NumTemperature = MDLOpt = IStrn = 0;
    NumSolidMat = 0;
    NumSolidVar = 0;
    NumAddSolidVar = 0;
    NumBeamMat = 0;
    NumBeamVar = 0;
    NumAddBeamVar = 0;
    NumShellMat = 0;
    NumShellVar = 0;
    NumAddShellVar = 0;
    NumTShellMat = 0;
    NumTShellVar = 0;
    NumShellInterpol = 0;
    NumInterpol = 0;
    NodeAndElementNumbering = 0;
    NumBRS = 0;
    NodeSort = 0;
    NumberOfWords = 0;
    NumNodalPoints = 0;
    IZControll = 0;
    IZElements = 0;
    IZArb = 0;
    IZStat0 = 0;
    NumFluidMat = 0;
    NCFDV1 = 0;
    NCFDV2 = 0;
    NADAPT = 0;

    c__1 = 1;
    c__0 = 0;
    c__13 = 13;
    c__8 = 8;
    c__5 = 5;
    c__4 = 4;
    c__10 = 10;
    c__16 = 16;

    if (byteswapFlag == BYTESWAP_OFF)
    {
        std::cerr << "Dyna3DReader::readDyna3D info: byteswap off" << std::endl;
    }
    else if (byteswapFlag == BYTESWAP_ON)
    {
        std::cerr << "Dyna3DReader::readDyna3D info: byteswap on" << std::endl;
    }
    else if (byteswapFlag == BYTESWAP_AUTO)
    {
        std::cerr << "Dyna3DReader::readDyna3D info: byteswap auto" << std::endl;
    }
    else
    {
        std::cerr << "Dyna3DReader::readDyna3D info: byteswap unknown error" << std::endl;
    }

#if 0
    //=======================================================================
    // get input parameters and data object name

    // Get file path
    // Covise::get_browser_param("data_path", &data_Path);
    data_Path = p_data_path->getValue();
    // Get nodal output data type
    // Covise::get_choice_param("nodalDataType",&nodalDataType);
    nodalDataType = p_nodalDataType->getValue();
    // Get element output data type
    // Covise::get_choice_param("elementDataType",&elementDataType);
    elementDataType = p_elementDataType->getValue();
    // Get component of stress tensor
    // Covise::get_choice_param("component",&component);
    component = p_component->getValue();

    // Get the selection
    const char *selection;
    // Covise::get_string_param("Selection",&selection);
    selection = p_Selection->getValue();
    sel.add(selection);
#endif

    int i;
    coRestraint sel;
    sel.add(selection);

    // Initilize list of part IDs
    for (i = 0; i < MAXID; i++)
        IDLISTE[i] = 0;

    // Find selected parts per IDs
    int numParts = 0;
    for (i = 0; i < MAXID; i++)
    {
        if ((IDLISTE[i] = sel(i + 1)) > 0) // Notice: offset for ID (= material number - 1)
        {
            numParts++;
        }
    }
    if (numParts == 0)
    {
#ifdef SENDINFO
        m_module->sendError("No parts with selected IDs");
#endif
        return false;
    }

#if 0
    // Get timesteps
    // Covise::get_slider_param("State", &MinState, &MaxState, &State);
    //p_Step->getValue(MinState,MaxState,State);
    MinState = p_Step->getValue(0);
    if (MinState <= 0)
    {
#ifdef SENDINFO
        m_module->sendWarning("Minimum time step must be positive!!");
#endif
        p_Step->setValue(0, 1);
        MinState = 1;
    }
    NumState = p_Step->getValue(1);
    MaxState = MinState + NumState;
    if (MinState > MaxState)
    {
        int temp_swap;
        temp_swap = MaxState;
        MaxState = MinState;
        MinState = temp_swap;
    }
    NumState = MaxState - MinState;
    State = MaxState;
    // if(MinState>State || MaxState<State) State=MaxState;

    // p_Step->setValue(MinState,MaxState,State);
    p_Step->setValue(0, MinState);
    p_Step->setValue(1, MaxState - MinState);

    // Get geometry key
    // Covise::get_boolean_param("only geometry",&key);
    key = p_only_geometry->getValue();

    // Get output object names
    // Grid = Covise::get_object_name("grid");
    Grid = p_grid->getObjName();

    if (nodalDataType > 0)
    {
        // displacements
        // velocities
        // accelerations
        // Vertex = Covise::get_object_name("data_1");
        Vertex = p_data_1->getObjName();
    }
    if (elementDataType > 0)
    {
        // stress tensor
        // plastic strain
        // Scalar = Covise::get_object_name("data_2");
        Scalar = p_data_2->getObjName();
    }

    //=======================================================================
#endif

    /* Local variables */
    char buf[256];

#if 0
    for (INTEGER i = 0; i < MAXTIMESTEPS; i++)
    {
        grid_sets_out[i] = NULL;
        Vertex_sets_out[i] = NULL;
        Scalar_sets_out[i] = NULL;
    }
#endif
    NodeIds = NULL;
    SolidNodes = NULL;
    SolidMatIds = NULL;
    BeamNodes = NULL;
    BeamMatIds = NULL;
    ShellNodes = NULL;
    ShellMatIds = NULL;
    TShellNodes = NULL;
    TShellMatIds = NULL;
    SolidElemNumbers = NULL;
    BeamElemNumbers = NULL;
    ShellElemNumbers = NULL;
    TShellElemNumbers = NULL;
    Materials = NULL;
#if 0
    Vertex_timesteps = NULL;
    Scalar_timesteps = NULL;
#endif

    /* initialize TAURUS access */
    taurusinit_();

#if 0
    // Covise::get_choice_param("format", &format);
    format = p_format->getValue();
#endif

    /* read TAURUS control data */
    rdtaucntrl_(format);

    /* read TAURUS node coordinates */
    rdtaucoor_();

    /* read TAURUS elements */
    rdtauelem_(format);

    /* read user node and element numbers */
    rdtaunum_(format);

    // initialize
    ExistingStates = 0;

    createElementLUT();
    createGeometryList();

    return 0;
}

template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::readState(vistle::Reader::Token &token, int istate, int block)
{
    if (CTrace == 'Y')
    {
        fprintf(stderr, "* test existence of state: %6d\n", istate);
    }

#if 0
    // only reading when lying in timestep interval
    if ((istate >= MinState) && (istate <= State))
    {
#endif
        int flag = rdstate_(token, istate, block);
        //	  std::cerr << "Dyna3DReader::readDyna3D() istate : " << istate << " flag: " << flag <<  std::endl;
        if (flag == -1)
            return (-1);

#if 0
    }
    else
    {
        // open next file (!! Assume: number of files and time steps correspond !!)
        if (otaurusr_() < 0)
            return -1;
        if (CEndin == 'N')
            ExistingStates++;
        else
            return 0;
    }
#endif
#if 0
    // mark end of set array
    grid_sets_out[(State - MinState) + 1] = NULL;
    Vertex_sets_out[(State - MinState) + 1] = NULL;
    Scalar_sets_out[(State - MinState) + 1] = NULL;

    grid_timesteps = new coDoSet(Grid, (coDistributedObject **)(void *)grid_sets_out);
    sprintf(buf, "%d %d", MinState, State);
    grid_timesteps->addAttribute("TIMESTEP", buf);
    p_grid->setCurrentObject(grid_timesteps);

    if (nodalDataType > 0)
    {
        Vertex_timesteps = new coDoSet(Vertex, (coDistributedObject **)(void *)Vertex_sets_out);
        Vertex_timesteps->addAttribute("TIMESTEP", buf);
        p_data_1->setCurrentObject(Vertex_timesteps);
    }

    if (elementDataType > 0)
    {
        Scalar_timesteps = new coDoSet(Scalar, (coDistributedObject **)(void *)Scalar_sets_out);
        Scalar_timesteps->addAttribute("TIMESTEP", buf);
        p_data_2->setCurrentObject(Scalar_timesteps);
    }

    // update timestep slider
    if (MaxState > (ExistingStates + 1))
    {
        MaxState = ExistingStates + 1;
        // Covise::update_slider_param("State",1,State,MaxState);
        // p_State->setValue(1,State,MaxState);
        // p_Step->setValue(MinState,State,MaxState);
        p_Step->setValue(0, MinState);
        p_Step->setValue(1, State - MinState);
        MaxState = State;
        NumState = State - MinState;
    }
    if (State > MaxState)
    {
        State = MaxState;
        // Covise::update_slider_param("State",1,State,MaxState);
        // p_State->setValue(1,State,MaxState);
        // p_Step->setValue(MinState,State,MaxState);
        p_Step->setValue(0, MinState);
        p_Step->setValue(1, State - MinState);
        MaxState = State - MinState;
    }

    // free
    for (i = 0; i < State - MinState + 1; i++)
    {
        delete grid_sets_out[i];
        delete Vertex_sets_out[i];
        delete Scalar_sets_out[i];

        grid_sets_out[i] = NULL;
        Vertex_sets_out[i] = NULL;
        Scalar_sets_out[i] = NULL;
    }
#endif

    return 0;
}

template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::readOnlyGeo(Reader::Token &token, int blockToRead)
{
    std::cerr  << "Dyna3DReader::readOnlyGeo(block=" << blockToRead << ")" << std::endl;
    createGeometry(token, blockToRead);
#if 0
    grid_sets_out[0] = grid_set_out;
    grid_timesteps = new coDoSet(Grid, (coDistributedObject **)(void *)grid_sets_out);

    // free
    delete grid_sets_out[0];
    grid_sets_out[0] = NULL;
#endif

    return 0;
}

template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::readFinish()
{
    m_currentTimestep = -1;

    deleteElementLUT();

    deleteStateData();

    delete[] Coord;
    Coord = nullptr;
    delete[] NodeIds;
    NodeIds = nullptr;
    delete[] SolidNodes;
    SolidNodes = nullptr;
    delete[] SolidMatIds;
    SolidMatIds = nullptr;
    delete[] BeamNodes;
    BeamNodes = nullptr;
    delete[] BeamMatIds;
    BeamMatIds = nullptr;
    delete[] ShellNodes;
    ShellNodes = nullptr;
    delete[] ShellMatIds;
    ShellMatIds = nullptr;
    delete[] TShellNodes;
    TShellNodes = nullptr;
    delete[] TShellMatIds;
    TShellMatIds = nullptr;
    delete[] Materials;
    Materials = nullptr;

    delete[] SolidElemNumbers;
    SolidElemNumbers = nullptr;
    delete[] BeamElemNumbers;
    BeamElemNumbers = nullptr;
    delete[] ShellElemNumbers;
    ShellElemNumbers = nullptr;
    delete[] TShellElemNumbers;
    TShellElemNumbers = nullptr;

    delete[] matTyp;
    matTyp = nullptr;
    delete[] MaterialTables;
    MaterialTables = nullptr;

#if 0
    //free
    //
    for (INTEGER i = 0; i < NumIDs; i++)
    {
        delete[] My_elemList[i];
        delete[] conList[i];
        delete[] coordLookup[i];
    }
    delete[] My_elemList;
    delete[] conList;
    delete[] coordLookup;

    delete[] numcon;
    delete[] numelem;
    delete[] numcoo;

    for (INTEGER i = 0; i < NumSolidElements; i++)
        delete solidTab[i];
    delete[] solidTab;
    solidTab = nullptr;

    for (INTEGER i = 0; i < NumTShellElements; i++)
        delete tshellTab[i];
    delete[] tshellTab;
    tshellTab = nullptr;

    for (INTEGER i = 0; i < NumShellElements; i++)
        delete shellTab[i];
    delete[] shellTab;
    shellTab = nullptr;

    for (INTEGER i = 0; i < NumBeamElements; i++)
        delete beamTab[i];
    delete[] beamTab;
    beamTab = nullptr;

    delete[] delElem;
    delete[] delCon;

    //  delete grid_timesteps;
    //  delete Vertex_timesteps;
    //  delete Scalar_timesteps;

    delete[] Coord;
    delete[] NodeIds;
    delete[] SolidNodes;
    delete[] SolidMatIds;
    delete[] BeamNodes;
    delete[] BeamMatIds;
    delete[] ShellNodes;
    delete[] ShellMatIds;
    delete[] TShellNodes;
    delete[] TShellMatIds;
    delete[] Materials;

    delete[] SolidElemNumbers;
    delete[] BeamElemNumbers;
    delete[] ShellElemNumbers;
    delete[] TShellElemNumbers;

    delete[] matTyp;
    delete[] MaterialTables;
    matTyp = NULL;
    MaterialTables = NULL;
#endif

    return (0);

} /* readDyna3D() */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::taurusinit_()
{
    /* Local variables */

    /*------------------------------------------------------------------------------*/
    /*     initialize TAURUS access */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    /* g77  INTEGER ITAU(128) */
    /* iris INTEGER ITAU(512) */

    /* g77 4                TAU(128) */
    /* iris4                TAU(512) */

    /* ITermr = 5;
      ITermw = 6;
      ISesi = 15;
      ISeso = 16;
      IErr = 17;
      IMatdb = 18; */
    COpenin = 'N';
    CEndin = 'N';
    //CTrace = 'Y';
    CTrace = 'N';

    //tauio_1.irl = 512;
    /* g77 IRL     = 128 */
    /* iris IRL     = 512 */

    //tauio_1.iwpr = tauio_1.irl;
    /* g77 IWPR    = IRL * 4 */
    /* iris IWPR    = IRL */

    return 0;
} /* taurusinit_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::rdtaucntrl_(FORMAT format)
{
    /* Local variables */
    int izioshl1, izioshl2, izioshl3, izioshl4, idum[20], iznumelb,
        iznumelh, izmaxint, iznumels, iznumelt;
    int izndim, iziaflg, iznmatb, izneiph, izitflg, iziuflg,
        izivflg, iznglbv, iznmath, iznvarb, iznarbs, iznvarh, izneips,
        iznmats, iznmatt, iznvars, iznvart;

    /*------------------------------------------------------------------------------*/
    /*     read TAURUS control block */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    int iznver = 15;
    if (rdrecr_(&version, &iznver, 1) < 0)
        return -1;

    // Dumb auto detection. Checks if the version number of LSDYNA
    // has a sensible value. CAN FAIL!

    if ((byteswapFlag == BYTESWAP_AUTO) && ((version < 1) || (version > 10000)))
    {

        std::cerr << "Dyna3DReader::rdtaucntrl_ info: autobyteswap: "
             << "asuming byteswap on (" << version << ")"
             << std::endl;

        byteswapFlag = BYTESWAP_ON;
        unsigned int *buffer = (unsigned int *)(void *)(tauio_1.tau);
        byteswap(buffer, tauio_1.taulength);

        if (rdrecr_(&version, &iznver, 1) < 0)
            return -1;
    }
    else if (byteswapFlag == BYTESWAP_AUTO)
    {
        std::cerr << "Dyna3DReader::rdtaucntrl_ info: autobyteswap: asuming byteswap off"
             << std::endl;
        byteswapFlag = BYTESWAP_OFF;
    }

    /* number of dimensions */
    izndim = 16;
    if (rdreci_(&NumDim, &izndim, &c__1, format) < -1)
        return -1;
    if (NumDim == 4)
    {
        NumDim = 3;
    }
    else if (NumDim > 4)
    {
        std::cerr << "Dyna3DReader::rdtaucntrl_ info: unhandled NumDim=" << NumDim
             << std::endl;
    }

    /* number of nodal points */
    NumNodalPoints = 17;
    rdreci_(&numcoord, &NumNodalPoints, &c__1, format);
    /* number of global variable */
    iznglbv = 19;
    rdreci_(&NumGlobVar, &iznglbv, &c__1, format);
    /* flags */
    /*   flag for temp./node varb */
    izitflg = 20;
    rdreci_(&ITFlag, &izitflg, &c__1, format);
    /*   flag for geometry */
    iziuflg = 21;
    rdreci_(&IUFlag, &iziuflg, &c__1, format);
    /*   flag for velocities */
    izivflg = 22;
    rdreci_(&IVFlag, &izivflg, &c__1, format);
    /*   flag for accelerations */
    iziaflg = 23;
    rdreci_(&IAFlag, &iziaflg, &c__1, format);

    /* solid elements */
    /*   number of solid elements */
    iznumelh = 24;
    rdreci_(&NumSolidElements, &iznumelh, &c__1, format);
    /*   number of solid element materials */
    iznmath = 25;
    rdreci_(&NumSolidMat, &iznmath, &c__1, format);

    /*   number of variables in database for each brick element */
    iznvarh = 28;
    rdreci_(&NumSolidVar, &iznvarh, &c__1, format);
    //std::cerr << "NVARH: " << NumSolidVar << std::endl;

    /* beam elements */
    /*   number of beam elements */
    iznumelb = 29;
    rdreci_(&NumBeamElements, &iznumelb, &c__1, format);
    /*   number of beam element materials */
    iznmatb = 30;
    rdreci_(&NumBeamMat, &iznmatb, &c__1, format);

    /*   number of variables in database for each 1D element */
    iznvarb = 31;
    rdreci_(&NumBeamVar, &iznvarb, &c__1, format);
    //std::cerr << "NVARB: " << NumBeamVar << std::endl;

    /* shell elements */
    /*   number of shell elements */
    iznumels = 32;
    rdreci_(&NumShellElements, &iznumels, &c__1, format);
    /*   number of shell element materials */
    iznmats = 33;
    rdreci_(&NumShellMat, &iznmats, &c__1, format);

    /*   number of variables in database for each 4 node shells */
    iznvars = 34;
    rdreci_(&NumShellVar, &iznvars, &c__1, format);
    //std::cerr << "NVARS: " << NumShellVar << std::endl;
    /*   number of additional variables per brick element */
    izneiph = 35;
    rdreci_(&NumAddSolidVar, &izneiph, &c__1, format);
    /*   number of additional variables at shell integration points */
    izneips = 36;
    rdreci_(&NumAddShellVar, &izneips, &c__1, format);
    /*   number of integration points dumped for each shell */
    izmaxint = 37;
    rdreci_(&NumShellInterpol, &izmaxint, &c__1, format);
    //    std::cerr << "MAXINT: " << NumShellInterpol << std::endl;
    if (NumShellInterpol >= 0)
    {
        MDLOpt = 0;
#ifdef _AIRBUS
        NumShellInterpol = 0;
#endif
        NumInterpol = NumShellInterpol;
    }
    else
    {
        if (NumShellInterpol < 0)
        {
            MDLOpt = 1;
            NumInterpol = abs(NumShellInterpol);
        }
        if (NumShellInterpol < -10000)
        {
            MDLOpt = 2;
            NumInterpol = abs(NumShellInterpol) - 10000;
        }
    }

    /* node and element numbering */
    /*     NARBS = 10 + NUMNP + NUMELH + NUMELS + NUMELT */
    iznarbs = 40;
    rdreci_(&NodeAndElementNumbering, &iznarbs, &c__1, format);

    //For Dyna3D we have an additional offset
    int offset = 0;
#ifdef _AIRBUS
    if (version == 0.0)
    {
        offset = 6;
    }
#endif

    /* thick shell elements */
    /*   number of thick shell elements */
    iznumelt = 41 + offset;
    rdreci_(&NumTShellElements, &iznumelt, &c__1, format);
    /*   number of thick shell thick shell materials */
    iznmatt = 42 + offset;
    rdreci_(&NumTShellMat, &iznmatt, &c__1, format);
    /*   number of variables in database for each thick shell element */
    iznvart = 43 + offset;
    rdreci_(&NumTShellVar, &iznvart, &c__1, format);
    //std::cerr << "NVART: " << NumTShellVar << std::endl;
    /*   Symmetric stress tensor written flag (1000 = yes) */
    izioshl1 = 44 + offset;
    rdreci_(SomeFlags, &izioshl1, &c__1, format);
    /*   Plastic strain written flag          (1000 = yes) */
    izioshl2 = 45 + offset;
    rdreci_(&SomeFlags[1], &izioshl2, &c__1, format);
    /*   Shell force resultants written flag  (1000 = yes) */
    izioshl3 = 46 + offset;
    rdreci_(&SomeFlags[2], &izioshl3, &c__1, format);
    /*   Shell thickness, energy + 2 others   (1000 = yes) */
    izioshl4 = 47 + offset;
    rdreci_(&SomeFlags[3], &izioshl4, &c__1, format);
    if (SomeFlags[0] > 0)
    {
        SomeFlags[0] = 1;
    }
    if (SomeFlags[1] > 0)
    {
        SomeFlags[1] = 1;
    }
    if (SomeFlags[2] > 0)
    {
        SomeFlags[2] = 1;
    }
    if (SomeFlags[3] > 0)
    {
        SomeFlags[3] = 1;
    }
    int position;
    position = 48 + offset;
    rdreci_(&NumFluidMat, &position, &c__1, format);
    position = 49 + offset;
    rdreci_(&NCFDV1, &position, &c__1, format);
    position = 50 + offset;
    rdreci_(&NCFDV2, &position, &c__1, format);
    position = 51 + offset;
    rdreci_(&NADAPT, &position, &c__1, format);
    position = 57 + offset;
    rdreci_(&IDTDTFlag, &position, &c__1, format);

    /*
   std::cerr << "IOSHL(1): " << SomeFlags[0] << std::endl;
   std::cerr << "IOSHL(2): " << SomeFlags[1] << std::endl;
   std::cerr << "IOSHL(3): " << SomeFlags[2] << std::endl;
   std::cerr << "IOSHL(4): " << SomeFlags[3] << std::endl;
   */

    ctrlLen_ = 64;
    if (NumDim == 5)
    {
        ctrlLen_ += 2;
        int off_nrlelem = 65;
        int off_nmmat = off_nrlelem + 1;
        rdreci_(&nrelem, &off_nrlelem, &c__1, format);
        rdreci_(&nmmat, &off_nmmat, &c__1, format);
        ctrlLen_ += nmmat;
        matTyp = new int[nmmat];
        int off_listMatTyp = off_nmmat + 1;
        rdreci_(matTyp, &off_listMatTyp, &nmmat, format);
    }

    if (NumInterpol > 3)
        IStrn = abs(NumShellVar - NumInterpol * (NumAddShellVar + 7) - 12) / 12;
    else
        IStrn = abs(NumShellVar - 3 * (NumAddShellVar + 7) - 12) / 12;

#ifdef _AIRBUS

    if (NumShellElements > 0)
    {
        IStrn = 1;
    }
#endif

    // Calculate the number of temperature/flux/mass-scaling values per node based on ITFlag and IDTDTFlag.
    NumTemperature = ITFlag % 10 + IDTDTFlag;
    if (ITFlag % 10 == 2)
        NumTemperature += 2;
    else if (ITFlag % 10 == 3)
        NumTemperature += 3;
    if (ITFlag >= 10)
        ++NumTemperature;

    /* calculate pointers */
    IZControll = 1;
    NumNodalPoints = IZControll + ctrlLen_;
    IZElements = NumNodalPoints + numcoord * 3;
    IZArb = IZElements + (NumSolidElements + NumTShellElements) * 9 + NumBeamElements * 6 + NumShellElements * 5;
    NumBRS = 0;
    NumMaterials = 0;
    if (NodeAndElementNumbering > 0)
    {
        rdreci_(&NodeSort, &IZArb, &c__1, format);
        if (NodeSort >= 0)
        {
            IZStat0 = IZArb + 10 + numcoord + NumSolidElements + NumBeamElements + NumShellElements + NumTShellElements;
        }
        else
        {
            rdreci_(idum, &c__0, &c__13, format);
            rdreci_(&NumBRS, &c__0, &c__1, format);
            rdreci_(&NumMaterials, &c__0, &c__1, format);
            IZStat0 = IZArb + 16 + numcoord + NumSolidElements + NumBeamElements + NumShellElements + NumTShellElements + NumMaterials * 3;
            // @@@ take a look at material tables
            /*
                     int matLength = NumMaterials * 3;
                     MaterialTables = new int[matLength];
                     int IZ_MatTables = IZStat0 - matLength;
                     rdreci_(MaterialTables,&IZ_MatTables,&matLength,format);
                     delete [] MaterialTables;
         */
        }
    }
    else
    {
        IZStat0 = IZArb;
    }
    m_numBlocks = NumMaterials;

    /* write control data to output file */
    // not implemented

    // Info into mapeditor
#ifdef SENDINFO
    m_module->sendInfo("File '%s': %i nodal points, %i solid elements, %i thick shell elements, %i shell elements, %i beam elements",
             m_filename.c_str(), numcoord, NumSolidElements, NumTShellElements, NumShellElements, NumBeamElements);
#endif

    /* TAURUS geometry length (number of words) */
    taugeoml_();
    /* TAURUS state length (number of words) */
    taustatl_();

    fprintf(stderr, "* LSDYNA version                   %g\n", version);
    fprintf(stderr, "* number of dimensions             %d\n", NumDim);
    fprintf(stderr, "* number of nodes                  %d\n", numcoord);
    fprintf(stderr, "* NARBS                            %d\n", NodeAndElementNumbering);
    fprintf(stderr, "* number of solids                 %d\n", NumSolidElements);
    fprintf(stderr, "* number of 2 node beams           %d\n", NumBeamElements);
    fprintf(stderr, "* number of 4 node shells          %d\n", NumShellElements);
    fprintf(stderr, "* number of thick shell elements   %d\n", NumTShellElements);
    fprintf(stderr, "* variables (solids)               %d\n", NumSolidVar);
    fprintf(stderr, "* variables (node beams)           %d\n", NumBeamVar);
    fprintf(stderr, "* variables (4 node shells)        %d\n", NumShellVar);
    fprintf(stderr, "* variables (thick shell elements) %d\n", NumTShellVar);
    fprintf(stderr, "* NGLBV                            %d\n", NumGlobVar);
    fprintf(stderr, "* ITFLG                            %d\n", ITFlag);
    fprintf(stderr, "* IUFLG                            %d\n", IUFlag);
    fprintf(stderr, "* IVFLG                            %d\n", IVFlag);
    fprintf(stderr, "* IAFLG                            %d\n", IAFlag);
    fprintf(stderr, "* NEIPH                            %d\n", NumAddSolidVar);
    fprintf(stderr, "* NEIPS                            %d\n", NumAddShellVar);
    fprintf(stderr, "* ISTRN                            %d\n", IStrn);
    fprintf(stderr, "* IOSHL(1)                         %d\n", SomeFlags[0]);
    fprintf(stderr, "* IOSHL(2)                         %d\n", SomeFlags[1]);
    fprintf(stderr, "* IOSHL(3)                         %d\n", SomeFlags[2]);
    fprintf(stderr, "* IOSHL(4)                         %d\n", SomeFlags[3]);
    fprintf(stderr, "* MAXINT                           %d\n", NumShellInterpol);
    fprintf(stderr, "* NSORT                            %d\n", NodeSort);
    fprintf(stderr, "* NUMBRS                           %d\n", NumBRS);
    fprintf(stderr, "* NMMAT                            %d\n", NumMaterials);
    fprintf(stderr, "* IZCNTRL                          %d\n", IZControll);
    fprintf(stderr, "* IZNUMNP                          %d\n", NumNodalPoints);
    fprintf(stderr, "* IZELEM                           %d\n", IZElements);
    fprintf(stderr, "* IZARB                            %d\n", IZArb);
    fprintf(stderr, "* MDLOPT                           %d\n", MDLOpt);
    fprintf(stderr, "* IDTDTFLG                         %d\n", IDTDTFlag);
    fprintf(stderr, "* NUMRBE                           %d\n", nrelem);
    fprintf(stderr, "* NUMMAT                           %d\n", nmmat);

    fprintf(stderr, "* number of geometry words         %d\n", NumberOfWords);
    fprintf(stderr, "* number of state words            %d\n", NumWords);

    return 0;

} /* rdtaucntrl_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::taugeoml_()
{
    /*------------------------------------------------------------------------------*/
    /*     TAURUS geometry length (number of words) */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    NumberOfWords = 0;
    /*  Control block and title */
    NumberOfWords += ctrlLen_;
    /* Nodal coordinates */
    NumberOfWords += numcoord * 3;
    /* Solid topology */
    NumberOfWords += NumSolidElements * 9;
    /* Thick shell topology */
    NumberOfWords += NumTShellElements * 9;
    /* Beam topology */
    NumberOfWords += NumBeamElements * 6;
    /* Shell topology */
    NumberOfWords += NumShellElements * 5;
    /* Arbitary numbering tables */
    NumberOfWords += NodeAndElementNumbering;

    if (CTrace == 'Y')
    {
        fprintf(stderr, "* TAURUS geometry length NWORDG: %d\n", NumberOfWords);
    }

    return 0;
} /* taugeoml_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::taustatl_()
{
    /* Local variables */
    int nwordbe, nwordhe, nwordse, nwordte;

    /*------------------------------------------------------------------------------*/
    /*     TAURUS state length (number of words) */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    if (CTrace == 'Y')
    {
        fprintf(stderr, "* INTPNTS = %d\n", NumInterpol);
    }

    NumWords = 0;
    ++NumWords;
    NumWords += NumGlobVar;
    NumWords += numcoord * NumTemperature;
    NumWords += numcoord * 3 * IUFlag;
    NumWords += numcoord * 3 * IVFlag;
    NumWords += numcoord * 3 * IAFlag;
    /* solid elements */
    nwordhe = NumSolidElements * NumSolidVar;
    NumWords += nwordhe;
    /* thick shell elements */
    // new evaluation of NumTShellVar - why? -> uncomment 11.12.97
    if (IStrn < 1)
    {
        //NumTShellVar = (SomeFlags[0] * 6 + SomeFlags[1] + NumAddShellVar) * NumInterpol + IStrn * 12;
        nwordte = NumTShellElements * NumTShellVar;
    }
    else
    {
        //NumTShellVar = (SomeFlags[0] * 6 + SomeFlags[1] + NumAddShellVar) * NumInterpol + 1 * 12;
        nwordte = NumTShellElements * NumTShellVar;
    }
    NumWords += nwordte;
    /* beam elements */
    nwordbe = NumBeamElements * NumBeamVar;
    NumWords += nwordbe;
    /* shell elements */
    nwordse = (NumShellElements - nrelem) * NumShellVar;
    NumWords += nwordse;

    if (NumShellInterpol < 0)
    {
        if (MDLOpt == 1)
        {
            NumWords += numcoord;
        }
        if (MDLOpt == 2)
        {
            NumWords = NumWords + NumSolidElements + NumTShellElements + NumShellElements + NumBeamElements;
        }
    }

    if (CTrace == 'Y')
    {
        fprintf(stderr, "* words for SolidE   elements: %d\n", nwordhe);
        fprintf(stderr, "* words for TSHELL elements: %d\n", nwordte);
        fprintf(stderr, "* words for BEAM   elements: %d\n", nwordbe);
        fprintf(stderr, "* words for SHELL  elements: %d\n", nwordse);
        fprintf(stderr, "* TAURUS state length:           %d\n", NumWords);
    }

    return 0;
} /* taustatl_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::rdtaucoor_()
{

    /* Local variables */
    int nxcoor;

    /*------------------------------------------------------------------------------*/
    /*     read TAURUS node coordinates */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    if (CTrace == 'Y')
    {
        fprintf(stderr, "* read node coordinates\n");
    }

    nxcoor = numcoord * 3;
    Coord = new float[nxcoor];

#ifndef _AIRBUS
    rdrecr_(Coord, &NumNodalPoints, nxcoor);
#else
    if (version != 0.0)
    {
        rdrecr_(Coord, &NumNodalPoints, nxcoor);
    }
    else
    {
        NumNodalPoints += 3;

        //rdrecr_(Coord, &NumNodalPoints, nxcoor);
        int ic = 78;

        rdrecr_(Coord, &NumNodalPoints, ic);

        float dummy;
        int i;
        int cr = 79;

        ic = 612;

        while (cr < numcoord)
        {
            tauio_1.nrin++;
            rdrecr_(Coord + cr * 3, &NumNodalPoints, ic);
            cr += 204;
        }
    }
#endif

    return 0;
} /* rdtaucoor_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::rdtauelem_(FORMAT format)
{

    /* Local variables */
    int i;

    /*------------------------------------------------------------------------------*/
    /*     read TAURUS elements */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    if (CTrace == 'Y')
    {
        fprintf(stderr, "* read elements\n");
    }

    /* place pointer to address of 1st element */
    placpnt_(&IZElements);
    /* read solid elements */
    if (NumSolidElements > 0)
    {
        SolidNodes = new int[NumSolidElements * 8];
        SolidMatIds = new int[NumSolidElements];
        for (i = 0; i < NumSolidElements; i++)
        {
            rdreci_(SolidNodes + (i << 3), &c__0, &c__8, format);
            rdreci_(SolidMatIds + i, &c__0, &c__1, format);
        }
    }
    /* read thick shell elements */
    if (NumTShellElements > 0)
    {
        TShellNodes = new int[NumTShellElements * 8];
        TShellMatIds = new int[NumTShellElements];
        for (i = 0; i < NumTShellElements; i++)
        {
            rdreci_(TShellNodes + (i << 3), &c__0, &c__8, format);
            rdreci_(TShellMatIds + i, &c__0, &c__1, format);
        }
    }
    /* read beam elements */
    if (NumBeamElements > 0)
    {
        BeamNodes = new int[NumBeamElements * 5];
        BeamMatIds = new int[NumBeamElements];
        for (i = 0; i < NumBeamElements; i++)
        {
            rdreci_(BeamNodes + (i * 5), &c__0, &c__5, format);
            rdreci_(BeamMatIds + i, &c__0, &c__1, format);
        }
    }
    /* read shell elements */
    if (NumShellElements > 0)
    {
        ShellNodes = new int[NumShellElements * 4];
        ShellMatIds = new int[NumShellElements];
        for (i = 0; i < NumShellElements; i++)
        {
            rdreci_(ShellNodes + (i << 2), &c__0, &c__4, format);
            rdreci_(ShellMatIds + i, &c__0, &c__1, format);
        }
    }

    // why uncomment?
    /*    rtau_.nel = NumSolidElements + NumBeamElements + NumShellElements + NumTShellElements;*/

    return 0;
} /* rdtauelem_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::rdtaunum_(FORMAT format)
{
    /* Local variables */
    float rdum[20];
    /*int i,j,ie;*/

    /*------------------------------------------------------------------------------*/
    /*     read user node and element numbers */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    if (NodeAndElementNumbering > 0)
    {
        if (CTrace == 'Y')
        {
            fprintf(stderr, "* read user ids\n");
        }
        /* place pointer to address */
        placpnt_(&IZArb);
        if (NumSolidElements > 0)
            SolidElemNumbers = new int[NumSolidElements];
        if (NumBeamElements > 0)
            BeamElemNumbers = new int[NumBeamElements];
        if (NumShellElements > 0)
            ShellElemNumbers = new int[NumShellElements];
        if (NumTShellElements > 0)
            TShellElemNumbers = new int[NumTShellElements];
        NodeIds = new int[numcoord];
        if (NodeSort > 0)
        {
            rdrecr_(rdum, &c__0, 10);
            rdreci_(NodeIds, &c__0, &numcoord, format);
            rdreci_(SolidElemNumbers, &c__0, &NumSolidElements, format);
            rdreci_(BeamElemNumbers, &c__0, &NumBeamElements, format);
            rdreci_(ShellElemNumbers, &c__0, &NumShellElements, format);
            rdreci_(TShellElemNumbers, &c__0, &NumTShellElements, format);
        }
        else
        {
            Materials = new int[NumMaterials];
            rdrecr_(rdum, &c__0, 16);
            rdreci_(NodeIds, &c__0, &numcoord, format);
            rdreci_(SolidElemNumbers, &c__0, &NumSolidElements, format);
            rdreci_(BeamElemNumbers, &c__0, &NumBeamElements, format);
            rdreci_(ShellElemNumbers, &c__0, &NumShellElements, format);
            rdreci_(TShellElemNumbers, &c__0, &NumTShellElements, format);
            rdreci_(Materials, &c__0, &NumMaterials, format);
        }

        /*ie = 0;*/
        /* renumber solid element node ids */
        /*	if (NumSolidElements > 0) {
             i__1 = NumSolidElements;
             for (i = 1; i <= i__1; ++i) {
            ++ie;
            for (j = 1; j <= 8; ++j) {
                work_1.lnodes[j + (ie << 3) - 9] = NodeIds[
                   work_1.lnodes[j + (ie << 3) - 9] - 1];
            }
            if (NodeSort < 0) {
                work_1.iemat[ie - 1] = Materials[work_1.iemat[ie - 1] -
                   1];
      }
      }
      } */
        /* renumber thick shell element node ids */
        /*	if (NumTShellElements > 0) {
             i__1 = NumTShellElements;
             for (i = 1; i <= i__1; ++i) {
            ++ie;
            for (j = 1; j <= 8; ++j) {
                work_1.lnodes[j + (ie << 3) - 9] = NodeIds[
                   work_1.lnodes[j + (ie << 3) - 9] - 1];
            }
            if (NodeSort < 0) {
                work_1.iemat[ie - 1] = Materials[work_1.iemat[ie - 1] -
                   1];
      }
      }
      } */
        /* renumber beam element node ids */
        /*	if (NumBeamElements > 0) {
             i__1 = NumBeamElements;
             for (i = 1; i <= i__1; ++i) {
            ++ie;
            for (j = 1; j <= 3; ++j) {
                work_1.lnodes[j + (ie << 3) - 9] = NodeIds[
                   work_1.lnodes[j + (ie << 3) - 9] - 1];
            }
            if (NodeSort < 0) {
                work_1.iemat[ie - 1] = Materials[work_1.iemat[ie - 1] -
                   1];
      }
      }
      } */
        /* renumber shell element node ids */
        /*	if (NumShellElements > 0) {
             i__1 = NumShellElements;
             for (i = 1; i <= i__1; ++i) {
            ++ie;
            for (j = 1; j <= 4; ++j) {
                work_1.lnodes[j + (ie << 3) - 9] = NodeIds[
                   work_1.lnodes[j + (ie << 3) - 9] - 1];
            }
            if (NodeSort < 0) {
                work_1.iemat[ie - 1] = Materials[work_1.iemat[ie - 1] -
                   1];
      }
      }
      } */
    }

    return 0;
} /* rdtaunum_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::readTimestep()
{

    /*------------------------------------------------------------------------------*/
    /*     read state */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    /* g77  INTEGER ITAU(128) */
    /* iris INTEGER ITAU(512) */

    /* g77 4 TAU(128) */
    /* iris4 TAU(512) */

    if (CEndin == 'N')
    {
        int n;
        int iad;
        float val;

#if 0
        /* shift state pointer of opened data base file */
        if (istate == 1)
        {
            tauio_1.nrzin += NumberOfWords;
        }
        else
        {
            tauio_1.nrzin += NumWords;
        }
#endif

        /* test if the last record of the state exists. If file is too short,  */
        /* try to open next file. Max file id is 99. When file id 99 i reached */
        /* CENDIN --> Y */
        if (rdrecr_(&val, &NumWords, 1) < 0)
            return -1;

        //      std::cerr << "Dyna3DReader::rdstate_(..) CEndin: " << CEndin << std::endl;

        if (CEndin == 'N')
        {
            /*         time address = 1 */
            float TimestepTime = -1.;
            if (rdrecr_(&TimestepTime, &c__1, 1) < 0)
                return -1;
            std::cerr << "timestep time" << TimestepTime << std::endl;
        }
    }

    return 0;
}

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::rdstate_(vistle::Reader::Token &token, int istate, int block)
{
    int ts = istate - 1;
    bool needToRead = false;
    /* Local variables */
    std::shared_lock<std::shared_timed_mutex> shlock(m_timestepMutex);
    if (ts != m_currentTimestep) {
        needToRead = true;
        shlock.unlock();
    }

    if (needToRead) {
        std::unique_lock<std::shared_timed_mutex> ulock(m_timestepMutex, 10*std::chrono::milliseconds());
        while (!ulock.owns_lock()) {
            shlock.lock();
            needToRead = ts != m_currentTimestep;
            shlock.unlock();
            if (!needToRead)
                break;
            ulock.try_lock_for(10*std::chrono::milliseconds());
        }

        if (ulock.owns_lock()) {
            assert(m_currentTimestep < ts);

            deleteStateData();

            do {
                ++m_currentTimestep;

                /*------------------------------------------------------------------------------*/
                /*     read state */
                /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

                /* g77  INTEGER ITAU(128) */
                /* iris INTEGER ITAU(512) */

                /* g77 4 TAU(128) */
                /* iris4 TAU(512) */

                if (CEndin == 'N')
                {
                    float val;

                    /* shift state pointer of opened data base file */
                    if (istate == 1)
                    {
                        tauio_1.nrzin += NumberOfWords;
                    }
                    else
                    {
                        tauio_1.nrzin += NumWords;
                    }

                    /* test if the last record of the state exists. If file is too short,  */
                    /* try to open next file. Max file id is 99. When file id 99 i reached */
                    /* CENDIN --> Y */
                    rdrecr_(&val, &NumWords, 1);
                }

                if (CEndin != 'N') {
                    return -1;
                }
            } while (m_currentTimestep < ts);

            //      std::cerr << "Dyna3DReader::rdstate_(..) CEndin: " << CEndin << std::endl;

            if (CEndin == 'N')
            {
                int n;
                int iad;
                /*         time address = 1 */
                rdrecr_(&TimestepTime, &c__1, 1);
                m_currentTime = TimestepTime;

                /*         NGLBV (global variables for this state) */
                /*         6 Model data : */
                /*             total kinetic energy */
                /*             internal energy */
                /*             total energy */
                /*             vx vy vz */
                /*         6 data blocks for each material */
                /*             ie */
                /*             ke */
                /*             vx vy vz */
                /*             m */
                /*         for all stonewalls */
                /*             Fn */
                /*         if IAFLG = 1: accelerations */
                /*         if IVFLG = 1: velocities */
                /*         matn: vx vy vz */
                /*         mass of each material */
                /*         internal energy by material */
                /*         kinetic energy by material */
                /*         ? */
                /*         vx vy vz rigid body velocities for each material */
                /*         mass of each material */

                iad = NumGlobVar + 2; // ! skipping current analysis time + global variables
                n = numcoord * 3;

                if (IUFlag == 1)
                {
                    DisCo = new float[numcoord * 3];
                    rdrecr_(DisCo, &iad, n);
                }
                else
                {
#ifdef SENDINFO
                    m_module->sendError("Dyna3D Plotfile doesn't contain nodal coordinate data.");
#endif
                    m_currentTimestep = -1;
                    return (-1);
                }

                // nodal data stored as followed (at least I think it is, I found no information about this):
                // ( posX posY posZ ) ( temp1 ( temp2 temp3 ) ( tempChange ) ( flux1 flux2 flux3 )) ( massScaling ) ( velX velY velZ ) ( accelX accelY accelZ )
                //       IUFlag      |                              ITFlag and IDTDT                               |     IVFlag       |         IAFlag
                switch (nodalDataType)
                {
                case 0:
                    break;

                case 2:
                    if (IVFlag == 1)
                    {
                        NodeData = new float[numcoord * 3];
                        INTEGER iaddress = iad + n * IUFlag + numcoord * NumTemperature;
                        rdrecr_(NodeData, &iaddress, n);
                    }
                    else
                    {
#ifdef SENDINFO
                        m_module->sendError("Dyna3D Plotfile doesn't contain nodal velocity data.");
#endif
                        m_currentTimestep = -1;
                        return (-1);
                    }
                    break;

                case 3:
                    if (IAFlag == 1)
                    {
                        NodeData = new float[numcoord * 3];
                        INTEGER iaddress = iad + n * IUFlag + numcoord * NumTemperature + n * IVFlag;
                        rdrecr_(NodeData, &iaddress, n);
                    }
                    else
                    {
#ifdef SENDINFO
                        m_module->sendError("Dyna3D Plotfile doesn't contain nodal acceleration data.");
#endif
                        m_currentTimestep = -1;
                        return (-1);
                    }
                    break;
                }

                if (elementDataType > 0)
                {
                    int numResults = NumSolidVar * NumSolidElements + NumBeamVar * NumBeamElements
                                     + NumShellVar * (NumShellElements - nrelem) + NumTShellVar * NumTShellElements;
                    if (numResults != 0)
                    {
                        int nodeLength = numcoord * NumTemperature + n * IUFlag + n * IVFlag + n * IAFlag;

                        /* solid elements */
                        int solidLength = NumSolidElements * NumSolidVar;
                        SolidData = new float[solidLength];
                        INTEGER iaddress = 2 + NumGlobVar + nodeLength;
                        rdrecr_(SolidData, &iaddress, solidLength);

                        /* thick shell elements */
                        int tshellLength;
                        tshellLength = NumTShellElements * NumTShellVar;
                        TShellData = new float[tshellLength];
                        iaddress = 2 + NumGlobVar + nodeLength + solidLength;
                        rdrecr_(TShellData, &iaddress, tshellLength);

                        /* beam elements */
                        int beamLength = NumBeamElements * NumBeamVar;
                        BeamData = new float[beamLength];
                        iaddress = 2 + NumGlobVar + nodeLength + solidLength + tshellLength;
                        rdrecr_(BeamData, &iaddress, beamLength);

                        /* shell elements */
                        int shellLength = (NumShellElements - nrelem) * NumShellVar;
                        ShellData = new float[shellLength];
                        iaddress = 2 + NumGlobVar + nodeLength + solidLength + tshellLength + beamLength;
                        rdrecr_(ShellData, &iaddress, shellLength);
                    }
                    else
                    {
#ifdef SENDINFO
                        m_module->sendError("Dyna3D Plotfile doesn't contain element results data.");
#endif
                        m_currentTimestep = -1;
                        return (-1);
                    }
                }

                // lists of deleted nodes/elements ?
                if (NumShellInterpol < 0)
                {
                    // read element deletion table
                    if (MDLOpt == 1) // old VEC_DYN
                    {
                        // not yet supported
                    }
                    if (MDLOpt == 2) // LS_930
                    {
                        int NumElements = NumSolidElements + NumTShellElements + NumShellElements
                                          + NumBeamElements;
                        DelTab = new int[NumElements];
                        // total length of node data
                        int nodeLength = numcoord * NumTemperature + n * IUFlag + n * IVFlag + n * IAFlag;
                        // total number of element results
                        int numResults = NumSolidVar * NumSolidElements + NumBeamVar * NumBeamElements
                                         + NumShellVar * (NumShellElements - nrelem) + NumTShellVar * NumTShellElements;
                        INTEGER iaddress = 2 + NumGlobVar + nodeLength + numResults;
                        float *tmpTab = new float[NumElements];
                        rdrecr_(tmpTab, &iaddress, NumElements);
                        for (int i = 0; i < NumElements; ++i)
                        {
                            DelTab[i] = (int)(tmpTab[i]);
                        }
                        delete[] tmpTab;
                        // Notice: The Flags of the deletion table are written in the order:
                        //                Solids, Thick shells, Thin shells, Beams
                    }

                    visibility();
                }
            }
            assert(ts == m_currentTimestep);
            ulock.unlock();
        }
        shlock.lock();

        if (CEndin == 'N')
        {
            ExistingStates++;
            fprintf(stderr, "* read state: %2d   time: %12.6f\n", istate, TimestepTime);
        }
    }

    {
        {
#if 0
            if ((istate >= MinState) && (istate <= State))
#endif
            {
                // This line has been commented out and modified because in case that
                // the files contain more than 1 time step, it may produce wrong
                // informations.
                // sprintf(buf, "Timestep =  %2d \t with time = %6.3f \n", istate, TimestepTime);
#ifdef SENDINFO
                m_module->sendInfo("Timestep with time = %6.3f \n", TimestepTime);
#endif

#if 0
                if (NumShellInterpol < 0)
                {
                    // consider visibility (using the actual deletion table)
                    visibility();
                }
#endif
                createStateObjects(token, istate-1, block);

#if 0
                grid_sets_out[(istate) - MinState] = grid_set_out;
                Vertex_sets_out[(istate) - MinState] = Vertex_set_out;
                Scalar_sets_out[(istate) - MinState] = Scalar_set_out;
#endif
            }
        } // end of if (CEndin == 'N')

    } // end of if (CEndin == 'N') ???

    return 0;
} /* rdstate_ */

/*==============================================================================*/
/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::rdrecr_(float *val, const int *istart, int n)
{
    /* Local variables */
    int i, irdst, iz;

    /*------------------------------------------------------------------------------*/
    /*     read taurus record for floats */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    /* g77  INTEGER ITAU(128) */
    /* iris INTEGER ITAU(512) */

    /* g77 4                TAU(128) */
    /* iris4                TAU(512) */

    /* Parameter adjustments */
    --val;

    /* Function Body */
    if (n > 0)
    {
        /* open TAURUS database for reading (control data and geometry) */
        if (COpenin == 'N' && CEndin == 'N')
        {
            if (otaurusr_() < 0)
                return -1;
        }
        /* read n values starting at current position */
        for (i = 1; i <= n; ++i)
        {
            /*         get record address */
            grecaddr_(i, *istart, &iz, &irdst);
            if (irdst == 0)
            {
                /*           required word found in this file */
                val[i] = tauio_1.tau[iz - 1];
            }
            else
            {
                /*           required word not found in this file. */
                /*           search and open next file. */
                if (otaurusr_() < 0)
                    return -1;
                if (CEndin == 'N')
                {
                    if (*istart > 0)
                    {
                        tauio_1.nrin = tauio_1.nrzin + *istart - 1;
                    }
                    /*             get record address */
                    grecaddr_(i, *istart, &iz, &irdst);
                    if (irdst == 0) // !@@@
                    {
                        /*               required word found in this file */
                        val[i] = tauio_1.tau[iz - 1];
                    }
                    else
                    {
                        /*               error ! */
#ifdef SENDINFO
                        m_module->sendError("*E* Opened file is too short! (RDRECI)");
#endif
                    }
                }
                else
                {
                    val[i] = (float)0.;
                    return 0;
                }
            }
        }
    }

    return 0;
} /* rdrecr_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::rdreci_(int *ival, const int *istart, const int *n, FORMAT format)
{
    /* System generated locals */
    int i__1;

    /* Local variables */

    /*------------------------------------------------------------------------------*/
    /*     read taurus record for ints */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    /* g77  INTEGER ITAU(128) */
    /* iris INTEGER ITAU(512) */

    /* g77 4                TAU(128) */
    /* iris4                TAU(512) */

    /* Parameter adjustments */
    --ival;

    /* Function Body */
    if (*n > 0)
    {
        /* open TAURUS database for reading (control data and geometry) */
        if (COpenin == 'N' && CEndin == 'N')
        {
            if (otaurusr_() < 0)
                return -1;
        }
        /*       read n values starting at current position */
        i__1 = *n;
        for (INTEGER i = 1; i <= i__1; ++i)
        {
            /*         get record address */
            INTEGER irdst, iz;
            grecaddr_(i, *istart, &iz, &irdst);
            if (irdst == 0)
            {
                /*           required word found in this file */
                switch(format) {
                case CADFEM:
                    ival[i] = (INTEGER)(tauio_1.tau[iz - 1]);
                    break;
                case ORIGINAL:
                    ival[i] = ((INTEGER *)(void *)(tauio_1.tau))[iz - 1];
                    break;
                default:
                    std::cerr << "Dyna3DReader::rdreci_ info: Unknown format '" << format << "' selected" << std::endl;
                    break;
                }
            }
            else
            {
                /*           required word not found in this file. */
                /*           search and open next file. */
                if (otaurusr_() < 0)
                    return -1;
                if (CEndin == 'N')
                {
                    if (*istart > 0)
                    {
                        tauio_1.nrin = tauio_1.nrzin + *istart - 1;
                    }
                    /*             get record address */
                    grecaddr_(i, *istart, &iz, &irdst);
                    if (irdst == 0)
                    {
                        /*               required word found in this file */
                        switch(format) {
                        case CADFEM:
                            ival[i] = static_cast<INTEGER>(tauio_1.tau[iz - 1]);
                            break;
                        case ORIGINAL:
                            ival[i] = *reinterpret_cast<INTEGER *>(&tauio_1.tau[iz-1]);
                            break;
                        default:
                            std::cerr << "Dyna3DReader::rdreci_ info: Unknown format '" << format << "' selected" << std::endl;
                            break;
                        }
                    }
                    else
                    {
                        /*               error ! */
                        if (CTrace == 'Y')
                        {
#ifdef SENDINFO
                            m_module->sendError("*E* Opened file is too short! (RDRECI)");
#endif
                        }
                    }
                }
                else
                {
                    ival[i] = 0;
                    return 0;
                }
            }
        }
    }

    return 0;
} /* rdreci_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::grecaddr_(INTEGER i, INTEGER istart, INTEGER *iz, INTEGER *irdst)
{
    /* Local variables */
    int itrecn;

    /*------------------------------------------------------------------------------*/
    /*     get record address for reading */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    /* g77  INTEGER ITAU(128) */
    /* iris INTEGER ITAU(512) */

    /* g77 4                TAU(128) */
    /* iris4                TAU(512) */

    *irdst = 0;
    if (istart > 0)
    {
        tauio_1.nrin = tauio_1.nrzin + istart + i - 1;
    }
    else
    {
        ++tauio_1.nrin;
    }
    if (i == 1 && CTrace == 'Y')
    {
        fprintf(stderr, "* NRIN = %d\n", tauio_1.nrin);
    }
    if (i > 0)
    {
        itrecn = (tauio_1.nrin - 1) / tauio_1.irl + 1;
        *iz = tauio_1.nrin - (itrecn - 1) * tauio_1.irl;
        if (tauio_1.itrecin != itrecn)
        {
            tauio_1.itrecin = itrecn;
            if (lseek(infile, (itrecn - 1) * sizeof(tauio_1.tau), SEEK_SET) >= 0)
            {
                *irdst = ::read(infile, tauio_1.tau, sizeof(tauio_1.tau))
                         - sizeof(tauio_1.tau);

                tauio_1.taulength = *irdst + sizeof(tauio_1.tau) / sizeof(WORD);
                if (byteswapFlag == BYTESWAP_ON)
                {
                    WORD *buffer = (WORD *)(void *)(tauio_1.tau);
                    byteswap(buffer, tauio_1.taulength);
                }
            }
            else
            {
                *irdst = -1;
            }
        }
    }

    return 0;
} /* grecaddr_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::placpnt_(int *istart)
{
    /* Local variables */

    /*------------------------------------------------------------------------------*/
    /*     place pointer to address ISTART */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    /* g77  INTEGER ITAU(128) */
    /* iris INTEGER ITAU(512) */

    /* g77 4                TAU(128) */
    /* iris4                TAU(512) */

    if (*istart > 0)
    {
        tauio_1.nrin = tauio_1.nrzin + *istart - 1;
    }

    return 0;
} /* placpnt_ */

/* Subroutine */
template<int wordsize, class INTEGER, class REAL>
int Dyna3DReader<wordsize,INTEGER,REAL>::otaurusr_()
{
    std::cerr << "otaurusr_" << std::endl;
    /* Local variables */
    char ctaun[2000];

    /*------------------------------------------------------------------------------*/
    /*     open a TAURUS database for reading */
    /*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----*/

    /* g77  INTEGER ITAU(128) */
    /* iris INTEGER ITAU(512) */

    /* g77 4                TAU(128) */
    /* iris4                TAU(512) */

    if (COpenin == 'N')
    {
        tauio_1.ifilin = 0;
        tauio_1.nrin = 0;
        tauio_1.nrzin = 0;
        tauio_1.itrecin = 0;
        tauio_1.adaptation = 0;
        tauio_1.adapt[0] = '\0';
        tauio_1.taulength = 0;
        COpenin = 'Y';
        fprintf(stderr, " TAURUS input file : %s\n", m_filename.c_str());
        strcpy(CTauin, m_filename.c_str());
        infile = ::open(CTauin, OpenFlags);
        if (infile < 0) {
            fprintf(stderr, "could not open %s\n", CTauin);
            return -1;
        }
    }
    else
    {
        if (infile >= 0) {
            close(infile);
            infile = -1;
        }

        int numtries = 0;
        while ((tauio_1.ifilin < MAXTIMESTEPS) && (numtries < 10))
        {
            numtries++;
            ++tauio_1.ifilin;
            tauio_1.nrin = 0;
            tauio_1.nrzin = 0;
            tauio_1.itrecin = 0;
            sprintf(ctaun, "%s%s%02d", CTauin, tauio_1.adapt, tauio_1.ifilin);
// infile=Covise::open(ctaun,O_RDONLY);
#ifndef _AIRBUS
            infile = ::open(ctaun, OpenFlags);
            if (infile >= 0)
            {
                fprintf(stderr, "opened %s\n", ctaun);
                return (0);
            }
#else
            if ((version == 0.0 && tauio_1.ifilin % 2 == 1) || version != 0.0)
            {
                infile = ::open(ctaun, OpenFlags);
                if (infile >= 0)
                {
                    fprintf(stderr, "opened %s\n", ctaun);
                    return (0);
                }
            }
#endif
        }
        int num1 = (tauio_1.adaptation) / 26;
        int num2 = (tauio_1.adaptation) - 26 * num1;
        tauio_1.adapt[0] = 'a' + num1;
        tauio_1.adapt[1] = 'a' + num2;
        tauio_1.adapt[2] = '\0';
        tauio_1.adaptation++;
        tauio_1.ifilin = 0;
        tauio_1.nrin = 0;
        tauio_1.nrzin = 0;
        tauio_1.itrecin = 0;
        sprintf(ctaun, "%s%s", CTauin, tauio_1.adapt);
// infile=Covise::open(ctaun,O_RDONLY);
        infile = ::open(ctaun, OpenFlags);
        if (infile >= 0)
        {
            fprintf(stderr, "opened %s\n", ctaun);

            deleteElementLUT();
            /* read TAURUS control data */
            rdtaucntrl_(format);

            /* read TAURUS node coordinates */
            rdtaucoor_();
            /* read TAURUS elements */
            rdtauelem_(format);

            /* read user node and element numbers */
            rdtaunum_(format);

            // initialize

            createElementLUT();
            createGeometryList();
            /* shift state pointer of opened data base file */
            tauio_1.nrzin += NumberOfWords;

            return (0);
        }
        else
        {
            fprintf(stderr, "could not open %s\n", ctaun);
        }
        CEndin = 'Y';

        return -1;
    }

    return 0;
} /* otaurusr_ */

//---------------------------------------------------------------------

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::visibility()
{
    /*                                                        */
    /* decrease the grids in case of invisible nodes/elements */
    /*                                                        */

    int i;
    int elemNo;

    // initialize
    for (i = 0; i < NumIDs; i++)
    {
        delElem[i] = 0;
        delCon[i] = 0;
    }

#ifndef _AIRBUS
    if (MDLOpt == 2) // LS_930
#else
    if (version == 0.0 || MDLOpt == 2)
#endif
    {

        // solids
        for (i = 0; i < NumSolidElements; i++)
        {
            if (DelTab[i] == 0)
            {
                delElem[SolidMatIds[i]]++;
                solidTab[i].visible = false;

                switch (solidTab[i].coType)
                {
                case UnstructuredGrid::TETRAHEDRON:
                    delCon[SolidMatIds[i]] += 4;
                    break;

                case UnstructuredGrid::PRISM:
                    delCon[SolidMatIds[i]] += 6;
                    break;

                case UnstructuredGrid::HEXAHEDRON:
                    delCon[SolidMatIds[i]] += 8;
                    break;
                }
            }
        }
        // !!! thick shells omitted

        // thin shells
        for (i = 0; i < NumShellElements; i++)
        {
            elemNo = NumSolidElements + i;
            if (DelTab[elemNo] == 0)
            {
                delElem[ShellMatIds[i]]++;
                shellTab[i].visible = false;

                switch (shellTab[i].coType)
                {

                case UnstructuredGrid::TRIANGLE:
                    delCon[ShellMatIds[i]] += 3;
                    break;

                case UnstructuredGrid::QUAD:
                    delCon[ShellMatIds[i]] += 4;
                    break;
                }
            }
        }
        // beams
        for (i = 0; i < NumBeamElements; i++)
        {
            elemNo = NumSolidElements + NumShellElements + i;
            if (DelTab[elemNo] == 0)
            {
                delElem[BeamMatIds[i]]++;
                delCon[BeamMatIds[i]] += 2;
                beamTab[i].visible = false;
            }
        }
    }
    if (MDLOpt == 1) // VEC_DYN
    {
        // not yet supported
    }
}

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::createElementLUT()
{
    int i;

    // allocate data objects for element lookup tables
    //solids
    solidTab.resize(NumSolidElements);
    // thick shells
    tshellTab.resize(NumTShellElements);
    // shells
    shellTab.resize(NumShellElements);
    // beams
    beamTab.resize(NumBeamElements);

    // init material ID counter
    NumIDs = 0;
    // maximum material ID number
    for (i = 0; i < NumSolidElements; i++)
    {
        SolidMatIds[i]--; // !? decrement ?!
        if (SolidMatIds[i] > NumIDs)
            NumIDs = SolidMatIds[i];
    }
    for (i = 0; i < NumTShellElements; i++)
    {
        TShellMatIds[i]--;
        if (TShellMatIds[i] > NumIDs)
            NumIDs = TShellMatIds[i];
    }
    for (i = 0; i < NumShellElements; i++)
    {
        ShellMatIds[i]--;
        if (ShellMatIds[i] > NumIDs)
            NumIDs = ShellMatIds[i];
    }
    for (i = 0; i < NumBeamElements; i++)
    {
        BeamMatIds[i]--;
        if (BeamMatIds[i] > NumIDs)
            NumIDs = BeamMatIds[i];
    }
    NumIDs++;

    // allocate
    numcon = new int[NumIDs];
    numelem = new int[NumIDs];
    numcoo = new int[NumIDs];

    coordLookup = new int *[NumIDs];

    My_elemList = new int *[NumIDs];
    conList = new int *[NumIDs];

    delElem = new int[NumIDs];
    delCon = new int[NumIDs];

    // initialize
    for (i = 0; i < NumIDs; i++)
    {
        numcon[i] = 0;
        numelem[i] = 0;
        numcoo[i] = 0;

        coordLookup[i] = NULL;

        My_elemList[i] = NULL;
        conList[i] = NULL;

        delElem[i] = 0;
        delCon[i] = 0;
    }

    // SETTING TYPE AND NODE NUMBERS
    // solids
    for (i = 0; i < NumSolidElements; i++)
    {

        if ((SolidNodes[i * 8 + 4] == SolidNodes[i * 8 + 5]) && (SolidNodes[i * 8 + 6] == SolidNodes[i * 8 + 7]))
        {
            if (SolidNodes[i * 8 + 5] == SolidNodes[i * 8 + 6])
            {
                numelem[SolidMatIds[i]]++;
                numcon[SolidMatIds[i]] += 4;
                solidTab[i].coType = UnstructuredGrid::TETRAHEDRON;
            }
            else
            {
                numelem[SolidMatIds[i]]++;
                numcon[SolidMatIds[i]] += 6;
                solidTab[i].coType = UnstructuredGrid::PRISM;
            }
        }
        else
        {
            numelem[SolidMatIds[i]]++;
            numcon[SolidMatIds[i]] += 8;
            solidTab[i].coType = UnstructuredGrid::HEXAHEDRON;
        }

        /*
      switch ( solidTab[i]->coType ){

        case UnstructuredGrid::TETRAHEDRON:
      for (nn=0; nn<4; nn++)
       solidTab[i]->set_node(nn, SolidNodes[i*8+nn]);
        break;

        case UnstructuredGrid::PRISM:
      for (nn=0; nn<6; nn++)
       solidTab[i]->set_node(nn, SolidNodes[i*8+nn]);
      break;

      case UnstructuredGrid::HEXAHEDRON:
      for (nn=0; nn<8; nn++)
      solidTab[i]->set_node(nn, SolidNodes[i*8+nn]);
      break;

      }
      */
    }
    // !!! thick shells omitted
    // thin shells
    for (i = 0; i < NumShellElements; i++)
    {

        if (ShellNodes[i * 4 + 2] == ShellNodes[i * 4 + 3])
        {
            numelem[ShellMatIds[i]]++;
            numcon[ShellMatIds[i]] += 3;
            shellTab[i].coType = UnstructuredGrid::TRIANGLE;
        }
        else
        {
            numelem[ShellMatIds[i]]++;
            numcon[ShellMatIds[i]] += 4;
            shellTab[i].coType = UnstructuredGrid::QUAD;
        }

        /*
      switch ( shellTab[i]->coType ){

        case UnstructuredGrid::TRIANGLE:
      for (nn=0; nn<3; nn++)
       shellTab[i]->set_node(nn, ShellNodes[i*4+nn]);
        break;

        case UnstructuredGrid::QUAD:
      for (nn=0; nn<4; nn++)
       shellTab[i]->set_node(nn, ShellNodes[i*4+nn]);
      break;

      }
      */
    }
    // beams
    for (i = 0; i < NumBeamElements; i++)
    {
        numelem[BeamMatIds[i]]++;
        numcon[BeamMatIds[i]] += 2;
        beamTab[i].coType = UnstructuredGrid::BAR;
        /*
      for (nn=0; nn<2; nn++)
        beamTab[i]->set_node(nn, BeamNodes[i*5+nn]);
      */
    }

    // SETTING MATERIAL NUMBER
    materials.clear();
    materials.resize(NumIDs);
    for (auto i=0; i<NumSolidElements; ++i) {
        auto id = SolidMatIds[i];
        materials[id].solid.push_back(i);
    }
    for (auto i=0; i<NumTShellElements; ++i) {
        auto id = TShellMatIds[i];
        materials[id].tshell.push_back(i);
    }
    for (auto i=0; i<NumShellElements; ++i) {
        auto id = ShellMatIds[i];
        materials[id].shell.push_back(i);
    }
    for (auto i=0; i<NumBeamElements; ++i) {
        auto id = BeamMatIds[i];
        materials[id].beam.push_back(i);
    }

#if 0
    for (ID = 0; ID < NumIDs; ID++)
    {
        std::sort(materials[ID].solid.begin(), materials[ID].solid.end());
        std::sort(materials[ID].tshell.begin(), materials[ID].tshell.end());
        std::sort(materials[ID].shell.begin(), materials[ID].shell.end());
        std::sort(materials[ID].beam.begin(), materials[ID].beam.end());
    }
#endif
}

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::deleteElementLUT()
{
    materials.clear();

    solidTab.clear();
    tshellTab.clear();
    shellTab.clear();
    beamTab.clear();

    delete[] numcon;
    numcon = nullptr;

    delete[] numelem;
    numelem = nullptr;

    delete[] numcoo;
    numcoo = nullptr;

    if (My_elemList)
    {
        for (INTEGER i = 0; i < NumIDs; i++)
        {
            delete[] My_elemList[i];
            delete[] conList[i];
            delete[] coordLookup[i];
        }
    }
    delete[] My_elemList;
    My_elemList = nullptr;
    delete[] coordLookup;
    coordLookup = nullptr;
    delete[] conList;
    conList = nullptr;

    delete[] delElem;
    delElem = nullptr;

    delete[] delCon;
    delCon = nullptr;
}

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::createGeometryList()
{
    int i, j;
    int ID;

    int IDcount = 0; // part ID counter
    int elemNo; // COVISE element list entry
    int coElem; // COVISE element number counter
    int coCon; // COVISE connectivity counter

    for (ID = 0; ID < NumIDs; ID++)
    {
        if (IDLISTE[ID] && numelem[ID] > 0)

        {
            auto lookupTab = new int[numcoord+1];
            createNodeLUT(ID, lookupTab);

            // allocate element list
            My_elemList[ID] = new int[numelem[ID]];
            // allocate connection list
            conList[ID] = new int[numcon[ID]];

            // initialize element numbering
            elemNo = 0;
            coElem = 0;
            coCon = 0;

#ifndef _AIRBUS
            if (MDLOpt == 2) // LS_930
#else
            if (version == 0.0 || MDLOpt == 2)
#endif
            {

                // solids
                for (auto i: materials[ID].solid)
                {
                    assert(SolidMatIds[i] == ID);
                    {
                        switch (solidTab[i].coType)
                        {

                        case UnstructuredGrid::TETRAHEDRON:
                            My_elemList[ID][coElem] = elemNo;
                            for (j = 0; j < 4; j++)
                            {
                                conList[ID][coCon] = lookupTab[SolidNodes[i * 8 + j]];
                                coCon++;
                            }
                            elemNo += 4;
                            solidTab[i].coElem = coElem;
                            coElem++;
                            break;

                        case UnstructuredGrid::PRISM:
                            My_elemList[ID][coElem] = elemNo;
                            conList[ID][coCon] = lookupTab[SolidNodes[i * 8 + 4]];
                            coCon++;
                            conList[ID][coCon] = lookupTab[SolidNodes[i * 8 + 1]];
                            coCon++;
                            conList[ID][coCon] = lookupTab[SolidNodes[i * 8]];
                            coCon++;
                            conList[ID][coCon] = lookupTab[SolidNodes[i * 8 + 6]];
                            coCon++;
                            conList[ID][coCon] = lookupTab[SolidNodes[i * 8 + 2]];
                            coCon++;
                            conList[ID][coCon] = lookupTab[SolidNodes[i * 8 + 3]];
                            coCon++;
                            elemNo += 6;
                            solidTab[i].coElem = coElem;
                            coElem++;
                            break;

                        case UnstructuredGrid::HEXAHEDRON:
                            My_elemList[ID][coElem] = elemNo;
                            for (j = 0; j < 8; j++)
                            {
                                conList[ID][coCon] = lookupTab[SolidNodes[i * 8 + j]];
                                coCon++;
                            }
                            elemNo += 8;
                            solidTab[i].coElem = coElem;
                            coElem++;
                            break;
                        }
                    }
                }

                // Shells
                for (auto i: materials[ID].shell)
                {
                    assert(ShellMatIds[i] == ID);
                    {
                        switch (shellTab[i].coType)
                        {

                        case UnstructuredGrid::TRIANGLE:
                            My_elemList[ID][coElem] = elemNo;
                            for (j = 0; j < 3; j++)
                            {
                                conList[ID][coCon] = lookupTab[ShellNodes[i * 4 + j]];
                                coCon++;
                            }
                            elemNo += 3;
                            shellTab[i].coElem = coElem;
                            coElem++;
                            break;

                        case UnstructuredGrid::QUAD:
                            My_elemList[ID][coElem] = elemNo;
                            for (j = 0; j < 4; j++)
                            {
                                conList[ID][coCon] = lookupTab[ShellNodes[i * 4 + j]];
                                coCon++;
                            }
                            elemNo += 4;
                            shellTab[i].coElem = coElem;
                            coElem++;
                            break;
                        }
                    }
                }

                // Beams
                for (auto i: materials[ID].beam)
                {
                    assert(BeamMatIds[i] == ID);
                    {
                        My_elemList[ID][coElem] = elemNo;
                        for (j = 0; j < 2; j++)
                        {
                            conList[ID][coCon] = lookupTab[BeamNodes[i * 5 + j]];
                            coCon++;
                        }
                        elemNo += 2;
                        beamTab[i].coElem = coElem;
                        coElem++;
                    }
                }

                // nodal data
                // initialize coordinate numbering
                coordLookup[ID] = new int[numcoo[ID]];
                for (i = 1; i <= numcoord; i++)
                {
                    if (lookupTab[i] >= 0)
                    {
                        coordLookup[ID][lookupTab[i]] = i;
                    }
                }

            } // end of if (MDLOpt ...)
            else
            {
#ifdef SENDINFO
                m_module->sendError("LS-DYNA Plotfile has wrong version. Only support for version LS-930 and higher.");
#endif
                //exit(-1);
            }
            IDcount++;

            //free
            delete[] lookupTab;

        } // end of if ( IDLISTE[ID] && numelem[ID] > 0)
    } // end of for (ID=0 ...)
}

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::createGeometry(Reader::Token &token, int blockToRead) const
{
    char name[256];
    char part_buf[256];

    int i, j;

    int IDcount = 0; // part ID counter
    int conNo; // COVISE connection list entry
    int elemNo; // COVISE element list entry

#if 0
    // allocate
    grids_out = new coDoUnstructuredGrid *[NumIDs + 1];
    grids_out[NumIDs] = NULL; // mark end of set array
#endif

    for (INTEGER ID = 0; ID < NumIDs; ID++)
    {
        if (blockToRead >= 0 && blockToRead != ID)
            continue;
#if 0
        // initialize
        grids_out[ID] = NULL;
#endif

        if (IDLISTE[ID] && numelem[ID] > 0)
        {
#if 0
            int *el, *cl, *tl;
            float *x_c, *y_c, *z_c;

            // mesh
            sprintf(name, "%s_%d_ID%d", Grid, 0, ID);
            grid_out = new coDoUnstructuredGrid(name,
                                                numelem[ID],
                                                numcon[ID],
                                                numcoo[ID],
                                                1);
            grid_out->getAddresses(&el, &cl, &x_c, &y_c, &z_c);
            grid_out->getTypeList(&tl);
            // COLOR attribute
            grid_out->addAttribute("COLOR", colornames[ID % 10]);
            // PART attribute
            sprintf(part_buf, "%d", ID + 1);
            grid_out->addAttribute("PART", part_buf);
#else
            UnstructuredGrid::ptr grid_out(new UnstructuredGrid(numelem[ID], numcon[ID], numcoo[ID]));
            grid_out->addAttribute("_part", std::to_string(ID+1));
            auto el = &grid_out->el()[0];
            auto cl = &grid_out->cl()[0];
            auto tl = &grid_out->tl()[0];
            auto x_c = &grid_out->x()[0];
            auto y_c = &grid_out->y()[0];
            auto z_c = &grid_out->z()[0];
#endif

            // initialize element numbering
            elemNo = 0;

#ifndef _AIRBUS
            if (MDLOpt == 2) // LS_930
#else
            if (version == 0.0 || MDLOpt == 2)
#endif
            {

                // solids
                for (auto i: materials[ID].solid)
                {
                    assert(SolidMatIds[i] == ID);

                        switch (solidTab[i].coType)
                        {

                        case UnstructuredGrid::TETRAHEDRON:
                            *tl++ = UnstructuredGrid::TETRAHEDRON;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][solidTab[i].coElem];
                            for (j = 0; j < 4; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 4;
                            break;

                        case UnstructuredGrid::PRISM:
                            *tl++ = UnstructuredGrid::PRISM;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][solidTab[i].coElem];
                            for (j = 0; j < 6; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 6;
                            break;

                        case UnstructuredGrid::HEXAHEDRON:
                            *tl++ = UnstructuredGrid::HEXAHEDRON;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][solidTab[i].coElem];
                            for (j = 0; j < 8; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 8;
                            break;
                        }
                }
                // Shells
                for (auto i: materials[ID].shell)
                {
                    assert(ShellMatIds[i] == ID);
                        switch (shellTab[i].coType)
                        {

                        case UnstructuredGrid::TRIANGLE:
                            *tl++ = UnstructuredGrid::TRIANGLE;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][shellTab[i].coElem];
                            for (j = 0; j < 3; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 3;
                            break;

                        case UnstructuredGrid::QUAD:
                            *tl++ = UnstructuredGrid::QUAD;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][shellTab[i].coElem];
                            for (j = 0; j < 4; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 4;
                            break;
                        }
                }

                // Beams
                for (auto i: materials[ID].beam)
                {
                    assert(BeamMatIds[i] == ID);
                        *tl++ = UnstructuredGrid::BAR;
                        *el++ = elemNo;
                        conNo = My_elemList[ID][beamTab[i].coElem];
                        for (j = 0; j < 2; j++)
                        {
                            *cl++ = conList[ID][conNo + j];
                        }
                        elemNo += 2;
                }

                // nodal data
                for (i = 0; i < numcoo[ID]; i++)
                {
                    x_c[i] = Coord[(coordLookup[ID][i] - 1) * 3];
                    y_c[i] = Coord[(coordLookup[ID][i] - 1) * 3 + 1];
                    z_c[i] = Coord[(coordLookup[ID][i] - 1) * 3 + 2];
                }

            } // end of if (MDLOpt ...)
            else
            {
#ifdef SENDINFO
                m_module->sendError("LS-DYNA Plotfile has wrong version. Only support for version LS-930 and higher.");
#endif
                exit(-1);
            }
#if 0
            grids_out[IDcount] = grid_out;
#endif
            *el++ = elemNo;
            grid_out->setBlock(ID);
            std::cerr << "adding " << grid_out->getName() << " to " << gridPort->getName() << " Token: " << token << std::endl;
            token.addObject(gridPort, grid_out);
            IDcount++;

        } // end of if ( IDLISTE[ID] && numelem[ID] > 0)
    } // end of for (ID=0 ...)

    // mark end of set array
#if 0
    grids_out[IDcount] = NULL;

    sprintf(name, "%s_%d", Grid, 0);
    grid_set_out = new coDoSet(name, (coDistributedObject **)grids_out);

    // free
    for (i = 0; i <= IDcount; i++)
    {
        delete grids_out[i];
    }
    delete[] grids_out;
#endif
}

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::createNodeLUT(int id, int *lut)
{
    int nn;

    // init node lookup table
    for (int i = 0; i <= numcoord; i++)
        lut[i] = -1;

    // solids
    for (auto i: materials[id].solid)
    {
        assert(SolidMatIds[i] == id);
        {
            for (nn = 0; nn < 8; nn++)
            {
                if (lut[SolidNodes[i * 8 + nn]] < 0)
                {
                    lut[SolidNodes[i * 8 + nn]] = numcoo[id]++;
                }
            }
        }
    }
    // thick shells
    for (auto i: materials[id].tshell)
    {
        assert(TShellMatIds[i] == id);
        {
            for (nn = 0; nn < 8; nn++)
            {
                if (lut[TShellNodes[i * 8 + nn]] < 0)
                {
                    lut[TShellNodes[i * 8 + nn]] = numcoo[id]++;
                }
            }
        }
    }
    // shells
    for (auto i: materials[id].shell)
    {
        assert(ShellMatIds[i] == id);
        {
            for (nn = 0; nn < 4; nn++)
            {
                if (lut[ShellNodes[i * 4 + nn]] < 0)
                {
                    lut[ShellNodes[i * 4 + nn]] = numcoo[id]++;
                }
            }
        }
    }
    // beams
    for (auto i: materials[id].beam)
    {
        assert(BeamMatIds[i] == id);
        {
            for (nn = 0; nn < 2; nn++)
            {
                if (lut[BeamNodes[i * 5 + nn]] < 0)
                {
                    lut[BeamNodes[i * 5 + nn]] = numcoo[id]++;
                }
            }
        }
    }
}

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::createStateObjects(vistle::Reader::Token &token, int timestep, int blockToRead) const
{
    char name[256];
    char part_buf[256];
    float druck, vmises;

    int i, j;

    int IDcount = 0; // part ID counter
    int conNo; // COVISE connection list entry
    int elemNo; // COVISE element list entry

    // allocate
#if 0
    grids_out = new coDoUnstructuredGrid *[NumIDs + 1];
    grids_out[NumIDs] = NULL; // mark end of set array

    Vertexs_out = new coDoVec3 *[NumIDs + 1];
    Vertexs_out[NumIDs] = NULL;
    Scalars_out = new coDoFloat *[NumIDs + 1];
    Scalars_out[NumIDs] = NULL;
#endif

    for (INTEGER ID = 0; ID < NumIDs; ID++)
    {
#if 0
        // initialize
        grids_out[ID] = NULL;
        Vertexs_out[ID] = NULL;
        Scalars_out[ID] = NULL;
#endif

        if (blockToRead >= 0 && blockToRead != ID)
            continue;

        if (IDLISTE[ID] && numelem[ID] > 0)
        {
#if 0
            int *el, *cl, *tl;
            float *x_c, *y_c, *z_c;
            float *vx_out = NULL, *vy_out = NULL, *vz_out = NULL;
            float *s_el = NULL;

            // mesh
            sprintf(name, "%s_%d_ID%d", Grid, timestep, ID);
            grid_out = new coDoUnstructuredGrid(name,
                                                numelem[ID] - delElem[ID],
                                                numcon[ID] - delCon[ID],
                                                numcoo[ID],
                                                1);
            grid_out->getAddresses(&el, &cl, &x_c, &y_c, &z_c);
            grid_out->getTypeList(&tl);
            // COLOR attribute
            grid_out->addAttribute("COLOR", colornames[ID % 10]);
            // PART attribute
            sprintf(part_buf, "%d", ID + 1);
            grid_out->addAttribute("PART", part_buf);

            // nodal vector data
            if (nodalDataType > 0)
            {

                sprintf(name, "%s_%d_ID%d", Vertex, timestep, ID);
                Vertex_out = new coDoVec3(name, numcoo[ID]);
                Vertex_out->addAttribute("PART", part_buf);
                Vertex_out->getAddresses(&vx_out, &vy_out, &vz_out);
            }
            else
            {
                Vertex_out = NULL;
            }
            // element scalar data
            if (elementDataType > 0)
            {
                sprintf(name, "%s_%d_ID%d", Scalar, timestep, ID);
                // @@@ This only works if rigid shells are not mixed
                //     in a "part" with other element types!!!!!!
                if (NumDim == 5
                    && minShell[ID] >= 0
                    && minShell[ID] < NumShellElements
                    && ShellMatIds[minShell[ID]] == ID)
                {
                    Scalar_out = new coDoFloat(name, matTyp[ID] != 20 ? numelem[ID] - delElem[ID] : 0);
                }
                else
                {
                    Scalar_out = new coDoFloat(name, numelem[ID] - delElem[ID]);
                }
                Scalar_out->addAttribute("PART", part_buf);
                Scalar_out->getAddress(&s_el);
            }
            else
            {
                Scalar_out = NULL;
            }
#else
            UnstructuredGrid::ptr grid_out(new UnstructuredGrid(numelem[ID]-delElem[ID],
                                                                numcon[ID]-delCon[ID],
                                                                numcoo[ID]));
            auto el = &grid_out->el()[0];
            auto cl = &grid_out->cl()[0];
            auto tl = &grid_out->tl()[0];
            auto x_c = &grid_out->x()[0];
            auto y_c = &grid_out->y()[0];
            auto z_c = &grid_out->z()[0];

            Vec<Scalar,3>::ptr Vertex_out;
            Scalar *vx_out=nullptr, *vy_out=nullptr, *vz_out=nullptr;
            if (nodalDataType > 0) {
                Vertex_out.reset(new Vec<Scalar,3>(numcoo[ID]));
                vx_out = &Vertex_out->x()[0];
                vy_out = &Vertex_out->y()[0];
                vz_out = &Vertex_out->z()[0];
            }

            Vec<Scalar>::ptr Scalar_out;
            Scalar *s_el = nullptr;
            if (elementDataType > 0) {
                if (NumDim == 5 && !materials[ID].shell.empty())
                {
                    Scalar_out.reset(new Vec<Scalar>(matTyp[ID] != 20 ? numelem[ID] - delElem[ID] : 0));
                }
                else
                {
                    Scalar_out.reset(new Vec<Scalar>(numelem[ID] - delElem[ID]));
                }
                s_el = &Scalar_out->x()[0];
            }
#endif

            // initialize element numbering
            elemNo = 0;

#ifndef _AIRBUS
            if (MDLOpt == 2) // LS_930
#else
            if (version == 0.0 || MDLOpt == 2)
#endif
            {

                // solids
                for (auto i: materials[ID].solid)
                {
                    assert(SolidMatIds[i] == ID);
                    if (solidTab[i].visible)
                    {

                        switch (solidTab[i].coType)
                        {

                        case UnstructuredGrid::TETRAHEDRON:
                            *tl++ = UnstructuredGrid::TETRAHEDRON;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][solidTab[i].coElem];
                            for (j = 0; j < 4; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 4;
                            break;

                        case UnstructuredGrid::PRISM:
                            *tl++ = UnstructuredGrid::PRISM;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][solidTab[i].coElem];
                            for (j = 0; j < 6; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 6;
                            break;

                        case UnstructuredGrid::HEXAHEDRON:
                            *tl++ = UnstructuredGrid::HEXAHEDRON;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][solidTab[i].coElem];
                            for (j = 0; j < 8; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 8;
                            break;
                        }

                        // element data
                        switch (elementDataType)
                        {

                        case 1: // element stress (tensor component)

                            switch (component)
                            {

                            case 0:
                                *s_el++ = SolidData[i * NumSolidVar];
                                break;

                            case 1:
                                *s_el++ = SolidData[i * NumSolidVar + 1];
                                break;

                            case 2:
                                *s_el++ = SolidData[i * NumSolidVar + 2];
                                break;

                            case 3:
                                *s_el++ = SolidData[i * NumSolidVar + 3];
                                break;

                            case 4:
                                *s_el++ = SolidData[i * NumSolidVar + 4];
                                break;

                            case 5:
                                *s_el++ = SolidData[i * NumSolidVar + 5];
                                break;

                            case 6:
                                druck = SolidData[i * NumSolidVar];
                                druck += SolidData[i * NumSolidVar + 1];
                                druck += SolidData[i * NumSolidVar + 2];
                                druck /= -3.0;
                                *s_el++ = druck;
                                break;
                            case 7:
                                druck = SolidData[i * NumSolidVar];
                                druck += SolidData[i * NumSolidVar + 1];
                                druck += SolidData[i * NumSolidVar + 2];
                                druck /= 3.0;
                                vmises = (SolidData[i * NumSolidVar] - druck) * (SolidData[i * NumSolidVar] - druck);
                                vmises += (SolidData[i * NumSolidVar + 1] - druck) * (SolidData[i * NumSolidVar + 1] - druck);
                                vmises += (SolidData[i * NumSolidVar + 2] - druck) * (SolidData[i * NumSolidVar + 2] - druck);
                                vmises += 2.0f * SolidData[i * NumSolidVar + 3] * SolidData[i * NumSolidVar + 3];
                                vmises += 2.0f * SolidData[i * NumSolidVar + 4] * SolidData[i * NumSolidVar + 4];
                                vmises += 2.0f * SolidData[i * NumSolidVar + 5] * SolidData[i * NumSolidVar + 5];
                                vmises = (float)sqrt(1.5 * vmises);
                                *s_el++ = vmises;
                                break;
                            }

                            break;

                        case 2: // plastic strain
                            *s_el++ = SolidData[i * NumSolidVar + 6 * SomeFlags[0]];
                            break;
                        }
                    }
                }

                // Shells
                int CoviseElement = -1;
                for (auto i: materials[ID].shell)
                {
                    assert(ShellMatIds[i] == ID);
                    if (shellTab[i].visible)
                    {
                        ++CoviseElement;
                        switch (shellTab[i].coType)
                        {

                        case UnstructuredGrid::TRIANGLE:
                            *tl++ = UnstructuredGrid::TRIANGLE;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][shellTab[i].coElem];
                            for (j = 0; j < 3; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 3;
                            break;

                        case UnstructuredGrid::QUAD:
                            *tl++ = UnstructuredGrid::QUAD;
                            *el++ = elemNo;
                            conNo = My_elemList[ID][shellTab[i].coElem];
                            for (j = 0; j < 4; j++)
                            {
                                *cl++ = conList[ID][conNo + j];
                            }
                            elemNo += 4;
                            break;
                        }

                        // element data (NEIPS==0 !!!!!!)
                        int jumpIntegPoints = SomeFlags[0] * 6 + SomeFlags[1];

                        if (NumDim == 5
                            && matTyp[ID] == 20)
                        {
                            continue;
                        }
                        switch (elementDataType)
                        {

                        case 1: // element stress (tensor component)
                            float Sav[6];
                            int comp;
                            for (comp = 0; comp < 6; ++comp)
                            {
                                // average stresses over integration points
                                Sav[comp] = 0.0;
                                int intPoint;
                                for (intPoint = 0; intPoint < NumInterpol; ++intPoint)
                                {
                                    Sav[comp] += ShellData[CoviseElement * NumShellVar + comp + jumpIntegPoints * intPoint];
                                }
                                Sav[component] /= NumInterpol;
                            }
                            switch (component)
                            {
                            case 0:
                            case 1:
                            case 2:
                            case 3:
                            case 4:
                            case 5:
                                *s_el++ = Sav[component - 1];
                                break;
                            /*
                                          case 2:
                                                            *s_el++ = ShellData[CoviseElement*NumShellVar+1];
                                                  break;
                                                    case 3:
                                                            *s_el++ = ShellData[CoviseElement*NumShellVar+2];
                                                  break;
                                          case 4:
                                                            *s_el++ = ShellData[CoviseElement*NumShellVar+3];
                                                  break;
                                          case 5:
                              *s_el++ = ShellData[CoviseElement*NumShellVar+4];
                              break;
                              case 6:
                              *s_el++ = ShellData[CoviseElement*NumShellVar+5];
                              break;
                              */
                            case 6:
                                druck = Sav[0];
                                druck += Sav[1];
                                druck += Sav[2];
                                druck /= -3.0;
                                *s_el++ = druck;
                                break;
                            case 7:
                                druck = Sav[0];
                                druck += Sav[1];
                                druck += Sav[2];
                                druck /= 3.0;
                                vmises = (Sav[0] - druck) * (Sav[0] - druck);
                                vmises += (Sav[1] - druck) * (Sav[1] - druck);
                                vmises += (Sav[2] - druck) * (Sav[2] - druck);
                                vmises += 2.0f * Sav[3] * Sav[3];
                                vmises += 2.0f * Sav[4] * Sav[4];
                                vmises += 2.0f * Sav[5] * Sav[5];
                                vmises = (float)sqrt(1.5 * vmises);
                                *s_el++ = vmises;
                                break;
                            }

                            break;

                        case 2: // plastic strain
                            *s_el++ = (ShellData[CoviseElement * NumShellVar + 6 * SomeFlags[0]] + ShellData[CoviseElement * NumShellVar + 6 * SomeFlags[0] + jumpIntegPoints] + ShellData[CoviseElement * NumShellVar + 6 * SomeFlags[0] + jumpIntegPoints * 2]) / 3;
                            break;

                        case 3: // Thickness: NEIPS is 0!!!!!!
                            *s_el++ = ShellData[CoviseElement * NumShellVar + NumInterpol * (6 * SomeFlags[0] + SomeFlags[1]) + 8 * SomeFlags[2]];
                            break;
                        }
                    }
                }

                // Beams
                for (auto i: materials[ID].beam)
                {
                    assert(BeamMatIds[i] == ID);
                    if (beamTab[i].visible)
                    {

                        *tl++ = UnstructuredGrid::BAR;
                        *el++ = elemNo;
                        conNo = My_elemList[ID][beamTab[i].coElem];
                        for (j = 0; j < 2; j++)
                        {
                            *cl++ = conList[ID][conNo + j];
                        }
                        elemNo += 2;

                        // element data
                        switch (elementDataType)
                        {

                        case 1: // element stress (tensor component)
                            // For beams no stress available.
                            *s_el++ = 0.0;
                            break;

                        case 2: // plastic strain
                            // For beams no plastic strain available.
                            *s_el++ = 0.0;
                            ;
                            break;
                        }
                    }
                }

                // nodal data
                for (i = 0; i < numcoo[ID]; i++)
                {
                    x_c[i] = DisCo[(coordLookup[ID][i] - 1) * 3];
                    y_c[i] = DisCo[(coordLookup[ID][i] - 1) * 3 + 1];
                    z_c[i] = DisCo[(coordLookup[ID][i] - 1) * 3 + 2];

                    switch (nodalDataType)
                    {

                    case 0:
                        break;

                    case 1: // displacements
                        vx_out[i] = x_c[i] - Coord[(coordLookup[ID][i] - 1) * 3];
                        vy_out[i] = y_c[i] - Coord[(coordLookup[ID][i] - 1) * 3 + 1];
                        vz_out[i] = z_c[i] - Coord[(coordLookup[ID][i] - 1) * 3 + 2];
                        break;

                    case 2: // nodal velocity
                        vx_out[i] = NodeData[(coordLookup[ID][i] - 1) * 3];
                        vy_out[i] = NodeData[(coordLookup[ID][i] - 1) * 3 + 1];
                        vz_out[i] = NodeData[(coordLookup[ID][i] - 1) * 3 + 2];
                        break;
                    case 3: // nodal acceleration
                        vx_out[i] = NodeData[(coordLookup[ID][i] - 1) * 3];
                        vy_out[i] = NodeData[(coordLookup[ID][i] - 1) * 3 + 1];
                        vz_out[i] = NodeData[(coordLookup[ID][i] - 1) * 3 + 2];
                        break;
                    }
                }
                *el++ = elemNo;

            } // end of if (MDLOpt ...)
            else
            {
#ifdef SENDINFO
                m_module->sendError("LS-DYNA Plotfile has wrong version. Only support for version LS-930 and higher.");
#endif
                //exit(-1);
            }
#if 0
            grids_out[IDcount] = grid_out;
            Vertexs_out[IDcount] = Vertex_out;
            Scalars_out[IDcount] = Scalar_out;
#else
            grid_out->setBlock(ID);
            grid_out->setTimestep(timestep);
            grid_out->setRealTime(m_currentTime);
            token.addObject(gridPort, grid_out);
            if (Vertex_out) {
                Vertex_out->setBlock(ID);
                Vertex_out->setTimestep(timestep);
                Vertex_out->setRealTime(m_currentTime);
                token.addObject(vectorPort, Vertex_out);
            }
            if (Scalar_out) {
                Scalar_out->setBlock(ID);
                Scalar_out->setTimestep(timestep);
                Vertex_out->setRealTime(m_currentTime);
                token.addObject(scalarPort, Scalar_out);
            }
#endif
            IDcount++;

        } // end of if ( IDLISTE[ID] && numelem[ID] > 0)
    } // end of for (ID=0 ...)

#if 0
    // mark end of set array
    grids_out[IDcount] = NULL;
    Vertexs_out[IDcount] = NULL;
    Scalars_out[IDcount] = NULL;

    sprintf(name, "%s_%d", Grid, timestep);
    grid_set_out = new coDoSet(name, (coDistributedObject **)grids_out);

    // aw: only creates sets when creating data at the port at all
    if (nodalDataType > 0)
    {
        sprintf(name, "%s_%d", Vertex, timestep);
        Vertex_set_out = new coDoSet(name, (coDistributedObject **)Vertexs_out);
    }
    else
        Vertex_set_out = NULL;

    if (elementDataType > 0)
    {
        sprintf(name, "%s_%d", Scalar, timestep);
        Scalar_set_out = new coDoSet(name, (coDistributedObject **)Scalars_out);
    }
    else
        Scalar_set_out = NULL;

    // free
    for (i = 0; i <= IDcount; i++)
    {
        delete grids_out[i];
        delete Vertexs_out[i];
        delete Scalars_out[i];
    }
    delete[] grids_out;
    delete[] Vertexs_out;
    delete[] Scalars_out;
#endif
}

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::deleteStateData()
{
    // free
    delete[] DisCo;
    DisCo = nullptr;
    delete[] NodeData;
    NodeData = nullptr;
    delete[] SolidData;
    SolidData = nullptr;
    delete[] TShellData;
    TShellData = nullptr;
    delete[] BeamData;
    BeamData = nullptr;
    delete[] ShellData;
    ShellData = nullptr;
    delete[] DelTab;
    DelTab = nullptr;
}

template<int wordsize, class INTEGER, class REAL>
void Dyna3DReader<wordsize,INTEGER,REAL>::byteswap(WORD *buffer, INTEGER length)
{

    for (INTEGER ctr = 0; ctr < length; ctr++)
    {
        buffer[ctr] = vistle::byte_swap<vistle::little_endian, vistle::big_endian>(buffer[ctr]);
    }
}

Dyna3DReaderBase::Dyna3DReaderBase(Reader *module)
: m_module(module)
{
}

Dyna3DReaderBase::~Dyna3DReaderBase()
{
}

void Dyna3DReaderBase::setPorts(Port *grid, Port *scalar, Port *vector)
{
    gridPort = grid;
    scalarPort = scalar;
    vectorPort = vector;
}

void Dyna3DReaderBase::setFilename(const std::string &d3plot) {

    m_filename = d3plot;
}

void Dyna3DReaderBase::setByteswap(Dyna3DReaderBase::BYTESWAP bs) {

    byteswapFlag = bs;
}

void Dyna3DReaderBase::setFormat(Dyna3DReaderBase::FORMAT f) {

    format = f;

    std::string ciform;
    switch (format)
    {
    case GERMAN:
        ciform = "cadfem";
        break;

    case US:
        ciform = "original";
        break;

    default:
#ifdef SENDINFO
        m_module->sendError("ERROR: Incorrect file format selected");
#endif
        break;
    };

    std::cerr << "Dyna3DReader::readDyna3D info: selecting format " << ciform << std::endl;
}

void Dyna3DReaderBase::setOnlyGeometry(bool onlygeo) {

    only_geometry = onlygeo;
}

void Dyna3DReaderBase::setPartSelection(const std::string &parts)
{
    selection = parts;
}

std::string nextAdapt(const std::string &cur) {

    if (cur.empty())
        return "aa";

    assert(cur.length() == 2);
    if (cur == "zz")
        return "end";

    std::string next = cur;
    if (next[1] == 'z') {
        next[1] = 'a';
        ++next[0];
    } else {
        ++next[1];
    }

    return next;
}

std::string suffixForCount(int count) {

    if (count <= 0)
        return "";

    if (count >= 1000)
        return "XXX";

    char buf[100];
    if (count < 100)
        snprintf(buf, sizeof(buf), "%02d", count);
    else
        snprintf(buf, sizeof(buf), "%03d", count);

    return buf;
}

bool Dyna3DReaderBase::examine()
{
    m_numBlocks = -1;
    m_numTimesteps = -1;

    fs::path rootfile(m_filename);
    auto dir = rootfile.parent_path();
    auto name = rootfile.filename();

    struct Adaptation {
        std::string name;
        int numfiles = 0;
        size_t firstlen = 0;
        size_t len = 0;
        size_t lastlen = 0;

        size_t geomlen = 0;
        size_t statelen = 0;
    };
    std::map<std::string, Adaptation> adaptations;
    for (std::string adapt=""; adapt != "end"; adapt = nextAdapt(adapt)) {
        bool found = false;
        for (int count=0; count<1000; ++count) {
            fs::path curfile = rootfile;
            curfile += adapt + suffixForCount(count);
            boost::system::error_code ec;
            if (!exists(curfile, ec) || ec) {
                std::cerr << "filesystem error while checking for existance of " << curfile.string() << ":" << ec << std::endl;
                break;
            }
            size_t size = file_size(curfile, ec);
            if (ec) {
                std::cerr << "filesystem error while checking length of " << curfile.string() << ":" << ec << std::endl;
                break;
            } else if (size == 0) {
                std::cerr << "length of " << curfile.string() << " is zero" << std::endl;
                break;
            }
            found = true;
            if (count == 0) {
                adaptations[adapt].firstlen = size;
            } else if (count == 1) {
                adaptations[adapt].len = size;
            }
            adaptations[adapt].lastlen = size;
            adaptations[adapt].numfiles = count;
            adaptations[adapt].name = adapt;
        }
        if (!found)
            break;
    }

    m_numTimesteps = 0;
    std::string filename = m_filename;
    for (auto &a: adaptations) {
        int numtime = 0;
        auto &adapt = a.second;
        if (taurusinit_() != 0)
            return false;

        m_filename = filename + a.first;
        if (rdtaucntrl_(format) != 0)
            return false;

        adapt.geomlen = NumberOfWords * m_wordsize;
        adapt.statelen = NumWords * m_wordsize;

        // count initial state as one step
        if (adapt.firstlen >=  adapt.geomlen)
            ++numtime;
        // steps contained in first file after geometry
        numtime += (adapt.firstlen-adapt.geomlen)/adapt.statelen;
        // last file
        if (adapt.numfiles > 0) {
            numtime += adapt.lastlen/adapt.statelen;
        }
        // other files
        if (adapt.numfiles > 1) {
            numtime += (adapt.len/adapt.statelen) * (adapt.numfiles-2);
        }

        //std::cerr << "found " << numtime << " steps in adaptation '" << adapt.name  << "' contained in " << adapt.numfiles << " files" << std::endl;
        m_numTimesteps += numtime;
    }

    m_filename = filename;

    if (taurusinit_() != 0)
        return false;
    if (rdtaucntrl_(format) != 0)
        return false;

    return true;
}

Index Dyna3DReaderBase::numBlocks() const
{
    return m_numBlocks;
}

int Dyna3DReaderBase::numTimesteps() const
{
    return m_numTimesteps;
}

template class Dyna3DReader<4>;
//template class Dyna3DReader<8>;
