#ifndef _READTSUNAMI_IMPL_H
#define _READTSUNAMI_IMPL_H
#include <algorithm>
#include <iterator>
#include <vector>

/**
  * Adds ghost cells to given startvalue and current count of data based on given dimension.
  *
  * @start: start value for reading data.
  * @count: count of values for current block.
  * @dimData: dimension of data for corresponding start and count.
  * @numGhost: number of ghost cells to be added.
  */
template<typename ForwardIt>
void addGhostStructured_tmpl(ForwardIt &start, ForwardIt &count, const ForwardIt &dimData, const ForwardIt &numGhost)
{
    auto partitionSize = start + count;
    if (start == 0) {
        if (partitionSize < dimData)
            count += numGhost;
    } else if (partitionSize == dimData)
        start -= numGhost;
    else {
        start -= numGhost;
        count += 2 * numGhost;
    }
}

/**
  * Adds ghost cells to given startvalue and current count of data based on given dimension.
  *
  * @start: Begin iterator.
  * @count: End iterator.
  * @dimData: dimension of whole dataset (e.g. latitude = 773).
  * @numGhost: number of ghost cells to be added.
  */
/* template<class InputIt, class NumericType> */
/* void addGhostStructured_tmpl(InputIt &start, InputIt &end, const NumericType &dimData, const NumericType &numGhost) */
/* { */
/*     auto size = std::distance(start, end); */
/*     if (size == dimData) { */
/*         start -= numGhost; */
/*         end += numGhost; */
/*     } else if (size > 0) { */
/*         start -= numGhost; */
/*         end += 2 * numGhost; */
/*     } else */
/*         end += numGhost; */
/* } */

/**
  * Generates a vector<T> for structured data which contains block values for each dim partitioned for current blocknumber.
  *
  * @first: Begin pointer of block container.
  * @last: End pointer of block container.
  * @d_first: Begin pointer of container that results will be written to.
  * @blockNum: Current blocknumber.
  * @return: Begin iterator of container which contains block values specified for each direction and current blocknumber.
  */
template<class ForwardIt, class OutputBlockIt, class NumericType>
OutputBlockIt blockPartitionStructured_tmpl(ForwardIt first, ForwardIt last, OutputBlockIt d_first,
                                            const NumericType &blockNum)
{
    const auto numBlocks{std::distance(first, last)};
    std::transform(first, last, d_first, [it = 0, &numBlocks, b = blockNum](const NumericType &bS) mutable {
        if (it++ == numBlocks - 1)
            return b;
        else {
            NumericType bDist = b % bS;
            b /= bS;
            return bDist;
        };
    });
    return d_first;
}

/**
  * Generates a vector<T> for structured data which contains block values for each dim partitioned for current blocknumber.
  *
  * @blockSize: Blocksize specified for each dim.
  * @bockNum: Current active blocknumber.
  * @return: Vector<T> with block values specified for each direction and current blocknumber.
  */
template<typename NumericType>
const std::vector<NumericType> blockPartitionStructured_tmpl(const std::vector<NumericType> &blockSize,
                                                             const NumericType &blockNum)
{
    std::vector<NumericType> distBlocks(blockSize.size());
    blockPartitionStructured_tmpl(blockSize.begin(), blockSize.end(), distBlocks.begin(), blockNum);
    return distBlocks;
}


/**
  * Generates a vector<T> for structured data which contains block values for each dim partitioned for current blocknumber.
  *
  * @first: Begin pointer of block container.
  * @last: End pointer of block container.
  * @blockNum: Current blocknumber.
  * @return: Vector<T> with block values specified for each direction and current blocknumber.
  */
template<class ForwardIt, class NumericType>
const std::vector<NumericType> blockPartitionStructured_tmpl(ForwardIt first, ForwardIt last,
                                                             const NumericType &blockNum)
{
    std::vector<NumericType> distBlocks(std::distance(first, last));
    blockPartitionStructured_tmpl(first, last, distBlocks.begin(), blockNum);
    return distBlocks;
}

#endif
