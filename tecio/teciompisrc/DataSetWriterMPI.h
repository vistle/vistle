#include "DataSetWriter.h"
#include "ThirdPartyHeadersBegin.h"
#include <vector>
#include <mpi.h>
#include "ThirdPartyHeadersEnd.h"
namespace tecplot { namespace ___3931 { class ___37; class FileWriterInterface; class ___1842; }} namespace tecplot { namespace teciompi { class DataSetWriterMPI : public ___3931::DataSetWriter { public: DataSetWriterMPI( ___3931::___37*          ___36, ___3499                      vars, ___3499                      ___4668, ___3931::___1842 const&           maxIJKSubzoneSize, ItemAddress64::ItemOffset_t maxFESubzoneSize, MPI_Comm                    communicator, int                         mainProcess, int                         localProcess, bool                        flushToDisk = false); virtual ~DataSetWriterMPI(); virtual ___372 writeDataSet( tecplot::___3931::FileWriterInterface& szpltFile, uint32_t                             fileVersion, tecplot::___3931::___1390&        szpltZoneHeaderFileLocs); private: MPI_Comm m_communicator; int m_mainProcess; int m_localProcess; }; }}
