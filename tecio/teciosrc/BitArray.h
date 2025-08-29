 #pragma once
#include "ThirdPartyHeadersBegin.h"
 #if __cplusplus > 199711L
#include <algorithm>
#include <cmath>
#include <memory>
#include <type_traits>
 #else
#include <math.h>
#include <stdlib.h>
 #endif
#include "ThirdPartyHeadersEnd.h"
#include "CodeContract.h"
namespace tecplot { template <typename ARRAY_ITEM_TYPE, size_t ITEM_BIT_SIZE> class BitArray { public: static size_t byteArraySize(size_t size) { return size > 0 ? ITEM_BIT_SIZE*(size-1ul)/8ul + 1ul : 0ul; } static size_t itemBitSize() { return ITEM_BIT_SIZE; } struct ItemMutator { BitArray&    owner; size_t const ___2863; ItemMutator(BitArray& owner, size_t ___2863) : owner(owner), ___2863(___2863) { } ARRAY_ITEM_TYPE operator=(ARRAY_ITEM_TYPE value) { return owner.set(___2863, value); } operator ARRAY_ITEM_TYPE() const { return owner.get(___2863); } }; BitArray(size_t size) : m_size(size) , m_byteArraySize(byteArraySize(m_size)) , m_byteArray(makeByteArray(m_byteArraySize)) , m_isClientByteArray(false) {
 #if __cplusplus > 199711L
static_assert(sizeof(ARRAY_ITEM_TYPE)*8 >= ITEM_BIT_SIZE, "ARRAY_ITEM_TYPE bit size must be greater or equal to ITEM_BIT_SIZE"); static_assert(std::is_unsigned<ARRAY_ITEM_TYPE>::value, "ARRAY_ITEM_TYPE must be an unsigned integral type"); static_assert(ITEM_BIT_SIZE == 2 || ITEM_BIT_SIZE == 4 || ITEM_BIT_SIZE == 8, "ITEM_BIT_SIZE must be 2, 4, or 8");
 #else
REQUIRE(ITEM_BIT_SIZE == 2 || ITEM_BIT_SIZE == 4 || ITEM_BIT_SIZE == 8);
 #endif
if (m_byteArraySize > 0) m_byteArray[m_byteArraySize-1] = 0; } BitArray( size_t          size, ARRAY_ITEM_TYPE initValue) : m_size(size) , m_byteArraySize(byteArraySize(m_size)) , m_byteArray(makeByteArray(m_byteArraySize)) , m_isClientByteArray(false) {
 #if __cplusplus > 199711L
static_assert(sizeof(ARRAY_ITEM_TYPE)*8 >= ITEM_BIT_SIZE, "ARRAY_ITEM_TYPE bit size must be greater or equal to ITEM_BIT_SIZE"); static_assert(std::is_unsigned<ARRAY_ITEM_TYPE>::value, "ARRAY_ITEM_TYPE must be an unsigned integral type"); static_assert(ITEM_BIT_SIZE == 2 || ITEM_BIT_SIZE == 4 || ITEM_BIT_SIZE == 8, "ITEM_BIT_SIZE must be 2, 4, or 8");
 #else
REQUIRE(ITEM_BIT_SIZE == 2 || ITEM_BIT_SIZE == 4 || ITEM_BIT_SIZE == 8);
 #endif
REQUIRE(initValue < static_cast<ARRAY_ITEM_TYPE>(1ul<<ITEM_BIT_SIZE)); if (m_byteArraySize > 0) m_byteArray[m_byteArraySize-1] = 0; for (size_t ___2863 = 0; ___2863 < m_size; ++___2863) (*this)[___2863] = initValue; } BitArray( uint8_t* ___416, size_t   size) : m_size(size) , m_byteArraySize(byteArraySize(m_size)) , m_byteArray(___416) , m_isClientByteArray(true) {
 #if __cplusplus > 199711L
static_assert(sizeof(ARRAY_ITEM_TYPE)*8 >= ITEM_BIT_SIZE, "ARRAY_ITEM_TYPE bit size must be greater or equal to ITEM_BIT_SIZE"); static_assert(std::is_unsigned<ARRAY_ITEM_TYPE>::value, "ARRAY_ITEM_TYPE must be an unsigned integral type"); static_assert(ITEM_BIT_SIZE == 2 || ITEM_BIT_SIZE == 4 || ITEM_BIT_SIZE == 8, "ITEM_BIT_SIZE must be 2, 4, or 8");
 #else
REQUIRE(ITEM_BIT_SIZE == 2 || ITEM_BIT_SIZE == 4 || ITEM_BIT_SIZE == 8); REQUIRE(EQUIVALENCE(___416 != NULL, size > 0));
 #endif
if (m_byteArraySize > 0) m_byteArray[m_byteArraySize-1] = 0; } BitArray( uint8_t*        ___416, size_t          size, ARRAY_ITEM_TYPE initValue) : m_size(size) , m_byteArraySize(byteArraySize(m_size)) , m_byteArray(___416) , m_isClientByteArray(true) {
 #if __cplusplus > 199711L
static_assert(sizeof(ARRAY_ITEM_TYPE)*8 >= ITEM_BIT_SIZE, "ARRAY_ITEM_TYPE bit size must be greater or equal to ITEM_BIT_SIZE"); static_assert(std::is_unsigned<ARRAY_ITEM_TYPE>::value, "ARRAY_ITEM_TYPE must be an unsigned integral type"); static_assert(ITEM_BIT_SIZE == 2 || ITEM_BIT_SIZE == 4 || ITEM_BIT_SIZE == 8, "ITEM_BIT_SIZE must be 2, 4, or 8");
 #else
REQUIRE(ITEM_BIT_SIZE == 2 || ITEM_BIT_SIZE == 4 || ITEM_BIT_SIZE == 8); REQUIRE(EQUIVALENCE(___416 != NULL, size > 0));
 #endif
REQUIRE(initValue < static_cast<ARRAY_ITEM_TYPE>(1ul<<ITEM_BIT_SIZE)); if (m_byteArraySize > 0) m_byteArray[m_byteArraySize-1] = 0; for (size_t ___2863 = 0; ___2863 < m_size; ++___2863) (*this)[___2863] = initValue; } BitArray(BitArray const& ___2886) : m_size(___2886.m_size) , m_byteArraySize(___2886.m_byteArraySize) , m_byteArray(makeByteArray(m_byteArraySize)) , m_isClientByteArray(false) { std::copy(___2886.m_byteArray, ___2886.m_byteArray+___2886.m_byteArraySize, m_byteArray); } BitArray& operator=(BitArray const& ___3390) { if (this != &___3390) { uint8_t* const newArray = makeByteArray(___3390.m_byteArraySize); std::copy(___3390.m_byteArray, ___3390.m_byteArray+___3390.m_byteArraySize, newArray); if (!m_isClientByteArray) delete[] m_byteArray; m_size = ___3390.m_size; m_byteArraySize = ___3390.m_byteArraySize; m_byteArray = newArray; m_isClientByteArray = false; } return *this; }
 #if __cplusplus > 199711L 
BitArray(BitArray&& ___2886) : m_size(std::move(___2886.m_size)) , m_byteArraySize(std::move(___2886.m_byteArraySize)) , m_byteArray(std::move(___2886.m_byteArray)) , m_isClientByteArray(std::move(___2886.m_isClientByteArray)) { ___2886.m_size = 0; ___2886.m_byteArraySize = 0; ___2886.m_byteArray = static_cast<uint8_t*>(NULL); ___2886.m_isClientByteArray = false; }
 #endif
 #if __cplusplus > 199711L 
BitArray&& operator=(BitArray&& ___3390) { if (this != &___3390) { if (!m_isClientByteArray) delete[] m_byteArray; m_size = std::move(___3390.m_size); m_byteArraySize = std::move(___3390.m_byteArraySize); m_byteArray = std::move(___3390.m_byteArraySize); m_isClientByteArray = std::move(___3390.m_isClientByteArray); ___3390.m_size = 0; ___3390.m_byteArraySize = 0; ___3390.m_byteArray = static_cast<uint8_t*>(NULL); ___3390.m_isClientByteArray = false; } return *this; }
 #endif
~BitArray() { if (!m_isClientByteArray) delete[] m_byteArray; } ItemMutator operator[](size_t ___2863) { REQUIRE(___2863 < m_size); return ItemMutator(*this, ___2863); } size_t size() const { return m_size; } size_t byteSize() const { return m_byteArraySize; } bool empty() const { REQUIRE(EQUIVALENCE(m_size == 0, m_byteArray == static_cast<uint8_t*>(NULL))); return m_size == 0; } uint8_t const* data() const { return reinterpret_cast<uint8_t const*>(m_byteArray); } uint8_t* data() { return reinterpret_cast<uint8_t*>(m_byteArray); } private: static uint8_t* makeByteArray(size_t byteArraySize) { return byteArraySize > 0 ? new uint8_t[byteArraySize] : static_cast<uint8_t*>(NULL); } std::pair<size_t ,size_t/*bitShiftCount*/> itemOffsets(size_t ___2863) const { size_t const bitOffset = ___2863 * ITEM_BIT_SIZE; size_t const byteArrayOffset = bitOffset / 8ul; size_t const bitShiftCount = ITEM_SHIFT_COUNT - bitOffset % 8ul; ENSURE(byteArrayOffset < byteSize()); ENSURE(bitShiftCount <= ITEM_SHIFT_COUNT); return std::make_pair(byteArrayOffset, bitShiftCount); } ARRAY_ITEM_TYPE get(size_t ___2863) const { REQUIRE(___2863 < m_size); std::pair<size_t,size_t> const offsets = itemOffsets(___2863); size_t const byteArrayOffset = offsets.first; size_t const bitShiftCount = offsets.second; return (m_byteArray[byteArrayOffset] >> bitShiftCount) & ITEM_MASK; } ARRAY_ITEM_TYPE set( size_t          ___2863, ARRAY_ITEM_TYPE value) { REQUIRE(___2863 < m_size); REQUIRE(value <= VALUE_MAX); std::pair<size_t,size_t> const offsets = itemOffsets(___2863); size_t const byteArrayOffset = offsets.first; size_t const bitShiftCount = offsets.second; value &= ITEM_MASK; m_byteArray[byteArrayOffset] &= ~(ITEM_MASK << bitShiftCount); m_byteArray[byteArrayOffset] |= value << bitShiftCount; return value; } static size_t const ITEM_SHIFT_COUNT = static_cast<size_t>(8) - ITEM_BIT_SIZE; static ARRAY_ITEM_TYPE const VALUE_MAX = static_cast<ARRAY_ITEM_TYPE>((static_cast<size_t>(1) << ITEM_BIT_SIZE) - 1); static size_t const ITEM_MASK = (~static_cast<size_t>(0)) >> (sizeof(size_t)*8ul - ITEM_BIT_SIZE); size_t   m_size; size_t   m_byteArraySize; uint8_t* m_byteArray; bool     m_isClientByteArray; }; }
