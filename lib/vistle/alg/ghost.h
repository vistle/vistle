#ifndef VISTLE_ALG_GHOST_H
#define VISTLE_ALG_GHOST_H

#include <algorithm>
#include <iterator>
#include <vector>

namespace vistle {

/**
  * @brief Adds ghost cells to given startvalue and current count of data based on given dimension.
  *
  * @start: start value for reading data.
  * @count: count of values for current block.
  * @dimData: dimension of data for corresponding start and count.
  * @numGhost: number of ghost cells to be added.
  */
template<typename NumType>
void structured_ghost_addition(NumType &start, NumType &count, const NumType &dimData, const NumType &numGhost)
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
  * @brief Generates partition for structured grid which contains block values for each dim partitioned for current blocknumber.
  *
  * @first: Begin pointer of block container.
  * @last: End pointer of block container.
  * @d_first: Begin pointer of container where results will be written to.
  * @blockNum: Current blocknumber.
  * @return: Begin iterator of container which contains block values specified for each direction and current blocknumber.
  */
template<class ForwardIt, class OutputBlockIt, class NumericType>
OutputBlockIt structured_block_partition(ForwardIt first, ForwardIt last, OutputBlockIt d_first,
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
  * @brief Generates a vector<T> for structured grid which contains block values for each dim partitioned for current blocknumber.
  *
  * @blockSize: Blocksize specified for each dim.
  * @bockNum: Current active blocknumber.
  * @return: Vector<T> with block values specified for each direction and current blocknumber.
  */
template<typename NumericType>
const std::vector<NumericType> structured_block_partition(const std::vector<NumericType> &blockSize,
                                                          const NumericType &blockNum)
{
    std::vector<NumericType> distBlocks(blockSize.size());
    blockPartitionStructured_tmpl(blockSize.begin(), blockSize.end(), distBlocks.begin(), blockNum);
    return distBlocks;
}

/**
  * @brief Generates a vector<T> for structured grid which contains block values for each dim partitioned for current blocknumber.
  *
  * @first: Begin pointer of block container.
  * @last: End pointer of block container.
  * @blockNum: Current blocknumber.
  * @return: Vector<T> with block values specified for each direction and current blocknumber.
  */
template<class ForwardIt, class NumericType>
const std::vector<NumericType> structured_block_partition(ForwardIt first, ForwardIt last, const NumericType &blockNum)
{
    std::vector<NumericType> distBlocks(std::distance(first, last));
    blockPartitionStructured_tmpl(first, last, distBlocks.begin(), blockNum);
    return distBlocks;
}

} // namespace vistle
#endif
