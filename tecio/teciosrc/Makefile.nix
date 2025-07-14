BOOST_ROOT = /3rdpartylibs/boost/1.69.0/build/linux64-centos6.10/release/include
#BOOST_ROOT = /3rdpartylibs/boost/1.69.0/build/macos10.15/release/include

SRCS = arrlist.cpp \
       AsciiOutputInfo.cpp \
       auxdata.cpp \
       checkPercentDone.cpp \
       dataio.cpp \
       dataio4.cpp \
       dataset.cpp \
       dataset0.cpp \
       DataSetWriter.cpp \
       datautil.cpp \
       exportSubzonePlt.cpp \
       FaceNeighborGeneratorAbstract.cpp \
       FECellSubzoneCompressor.cpp \
       FieldData.cpp \
       FieldData_s.cpp \
       FileDescription.cpp \
       FileIOStream.cpp \
       filestream.cpp \
       FileStreamReader.cpp \
       FileStreamWriter.cpp \
       fileStuff.cpp \
       FileSystem.cpp \
       geom2.cpp \
       Geom_s.cpp \
       IJKSubzoneInfo.cpp \
       IJKZoneInfo.cpp \
       importSzPltFile.cpp \
       IntervalTree.cpp \
       MinMaxTree.cpp \
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
       q_msg.cpp \
       readValueArray.cpp \
       Scanner.cpp \
       set.cpp \
       strlist.cpp \
       strutil.cpp \
       SZLFEPartitionedZoneHeaderWriter.cpp \
       SZLFEPartitionedZoneWriter.cpp \
       SZLFEPartitionWriter.cpp \
       SZLFEZoneHeaderWriter.cpp \
       SZLFEZoneWriter.cpp \
       SZLOrderedPartitionedZoneHeaderWriter.cpp \
       SZLOrderedPartitionedZoneWriter.cpp \
       SZLOrderedPartitionWriter.cpp \
       SZLOrderedZoneHeaderWriter.cpp \
       SZLOrderedZoneWriter.cpp \
       tecio.cpp \
       TecioData.cpp \
       TecioPLT.cpp \
       TecioSZL.cpp \
       TecioTecUtil.cpp \
       TranslatedString.cpp \
       UnicodeStringUtils.cpp \
       writeValueArray.cpp \
       Zone_s.cpp \
       ZoneHeaderWriterAbstract.cpp \
       ZoneInfoCache.cpp \
       ZoneVarMetadata.cpp \
       ZoneWriterAbstract.cpp \
       ZoneWriterFactory.cpp

TECIOLIB = libtecio.a
SZCOMBINE = szcombine
OBJS = $(SRCS:%.cpp=%.o)

CXX = g++
CXXFLAGS = -DLINUX \
		   -DLINUX64 \
		   -DTP_PROJECT_USES_BOOST \
		   -DBOOST_ALL_NO_LIB \
		   -DMAKEARCHIVE \
		   -DNO_THIRD_PARTY_LIBS \
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

all: $(TECIOLIB) $(SZCOMBINE)

clean:
	rm *.o *.a szcombine

$(OBJS) : %.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

$(TECIOLIB) : $(OBJS)
	$(AR) $(ARFLAGS) $(TECIOLIB) $(OBJS)

$(SZCOMBINE) : szcombine.cpp $(TECIOLIB)
	$(CXX) $< -o $@ $(TECIOLIB) -lpthread
