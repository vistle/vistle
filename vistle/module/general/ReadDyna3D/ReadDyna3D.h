/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#ifndef _READDYNA3D_H
#define _READDYNA3D_H
/**************************************************************************\ 
 **                                                           (C)1995 RUS  **
 **                                                                        **
 ** Description: Read module for Dyna3D data         	                  **
 **                                                                        **
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
/////////////////////////////////////////////////////////
//                I M P O R T A N T
/////////////////////////////////////////////////////////
// NumState keeps the number of time steps to be read
// (apart from the first one).
// MaxState is kept equal to MinState+NumState.
// Now State is also a redundant variable which is kept
// equal to MaxState.
// These redundancies have been introduced in order to facilitate
// that the authors of this code (or perhaps
// someone else who has the courage to accept
// the challenge) may easily recognise its original state and
// eventually improve its functionality and correct
// its original flaws in spite of the
// modifications that have been introduced to make
// the parameter interface (of parameter p_Step,
// the former parameter p_State) simpler.
// These last changes have not contributed perhaps
// to make the code more clear, but also not
// darker as it already was.
//
// The main problem now is how to determine the first time step
// to be read. It works for the first (if MinState==1), but
// the behaviour is not very satisfactory in other cases.
// This depends on how many time steps are contained per file.
// A second improvement would be to read time steps with
// a frequency greater than 1.
/////////////////////////////////////////////////////////

//#include <appl/ApplInterface.h>

#include <util/coRestraint.h>
#include <core/unstr.h>
#include <core/vec.h>

#include <module/reader.h>

#include "Element.h"
#include "Dyna3DReader.h"

class ReadDyna3D: public vistle::Reader
{
public:
private:
    bool prepareRead() override;
    bool read(Token &token, int timestep=-1, int block=-1) override;
    bool examine(const vistle::Parameter *param) override;
    bool finishRead() override;

    Dyna3DReaderBase *newReader();

    // Ports
    vistle::Port *p_grid = nullptr;
    vistle::Port *p_data_1 = nullptr;
    vistle::Port *p_data_2 = nullptr;

    // Parameters
    vistle::StringParameter *p_data_path;
    vistle::IntParameter *p_nodalDataType;
    vistle::IntParameter *p_elementDataType;
    vistle::IntParameter *p_component;
    vistle::StringParameter *p_Selection;
    // coIntSliderParam *p_State;
    //vistle::IntVectorParameter *p_Step;
    vistle::IntParameter *p_format;
    vistle::IntParameter *p_only_geometry;
    vistle::IntParameter *p_byteswap;

public:
    ReadDyna3D(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadDyna3D() override;

    std::unique_ptr<Dyna3DReaderBase> dyna3dReader;
};


#if 0
template<int wordsize>
struct WordTraits;

template<>
struct WordTraits<4> {
    typedef uint32_t WORD;
    typedef int32_t INTEGER;
    typedef float REAL;
};

template<>
struct WordTraits<8> {
    typedef uint64_t WORD;
    typedef int64_t INTEGER;
    typedef double REAL;
};

class Dyna3DReaderBase {
};

template<int wordsize,
         class INTEGER=typename WordTraits<wordsize>::INTEGER,
         class REAL=typename WordTraits<wordsize>::REAL>
class Dyna3DReader: public Dyna3DReaderBase
{
public:
    typedef typename WordTraits<wordsize>::WORD WORD;

    enum // maximum part ID
    {
        MAXID = 200000,
        MAXTIMESTEPS = 1000
    };

    enum
    {
        BYTESWAP_OFF = 0x00,
        BYTESWAP_ON = 0x01,
        BYTESWAP_AUTO = 0x02
    };

private:
    struct
    {
        INTEGER itrecin, nrin, nrzin, ifilin, itrecout, nrout, nrzout, ifilout, irl, iwpr, adaptation;
        REAL tau[512];
        INTEGER taulength;
        char adapt[3];

    } tauio_;

    //  member functions
    void byteswap(WORD *buffer, size_t length);


    char ciform[9];
    char InfoBuf[1000];

    int byteswapFlag;

    // Some old global variables have been hidden hier:
    vistle::Index NumIDs = 0;
    int nodalDataType;
    int elementDataType;
    int component;
    int ExistingStates = 0;
    int MinState = 1;
    int MaxState = 1;
    int NumState = 0;
    int State = 1;

    // More "old" global variables
    INTEGER IDLISTE[MAXID];
    INTEGER *numcoo=nullptr, *numcon=nullptr, *numelem=nullptr;

    // Another block of "old" global variables
    INTEGER *maxSolid = nullptr;
    INTEGER *minSolid = nullptr;
    INTEGER *maxTShell = nullptr;
    INTEGER *minTShell = nullptr;
    INTEGER *maxShell = nullptr;
    INTEGER *minShell = nullptr;
    INTEGER *maxBeam = nullptr;
    INTEGER *minBeam = nullptr;

    // More and more blocks of "old" global variables to be initialised to 0
    INTEGER *delElem = nullptr;
    INTEGER *delCon = nullptr;

    Element **solidTab = nullptr;
    Element **tshellTab = nullptr;
    Element **shellTab = nullptr;
    Element **beamTab = nullptr;

    // storing coordinate positions for all time steps
    INTEGER **coordLookup = nullptr;

    // element and connection list for time steps (considering the deletion table)
    INTEGER **My_elemList = nullptr;
    INTEGER **conList = nullptr;

    // node/element deletion table
    INTEGER *DelTab = nullptr;

    vistle::UnstructuredGrid::const_ptr grid_out; //    = NULL;
    vistle::Vec<vistle::Scalar,3>::const_ptr Vertex_out; //  = NULL;
    vistle::Vec<vistle::Scalar>::const_ptr Scalar_out; // = NULL;

#if 0
    coDoUnstructuredGrid **grids_out; //        = NULL;
    coDoVec3 **Vertexs_out; //  = NULL;
    coDoFloat **Scalars_out; // = NULL;

    // Even two additional blocks of "old" global variables
    coDoSet *grid_set_out, *grid_timesteps;
    coDoSet *Vertex_set_out, *Vertex_timesteps;
    coDoSet *Scalar_set_out, *Scalar_timesteps;

    coDoSet *grid_sets_out[MAXTIMESTEPS];
    coDoSet *Vertex_sets_out[MAXTIMESTEPS];
    coDoSet *Scalar_sets_out[MAXTIMESTEPS];
#endif

    // Yes, certainly! More "old" global variables!
    int infile = -1;
    INTEGER numcoord;
    int *NodeIds; //=NULL;
    int *SolidNodes; //=NULL;
    int *SolidMatIds; //=NULL;
    int *BeamNodes; //=NULL;
    int *BeamMatIds; //=NULL;
    int *ShellNodes; //=NULL;
    int *ShellMatIds; //=NULL;
    int *TShellNodes; //=NULL;
    int *TShellMatIds; //=NULL;
    int *SolidElemNumbers; //=NULL;
    int *BeamElemNumbers; //=NULL;
    int *ShellElemNumbers; //=NULL;
    int *TShellElemNumbers; //=NULL;
    int *Materials; //=NULL;

    /* Common Block Declarations */
    // Coordinates
    REAL *Coord; //= NULL;
    // displaced coordinates
    REAL *DisCo; //= NULL;
    REAL *NodeData; //= NULL;
    REAL *SolidData; //= NULL;
    REAL *TShellData; //= NULL;
    REAL *ShellData; //= NULL;
    REAL *BeamData; //= NULL;

    // A gigantic block of "old" global variables
    char CTauin[300];
    bool CTrace = false;
    bool CEndin = false, COpenin = false;
    float TimestepTime; //= 0.0;
    float version; //= 0.0;
    int NumSolidElements; //= 0;
    int NumBeamElements; //= 0;
    int NumShellElements; //= 0;
    int NumTShellElements; //= 0;
    int NumMaterials; //=0;
    int NumDim; //=0;
    int NumGlobVar; //=0;
    int NumWords; //=0;
    int ITFlag; //=0;
    int IUFlag; //=0;
    int IVFlag; //=0;
    int IAFlag; //=0;
    int IDTDTFlag; //=0;
    int NumTemperature; //=0;
    int MDLOpt; //=0;
    int IStrn; //=0;
    int NumSolidMat; //=0;
    int NumSolidVar; //=0;
    int NumAddSolidVar; //=0;
    int NumBeamMat; //=0;
    int NumBeamVar; //=0;
    int NumAddBeamVar; //=0;
    int NumShellMat; //=0;
    int NumShellVar; //=0;
    int NumAddShellVar; //=0;
    int NumTShellMat; //=0;
    int NumTShellVar; //=0;
    int NumShellInterpol; //=0;
    int NumInterpol; //=0;
    int NodeAndElementNumbering; //=0;
    int SomeFlags[5];
    int NumFluidMat;
    int NCFDV1;
    int NCFDV2;
    int NADAPT;
    int NumBRS; //=0;
    int NodeSort; //=0;
    int NumberOfWords; //=0;
    int NumNodalPoints; //=0;
    int IZControll; //=0;
    int IZElements; //=0;
    int IZArb; //=0;
    int IZStat0; //=0;
    int ctrlLen_;

    // A last block
    int c__1; //= 1;
    int c__0; //= 0;
    int c__13; //= 13;
    int c__8; //= 8;
    int c__5; //= 5;
    int c__4; //= 4;
    int c__10; //= 10;
    int c__16; //= 16;

    int nrelem = 0; // number of rigid body shell elements
    int nmmat = 0; // number of materials in the database
    int *matTyp = NULL;
    int *MaterialTables = NULL;


    //  Parameter names
    std::string data_Path;
    bool m_onlyGeometry = false;

    //  object names
    const char *Grid;
    //const char *Scalar;
    const char *Vertex;

    //  Local data

    //  Shared memory data

    //  LS-DYNA3D specific member functions

    int readDyna3D();
    void visibility();

    void createElementLUT();
    void deleteElementLUT();
    void createGeometryList();
    void createGeometry();
    void createNodeLUT(int id, int *lut);

    void createStateObjects(int timestep);

    /* Subroutines */
    int taurusinit_();
    int taurusreset_();
    int rdtaucntrl_(char *);
    int taugeoml_();
    int taustatl_();
    int rdtaucoor_();
    int rdtauelem_(char *);
    int rdtaunum_(char *);
    int rdstate_(int *istate);
    int rdrecr_(float *val, int *istart, int *n);
    int rdreci_(int *ival, int *istart, int *n, char *ciform);
    int grecaddr_(int *i, int *istart, int *iz, int *irdst);
    int placpnt_(int *istart);
    int otaurusr_();

    // Ports
    vistle::Port *p_grid = nullptr;
    vistle::Port *p_data_1 = nullptr;
    vistle::Port *p_data_2 = nullptr;

    // Parameters
    vistle::StringParameter *p_data_path;
    vistle::IntParameter *p_nodalDataType;
    vistle::IntParameter *p_elementDataType;
    vistle::IntParameter *p_component;
    vistle::StringParameter *p_Selection;
    // coIntSliderParam *p_State;
    //vistle::IntVectorParameter *p_Step;
    vistle::IntParameter *p_format;
    vistle::IntParameter *p_only_geometry;
    vistle::IntParameter *p_byteswap;

public:
    ReadDyna3D(const std::string &name, int moduleID, mpi::communicator comm);
    ~ReadDyna3D() override;
};
#endif
#endif // _READDYNA3D_H
