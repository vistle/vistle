BOOST_ROOT = /3rdpartylibs/boost/1.69.0/build/linux64-centos6.10/release/include
#BOOST_ROOT = /3rdpartylibs/boost/1.69.0/build/macos10.15/release/include

SRCS = AsciiOutputInfo.cpp \
       checkPercentDone.cpp \
       DataSetWriter.cpp \
       DataSetWriterMPI.cpp \
       exportSubzonePlt.cpp \
       FaceNeighborGeneratorAbstract.cpp \
       FECellSubzoneCompressor.cpp \
       FieldData.cpp \
       FieldData_s.cpp \
       FileDescription.cpp \
       FileIOStream.cpp \
       FileStreamReader.cpp \
       FileStreamWriter.cpp \
       fileStuff.cpp \
       FileSystem.cpp \
       IJKSubzoneInfo.cpp \
       IJKZoneInfo.cpp \
       importSzPltFile.cpp \
       IntervalTree.cpp \
       MinMaxTree.cpp \
       MPICommunicationCache.cpp \
       MPICommunicator.cpp \
       mpiDatatype.cpp \
       MPIFileIOStream.cpp \
       MPIFileReader.cpp \
       MPIFileWriter.cpp \
       MPINonBlockingCommunicationCollection.cpp \
       MPIUtil.cpp \
       NodeMap.cpp \
       NodeMap_s.cpp \
       NodeToElemMap_s.cpp \
       NonSzFEZoneConnectivityWriter.cpp \
       NonSzFEZoneFaceNeighborGenerator.cpp \
       NonSzFEZoneWriter.cpp \
       NonSzOrderedZoneFaceNeighborGenerator.cpp \
       NonSzOrderedZoneWriter.cpp \
       NonSzZoneFaceNeighborWriter.cpp \
       NonSzZoneHeaderWriter.cpp \
       NonSzZoneVariableWriter.cpp \
       NonSzZoneWriterAbstract.cpp \
       OrbFESubzonePartitioner.cpp \
       OrthogonalBisection.cpp \
       PartitionTecUtilDecorator.cpp \
       readValueArray.cpp \
       Scanner.cpp \
       SZLFEPartitionedZoneHeaderWriter.cpp \
       SZLFEPartitionedZoneWriter.cpp \
       SZLFEPartitionedZoneWriterMPI.cpp \
       SZLFEPartitionWriter.cpp \
       SZLFEZoneHeaderWriter.cpp \
       SZLFEZoneWriter.cpp \
       SZLOrderedPartitionedZoneHeaderWriter.cpp \
       SZLOrderedPartitionedZoneWriter.cpp \
       SZLOrderedPartitionedZoneWriterMPI.cpp \
       SZLOrderedPartitionWriter.cpp \
       SZLOrderedZoneHeaderWriter.cpp \
       SZLOrderedZoneWriter.cpp \
       tecio.cpp \
       TecioData.cpp \
       TecioSZL.cpp \
       TecioTecUtil.cpp \
       UnicodeStringUtils.cpp \
       writeValueArray.cpp \
       Zone_s.cpp \
       ZoneHeaderWriterAbstract.cpp \
       ZoneInfoCache.cpp \
       ZoneVarMetadata.cpp \
       ZoneWriterAbstract.cpp \
       ZoneWriterFactory.cpp \
       ZoneWriterFactoryMPI.cpp

TECIOLIB = libteciompi.a
SZCOMBINE = szcombine
OBJS = $(SRCS:%.cpp=%.o)

CXX = g++
CXXFLAGS = -DLINUX \
		   -DLINUX64 \
		   -DTP_PROJECT_USES_BOOST \
		   -DBOOST_ALL_NO_LIB \
		   -DMAKEARCHIVE \
		   -DNO_THIRD_PARTY_LIBS \
		   -DTECIOMPI \
		   -DOMPI_SKIP_MPICXX \
		   -DMPICH_SKIP_MPICXX \
		   -DMPI_NO_CPPBIND \
		   -Dtecio_EXPORTS \
		   -fmessage-length=0 \
		   -fPIC \
		   -isystem \
		   $(BOOST_ROOT) \
		   -fvisibility=hidden  \
		   -w \
		   -O3 \
		   -DNDEBUG \
		   -DNO_ASSERTS \
		   -std=c++14
# Add the following for debug builds:
# -g -D_DEBUG \
AR = ar
ARFLAGS = -rc

LDFLAGS = -fPIC -fmessage-length=0 -fvisibility=hidden -Wall

MPICXX = mpicxx

all: $(TECIOLIB) $(SZCOMBINE)

clean:
	rm *.o *.a szcombine

$(OBJS) : %.o: %.cpp
	$(MPICXX) -c $(CXXFLAGS) $< -o $@

$(TECIOLIB) : $(OBJS)
	$(AR) $(ARFLAGS) $(TECIOLIB) $(OBJS)

$(SZCOMBINE) : szcombine.cpp $(TECIOLIB)
	$(MPICXX) $< -o $@ $(TECIOLIB)
