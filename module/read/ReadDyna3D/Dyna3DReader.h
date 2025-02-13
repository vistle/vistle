#ifndef VISTLE_READDYNA3D_DYNA3DREADER_H
#define VISTLE_READDYNA3D_DYNA3DREADER_H

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

#include <vistle/util/coRestraint.h>

#include <vistle/module/reader.h>

#include "Element.h"

#include <vector>
#include <shared_mutex>

#include <vistle/util/enum.h>

DEFINE_ENUM_WITH_STRING_CONVERSIONS(NodalDataType, (No_Node_Data)(Displacements)(Velocities)(Accelerations))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ElementDataType, (No_Element_Data)(Stress_Tensor)(Plastic_Strain)(Thickness))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Component, (Sx)(Sy)(Sz)(Txy)(Tyz)(Txz)(Pressure)(Von_Mises))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(Format, (CADFEM)(Original)(Guess))
// format of cadfem
//DEFINE_ENUM_WITH_STRING_CONVERSIONS(CadfemFormat, (GERMAN)(US))

DEFINE_ENUM_WITH_STRING_CONVERSIONS(ByteSwap, (Off)(On)(Auto))


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
public:
    using NodalDataType = ::NodalDataType;
    using ElementDataType = ::ElementDataType;
    using Format = ::Format;
    using ByteSwap = ::ByteSwap;
    using Component = ::Component;

    explicit Dyna3DReaderBase(vistle::Reader *module);
    virtual ~Dyna3DReaderBase();

    void setPorts(vistle::Port *grid, vistle::Port *vector, vistle::Port *scalar);

    void setFilename(const std::string &d3plot);
    void setByteswap(ByteSwap bs);
    void setFormat(Format format);
    void setOnlyGeometry(bool onlygeo);
    void setPartSelection(const std::string &parts);
    void setNodalDataType(NodalDataType type);
    void setElementDataType(ElementDataType type);
    void setComponent(Component type);

    bool examine(bool rescan = true);

    virtual int readStart() = 0;
    virtual int readOnlyGeo(vistle::Reader::Token &token, int blockToRead) = 0;
    virtual int readState(vistle::Reader::Token &token, int timestep, int blockToRead) = 0;
    virtual int readFinish() = 0;

    vistle::Index numBlocks() const;
    int numTimesteps() const;

protected:
    virtual int taurusinit_() = 0;
    virtual int rdtaucntrl_() = 0;
    virtual int readTimestep() = 0;

    vistle::Port *gridPort = nullptr;
    vistle::Port *scalarPort = nullptr;
    vistle::Port *vectorPort = nullptr;

    vistle::Reader *m_module = nullptr;
    std::string m_filename;
    ByteSwap byteswapFlag = Auto;
    Format format = Original;
    bool only_geometry = false;

    vistle::Index m_numBlocks = 0;
    int m_numTimesteps = 0;

    // protect data of currently read timestep
    std::shared_timed_mutex m_timestepMutex;
    int m_timestepUseCount = 0;
    int m_currentTimestep = -1;
    double m_currentTime = 0.;

    int NumWords; //=0;
    int NumberOfWords; //=0;
    size_t m_wordsize = 0;

    NodalDataType nodalDataType = No_Node_Data;
    ElementDataType elementDataType = No_Element_Data;
    Component component = Von_Mises;

    bool isPartEnabled(int part) const;
    std::unique_ptr<vistle::coRestraint> selection;
};

template<int wordsize, class INTEGER = typename WordTraits<wordsize>::INTEGER,
         class REAL = typename WordTraits<wordsize>::REAL>
class Dyna3DReader: public Dyna3DReaderBase {
public:
    typedef typename WordTraits<wordsize>::WORD WORD;

    enum // maximum part ID
    {
        MAXTIMESTEPS = 1000
    };

private:
    struct tauio_ {
        REAL tau[512];
        INTEGER itrecin, nrin, nrzin, ifilin, itrecout, nrout, nrzout, ifilout, adaptation;
        const INTEGER irl = 512;
        INTEGER taulength;
        char adapt[3];

    } tauio_1;

    //  member functions
    void byteswap(WORD *buffer, INTEGER length);
    void byteswap(REAL *buffer, INTEGER length);

    char InfoBuf[1000];

    // Some old global variables have been hidden hier:
    int NumIDs = 0;
    int ExistingStates = 0; // initially = 0;
    int MinState = 1; // initially = 1;
    int State = 1; // initially = 1;

    // More "old" global variables
    std::vector<bool> IDLISTE;
    int *numcoo = nullptr, *numcon = nullptr, *numelem = nullptr;

    struct Material {
        std::vector<INTEGER> solid, tshell, shell, beam;
    };
    std::vector<Material> materials;

    // More and more blocks of "old" global variables to be initialised to 0
    int *delElem = nullptr; // = NULL;
    int *delCon = nullptr; //  = NULL;

