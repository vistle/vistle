#ifndef VISIT_DATA_TYPES_H
#define VISIT_DATA_TYPES_H

typedef int visit_handle;

constexpr auto VISIT_INVALID_HANDLE = -1;

constexpr auto VISIT_ERROR = 0;
constexpr auto VISIT_OKAY = 1;
constexpr auto VISIT_NODATA = 2;

/* Mesh Types */
typedef enum {
    VISIT_MESHTYPE_UNKNOWN = 0,
    VISIT_MESHTYPE_RECTILINEAR = 1,
    VISIT_MESHTYPE_CURVILINEAR = 2,
    VISIT_MESHTYPE_UNSTRUCTURED = 3,
    VISIT_MESHTYPE_POINT = 4,
    VISIT_MESHTYPE_CSG = 5,
    VISIT_MESHTYPE_AMR = 6
} VisIt_MeshType;

/* Centering */
typedef enum {
    VISIT_VARCENTERING_NODE = 0,
    VISIT_VARCENTERING_ZONE
} VisIt_VarCentering;

/* Variable Type */
typedef enum {
    /* Unknown variable type */
    VISIT_VARTYPE_UNKNOWN = 0,
    /* Field variable types*/
    VISIT_VARTYPE_SCALAR = 1,
    VISIT_VARTYPE_VECTOR = 2,
    VISIT_VARTYPE_TENSOR = 3,
    VISIT_VARTYPE_SYMMETRIC_TENSOR = 4,
    VISIT_VARTYPE_MATERIAL = 5,
    VISIT_VARTYPE_MATSPECIES = 6,
    VISIT_VARTYPE_LABEL = 7,
    VISIT_VARTYPE_ARRAY = 8,
    /* Mesh variable types (expressions only) */
    VISIT_VARTYPE_MESH = 100,
    VISIT_VARTYPE_CURVE = 101
} VisIt_VarType;

typedef enum {
    VISIT_ENUMTYPE_NONE = 0,
    VISIT_ENUMTYPE_BY_VALUE = 1,
    VISIT_ENUMTYPE_BY_RANGE = 2,
    VISIT_ENUMTYPE_BY_BITMASK = 3,
    VISIT_ENUMTYPE_BY_NCHOOSER = 4
} VisIt_EnumType;

/* Command Argument Type */
#define VISIT_CMDARG_NONE                 0
#define VISIT_CMDARG_INT                  1
#define VISIT_CMDARG_FLOAT                2
#define VISIT_CMDARG_STRING               3

/* Simulation Mode */
#define VISIT_SIMMODE_UNKNOWN             0
#define VISIT_SIMMODE_RUNNING             1
#define VISIT_SIMMODE_STOPPED             2

/* Data Type */
#define VISIT_DATATYPE_CHAR               0
#define VISIT_DATATYPE_INT                1
#define VISIT_DATATYPE_FLOAT              2
#define VISIT_DATATYPE_DOUBLE             3
#define VISIT_DATATYPE_LONG               4
#define VISIT_DATATYPE_STRING             10

/* Array Owner */
#define VISIT_OWNER_SIM                   0
#define VISIT_OWNER_VISIT                 1
#define VISIT_OWNER_COPY                  2
#define VISIT_OWNER_VISIT_EX              3

/* Array memory layout */
#define VISIT_MEMORY_CONTIGUOUS           0
#define VISIT_MEMORY_STRIDED              1

/* Cell Types */
#define VISIT_CELL_BEAM                   0
#define VISIT_CELL_TRI                    1
#define VISIT_CELL_QUAD                   2
#define VISIT_CELL_TET                    3
#define VISIT_CELL_PYR                    4
#define VISIT_CELL_WEDGE                  5
#define VISIT_CELL_HEX                    6
#define VISIT_CELL_POINT                  7
#define VISIT_CELL_POLYHEDRON             8

#define VISIT_CELL_QUADRATIC_EDGE         20
#define VISIT_CELL_QUADRATIC_TRI          21
#define VISIT_CELL_QUADRATIC_QUAD         22
#define VISIT_CELL_QUADRATIC_TET          23
#define VISIT_CELL_QUADRATIC_PYR          24
#define VISIT_CELL_QUADRATIC_WEDGE        25
#define VISIT_CELL_QUADRATIC_HEX          26

#define VISIT_CELL_BIQUADRATIC_TRI        27
#define VISIT_CELL_BIQUADRATIC_QUAD       28
#define VISIT_CELL_TRIQUADRATIC_HEX       29
#define VISIT_CELL_QUADRATIC_LINEAR_QUAD         30
#define VISIT_CELL_QUADRATIC_LINEAR_WEDGE        31
#define VISIT_CELL_BIQUADRATIC_QUADRATIC_WEDGE   32
#define VISIT_CELL_BIQUADRATIC_QUADRATIC_HEX     33

/* Coordinate modes */
#define VISIT_COORD_MODE_SEPARATE         0
#define VISIT_COORD_MODE_INTERLEAVED      1

/* Ghost cell types */
#define VISIT_GHOSTCELL_REAL                  0
#define VISIT_GHOSTCELL_INTERIOR_BOUNDARY     1
#define VISIT_GHOSTCELL_EXTERIOR_BOUNDARY     2
#define VISIT_GHOSTCELL_ENHANCED_CONNECTIVITY 4
#define VISIT_GHOSTCELL_REDUCED_CONNECTIVITY  8
#define VISIT_GHOSTCELL_BLANK                 16
#define VISIT_GHOSTCELL_REFINED_AMR_CELL      32

/* Ghost node types */
#define VISIT_GHOSTNODE_REAL                  0
#define VISIT_GHOSTNODE_INTERIOR_BOUNDARY     1
#define VISIT_GHOSTNODE_BLANK                 2
#define VISIT_GHOSTNODE_COARSE_SIDE           3
#define VISIT_GHOSTNODE_FINE_SIDE             4


constexpr auto VISIT_DOMAINLIST = 12;
constexpr auto VISIT_DOMAIN_BOUNDARIES = 13;
constexpr auto VISIT_DOMAIN_NESTING = 14;

constexpr auto VISIT_VARIABLE_DATA = 15;

constexpr auto VISIT_CURVILINEAR_MESH = 20;
constexpr auto VISIT_CSG_MESH = 21;
constexpr auto VISIT_POINT_MESH = 22;
constexpr auto VISIT_RECTILINEAR_MESH = 23;
constexpr auto VISIT_UNSTRUCTURED_MESH = 24;

constexpr auto VISIT_CURVE_DATA = 30;

constexpr auto VISIT_MATERIAL_DATA = 40;
constexpr auto VISIT_SPECIES_DATA = 50;

constexpr auto VISIT_SIMULATION_METADATA = 100;
constexpr auto VISIT_MESHMETADATA = 101;
constexpr auto VISIT_VARIABLEMETADATA = 102;
constexpr auto VISIT_MATERIALMETADATA = 103;
constexpr auto VISIT_CURVEMETADATA = 104;
constexpr auto VISIT_EXPRESSIONMETADATA = 105;
constexpr auto VISIT_SPECIESMETADATA = 106;
constexpr auto VISIT_NAMELIST = 107;
constexpr auto VISIT_COMMANDMETADATA = 108;
constexpr auto VISIT_OPTIONLIST = 109;

constexpr auto VISIT_MESSAGEMETADATA = 110;

constexpr auto VISIT_VIEW3D = 150;
constexpr auto VISIT_VIEW2D = 151;

#endif //VISIT_DATA_TYPES_H