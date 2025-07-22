#include "NodeToElemMap_s.h"
#include "ThirdPartyHeadersBegin.h"
 #if !defined TECIOMPI
#include <boost/atomic.hpp>
 #endif
#include <boost/ref.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>
#include "ThirdPartyHeadersEnd.h"
#include "NodeMap_s.h"
 #if !defined TECIOMPI
#include "JobControl_s.h"
 #endif
#include "CodeContract.h"
using namespace tecplot::___3931;
 #define MIN_CELLS_FOR_MULTITHREAD 100000 
namespace tecplot { namespace tecioszl {
 #if !defined TECIOMPI
namespace { struct NodeToElemData { tecplot::tecioszl::___2728 const& ___2721; boost::scoped_array<tecplot::___3931::___463> const& elemIndex; boost::scoped_array<tecplot::___3931::___463>& elem; boost::scoped_array<boost::atomic<int> >& count; int32_t section; ___463 baseCell; ___463 begin; ___463 end; NodeToElemData( tecplot::tecioszl::___2728 const& ___2721, boost::scoped_array<tecplot::___3931::___463> const& elemIndex, boost::scoped_array<tecplot::___3931::___463>& elem, boost::scoped_array<boost::atomic<int> >& count, int32_t section, ___463 baseCell, ___463 begin, ___463 end) : ___2721(___2721) , elemIndex(elemIndex) , elem(elem) , count(count) , section(section) , baseCell(baseCell) , begin(begin) , end(end) {} }; void fillElemArrayForCellRange(___90 ___2122) { NodeToElemData* nodeToElemData = reinterpret_cast<NodeToElemData*>(___2122); for(___463 ___447 = nodeToElemData->begin; ___447 < nodeToElemData->end; ++___447) { int32_t nodesPerCell = nodeToElemData->___2721.m_numNodesPerCellPerSection[nodeToElemData->section]; for (int32_t ___2707 = 0; ___2707 < nodesPerCell; ++___2707) { ___2716 ___2714 = nodeToElemData->___2721.value( nodeToElemData->section, ___447 * nodesPerCell + ___2707); ___463 ind = nodeToElemData->elemIndex[___2714] + nodeToElemData->count[___2714]; ++nodeToElemData->count[___2714]; ___476(nodeToElemData->elem[ind] == 0); nodeToElemData->elem[ind] = nodeToElemData->baseCell + ___447; } } } }
 #endif
___2741::___2741(tecplot::tecioszl::___2728 const& ___2721, ___2716 nodeCount) : m_nodeCount(nodeCount) { size_t indexSize = static_cast<size_t>(nodeCount + 1); m_elemIndex.reset(new ___463[indexSize]); memset(m_elemIndex.get(), 0, indexSize * sizeof(m_elemIndex[0])); size_t arraySize = 0; for (int32_t section = 0; section < checked_numeric_cast<int32_t>(___2721.m_numCellsPerSection.size()); ++section) arraySize += static_cast<size_t>(___2721.m_numCellsPerSection[section] * ___2721.m_numNodesPerCellPerSection[section]); m_elem.reset(new ___463[arraySize]); memset(m_elem.get(), 0, arraySize * sizeof(m_elem[0])); boost::scoped_array<int> count(new int[m_nodeCount]); memset(count.get(), 0, m_nodeCount * sizeof(count[0])); for (int32_t section = 0; section < checked_numeric_cast<int32_t>(___2721.m_numCellsPerSection.size()); ++section) { size_t ___2819 = static_cast<size_t>( ___2721.m_numCellsPerSection[section] * ___2721.m_numNodesPerCellPerSection[section]); for(size_t i = 0; i < ___2819; ++i) { int64_t ___2707 = ___2721.value(section, i); ___476(0 <= ___2707 && ___2707 < m_nodeCount); ++count[___2707]; } } m_elemIndex[0] = 0; for(___2716 ___2707 = 0; ___2707 < m_nodeCount; ++___2707) { m_elemIndex[___2707 + 1] = m_elemIndex[___2707] + count[___2707]; count[___2707] = 0; } ___463 baseCell = 0; for (int32_t section = 0; section < checked_numeric_cast<int32_t>(___2721.m_numCellsPerSection.size()); ++section) { int64_t const ___2779 = ___2721.m_numCellsPerSection[section];
 #if !defined TECIOMPI
int numThreads = (___2779 >= MIN_CELLS_FOR_MULTITHREAD) ? ___2120::___2825() : 1; if (numThreads == 1) {
 #endif
for (___463 ___447 = 0; ___447 < ___2779; ++___447) { int32_t nodesPerCell = ___2721.m_numNodesPerCellPerSection[section]; for (int32_t ___2707 = 0; ___2707 < nodesPerCell; ++___2707) { ___2716 ___2714 = ___2721.value(section, ___447 * nodesPerCell + ___2707); ___463 ind = m_elemIndex[___2714] + count[___2714]; ++count[___2714]; ___476(m_elem[ind] == 0); m_elem[ind] = baseCell + ___447; } }
 #if !defined TECIOMPI
} else { boost::scoped_array<boost::atomic<int> > atomiccount(new boost::atomic<int>[m_nodeCount]); for(___2716 i = 0; i < m_nodeCount; ++i) atomiccount[i] = 0; std::vector<boost::shared_ptr<NodeToElemData> > nodeToElemData; for(int i = 0; i < numThreads; ++i) { ___463 begin = static_cast<___463>((size_t)___2721.m_numCellsPerSection[section] * i / numThreads); ___463 end = static_cast<___463>((size_t)___2721.m_numCellsPerSection[section] * (i + 1) / numThreads); nodeToElemData.push_back(boost::make_shared<NodeToElemData>(boost::cref(___2721), boost::cref(m_elemIndex), boost::ref(m_elem), boost::ref(atomiccount), section, baseCell, begin, end)); } ___2120 ___2117; for(int i = 0; i < numThreads; ++i) ___2117.addJob(fillElemArrayForCellRange, reinterpret_cast<___90>(nodeToElemData[i].get())); ___2117.wait(); }
 #endif
baseCell += ___2779; } } tecplot::___3931::___463 ___2741::cellCountForNode(tecplot::___3931::___2716 ___2707) { REQUIRE(0 <= ___2707 && ___2707 < m_nodeCount); return m_elemIndex[___2707 + 1] - m_elemIndex[___2707]; } }}