    std::vector<Element> solidTab;
    std::vector<Element> tshellTab;
    std::vector<Element> shellTab;
    std::vector<Element> beamTab;

    // storing coordinate positions for all time steps
    int **coordLookup = nullptr; // = NULL;

    // element and connection list for time steps (considering the deletion table)
    int **My_elemList = nullptr; // = NULL;
    int **conList = nullptr; //  = NULL;

    // node/element deletion table
    int *DelTab = nullptr; // = NULL;

    // Yes, certainly! More "old" global variables!
    int infile = -1;
    int numcoord = 0;
    int *NodeIds = nullptr; //=NULL;
    int *SolidNodes = nullptr; //=NULL;
    int *SolidMatIds = nullptr; //=NULL;
    int *BeamNodes = nullptr; //=NULL;
    int *BeamMatIds = nullptr; //=NULL;
    int *ShellNodes = nullptr; //=NULL;
    int *ShellMatIds = nullptr; //=NULL;
    int *TShellNodes = nullptr; //=NULL;
    int *TShellMatIds = nullptr; //=NULL;
    int *SolidElemNumbers = nullptr; //=NULL;
    int *BeamElemNumbers = nullptr; //=NULL;
    int *ShellElemNumbers = nullptr; //=NULL;
    int *TShellElemNumbers = nullptr; //=NULL;
    int *Materials = nullptr; //=NULL;

    /* Common Block Declarations */
    // Coordinates
    float *Coord = nullptr; //= NULL;
    // displaced coordinates
    float *DisCo = nullptr; //= NULL;
    float *NodeData = nullptr; //= NULL;
    float *SolidData = nullptr; //= NULL;
    float *TShellData = nullptr; //= NULL;
    float *ShellData = nullptr; //= NULL;
    float *BeamData = nullptr; //= NULL;

    // A gigantic block of "old" global variables
    char CTauin[300];
    char CTrace, CEndin, COpenin;
    float TimestepTime; //= 0.0;
    float version; //= 0.0;
    int NumSolidElements; //= 0;
    int NumBeamElements; //= 0;
    int NumShellElements; //= 0;
    int NumTShellElements; //= 0;
    int NumMaterials; //=0;
    int NumDim; //=0;
    int NumGlobVar; //=0;
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

    //  Parameter names

#if 0
    //  object names
    const char *Grid = nullptr;
    const char *Scalar = nullptr;
    const char *Vertex = nullptr;
#endif

    //  Local data

    //  Shared memory data

    //  LS-DYNA3D specific member functions

    int readStart() override;
    int readOnlyGeo(vistle::Reader::Token &token, int blockToRead) override;
    int readState(vistle::Reader::Token &token, int state, int block) override;
    int readFinish() override;
    void visibility();

    void createElementLUT();
    void deleteElementLUT();
    void createGeometryList();
    void createGeometry(vistle::Reader::Token &token, int blockToRead = -1) const;
    INTEGER createNodeLUT(int id, int *lut);

    void createStateObjects(vistle::Reader::Token &token, int timestep, int block = -1) const;
    void deleteStateData();

    /* Subroutines */
    int taurusinit_() override;
    int rdtaucntrl_() override;
    int taugeoml_();
    int taustatl_();
    int rdtaucoor_();
    int rdtauelem_(Format format);
    int rdtaunum_(Format format);
    int rdstate_(vistle::Reader::Token &token, int istate, int block);
    int readTimestep() override;
    int rdrecr_(float *val, const int *istart, int n);
    int rdreci_(int *ival, const int *istart, const int *n, Format format);
    int grecaddr_(INTEGER i, INTEGER istart, INTEGER *iz, INTEGER *irdst);
    int placpnt_(int *istart);
    int otaurusr_();

    int nrelem = 0; // number of rigid body shell elements
    int nmmat = 0; // number of materials in the database
    int *matTyp = NULL;
    int *MaterialTables = NULL;

#if 0
    // Ports
    coOutputPort *p_grid;
    coOutputPort *p_data_1;
    coOutputPort *p_data_2;

    // Parameters
    coFileBrowserParam *p_data_path;
    coChoiceParam *p_nodalDataType;
    coChoiceParam *p_elementDataType;
    coChoiceParam *p_component;
    coStringParam *p_Selection;
    // coIntSliderParam *p_State;
    coIntVectorParam *p_Step;
    coChoiceParam *p_format;
    coBooleanParam *p_only_geometry;
    coChoiceParam *p_byteswap;
#endif

public:
    /* Constructor */
    Dyna3DReader(vistle::Reader *module);

    ~Dyna3DReader() override;
};

extern template class Dyna3DReader<4>;
extern template class Dyna3DReader<8>;

//typedef Dyna3DReader<4> Dyna3DReader32;
//typedef Dyna3DReader<8> Dyna3DReader64;
#endif
