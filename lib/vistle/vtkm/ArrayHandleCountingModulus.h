//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_cont_ArrayHandleCountingModulus_h
#define viskores_cont_ArrayHandleCountingModulus_h

#include <viskores/cont/ArrayHandleImplicit.h>

#include <viskores/cont/internal/ArrayRangeComputeUtils.h>

#include <viskores/Range.h>
#include <viskores/TypeTraits.h>
#include <viskores/VecFlat.h>
#include <viskores/VecTraits.h>

namespace viskores {
namespace cont {

struct VISKORES_ALWAYS_EXPORT StorageTagCountingModulus {};

namespace internal {

/// \brief An implicit array portal that returns a counting value.
template<class CountingValueType>
class VISKORES_ALWAYS_EXPORT ArrayPortalCountingModulus {
    using ComponentType = typename viskores::VecTraits<CountingValueType>::ComponentType;

public:
    using ValueType = CountingValueType;

    VISKORES_EXEC_CONT
    ArrayPortalCountingModulus(): Start(0), Step(1), NumberOfValues(0), Modulus(0), Repetition(1) {}

    /// @brief Create an implicit counting array.
    ///
    /// @param start The starting value in the first value of the array.
    /// @param step The increment between sequential values in the array.
    /// @param numValues The size of the array.
    /// @param modulus After how many repetition cycles the counting value wraps around.
    /// @param repetition How often a value is repeated before incrementing.
    VISKORES_EXEC_CONT
    ArrayPortalCountingModulus(ValueType start, ValueType step, viskores::Id numValues, viskores::Id modulus,
                               viskores::Id repetition)
    : Start(start), Step(step), NumberOfValues(numValues), Modulus(modulus), Repetition(repetition)
    {}

    /// Returns the starting value.
    VISKORES_EXEC_CONT
    ValueType GetStart() const { return this->Start; }

    /// Returns the step value.
    VISKORES_EXEC_CONT
    ValueType GetStep() const { return this->Step; }

    /// Returns the number of values in the array.
    VISKORES_EXEC_CONT
    viskores::Id GetNumberOfValues() const { return this->NumberOfValues; }

    /// Returns after how many repetition cycles the counting value wraps around.
    VISKORES_EXEC_CONT
    viskores::Id GetModulus() const { return this->Modulus; }

    /// Returns how often a value is repeated before incrementing.
    VISKORES_EXEC_CONT
    viskores::Id GetRepetition() const { return this->Repetition; }

    /// Returns the value for the given index.
    VISKORES_EXEC_CONT
    ValueType Get(viskores::Id index) const
    {
        viskores::Id i = this->Repetition > 1 ? index / this->Repetition : index;
        if (this->Modulus == 0)
            return ValueType(this->Start + this->Step * ValueType(static_cast<ComponentType>(i)));
        return ValueType(this->Start + this->Step * ValueType(static_cast<ComponentType>(i % this->Modulus)));
    }

private:
    ValueType Start;
    ValueType Step;
    viskores::Id NumberOfValues;
    viskores::Id Modulus;
    viskores::Id Repetition;
};

namespace detail {

template<typename T>
struct CanCountModImpl {
    using VTraits = viskores::VecTraits<T>;
    using BaseType = typename VTraits::BaseComponentType;
    using TTraits = viskores::TypeTraits<BaseType>;
    static constexpr bool IsNumeric =
        !std::is_same<typename TTraits::NumericTag, viskores::TypeTraitsUnknownTag>::value;
    static constexpr bool IsBool = std::is_same<BaseType, bool>::value;

    static constexpr bool value = IsNumeric && !IsBool;
};

} // namespace detail

// Not all types can be counted.
template<typename T>
struct CanCountMod {
    static constexpr bool value = detail::CanCountModImpl<T>::value;
};

template<typename T>
using StorageTagCountingModulusSuperclass = viskores::cont::StorageTagImplicit<internal::ArrayPortalCountingModulus<T>>;

template<typename T>
struct Storage<T, typename std::enable_if<CanCountMod<T>::value, viskores::cont::StorageTagCountingModulus>::type>
: Storage<T, StorageTagCountingModulusSuperclass<T>> {};

} // namespace internal

/// ArrayHandleCountingModulus is a specialization of ArrayHandle. By default it
/// contains an increment value, that is incremented after repetition steps between zero
/// and the passed in length. The increment wraps around when it reaches modulus.
template<typename CountingValueType>
class ArrayHandleCountingModulus
: public viskores::cont::ArrayHandle<CountingValueType, viskores::cont::StorageTagCountingModulus> {
public:
    VISKORES_ARRAY_HANDLE_SUBCLASS(ArrayHandleCountingModulus, (ArrayHandleCountingModulus<CountingValueType>),
                                   (viskores::cont::ArrayHandle<CountingValueType, StorageTagCountingModulus>));

    VISKORES_CONT
    ArrayHandleCountingModulus(CountingValueType start, CountingValueType step, viskores::Id length,
                               viskores::Id modulus, viskores::Id repetition = 1)
    : Superclass(internal::PortalToArrayHandleImplicitBuffers(
          internal::ArrayPortalCountingModulus<CountingValueType>(start, step, length, modulus, repetition)))
    {}

    VISKORES_CONT CountingValueType GetStart() const { return this->ReadPortal().GetStart(); }

    VISKORES_CONT CountingValueType GetStep() const { return this->ReadPortal().GetStep(); }

    VISKORES_CONT viskores::Id GetModulus() const { return this->ReadPortal().GetModulus(); }

    VISKORES_CONT viskores::Id GetRepetition() const { return this->ReadPortal().GetRepetition(); }
};

/// A convenience function for creating an ArrayHandleCountingModulus. It takes the
/// value to start counting from and and the number of times to increment.
template<typename CountingValueType>
VISKORES_CONT viskores::cont::ArrayHandleCountingModulus<CountingValueType>
make_ArrayHandleCountingModulus(CountingValueType start, CountingValueType step, viskores::Id length,
                                viskores::Id modulus, viskores::Id repetition = 1)
{
    return viskores::cont::ArrayHandleCountingModulus<CountingValueType>(start, step, length, modulus, repetition);
}

namespace internal {

template<typename S>
struct ArrayRangeComputeImpl;

template<>
struct VISKORES_CONT_EXPORT ArrayRangeComputeImpl<viskores::cont::StorageTagCountingModulus> {
    template<typename T>
    VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range>
    operator()(const viskores::cont::ArrayHandle<T, viskores::cont::StorageTagCountingModulus> &input,
               const viskores::cont::ArrayHandle<viskores::UInt8> &maskArray,
               bool viskoresNotUsed(computeFiniteRange), // assume array produces only finite values
               viskores::cont::DeviceAdapterId device) const
    {
        using Traits = viskores::VecTraits<viskores::VecFlat<T>>;
        viskores::cont::ArrayHandle<viskores::Range> result;
        result.Allocate(Traits::NUM_COMPONENTS);

        if (input.GetNumberOfValues() <= 0) {
            result.Fill(viskores::Range{});
            return result;
        }

        viskores::Id2 firstAndLast{0, input.GetNumberOfValues() - 1};
        if (maskArray.GetNumberOfValues() > 0) {
            firstAndLast = GetFirstAndLastUnmaskedIndices(maskArray, device);
        }

        if (firstAndLast[1] < firstAndLast[0]) {
            result.Fill(viskores::Range{});
            return result;
        }

        auto portal = result.WritePortal();
        // assume the values to be finite
        auto first = make_VecFlat(input.ReadPortal().Get(firstAndLast[0]));
        auto last = make_VecFlat(input.ReadPortal().Get(firstAndLast[1]));
        for (viskores::IdComponent cIndex = 0; cIndex < Traits::NUM_COMPONENTS; ++cIndex) {
            auto firstComponent = Traits::GetComponent(first, cIndex);
            auto lastComponent = Traits::GetComponent(last, cIndex);
            portal.Set(cIndex, viskores::Range(viskores::Min(firstComponent, lastComponent),
                                               viskores::Max(firstComponent, lastComponent)));
        }

        return result;
    }
};

} // namespace internal

} // namespace cont
} // namespace viskores

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace viskores {
namespace cont {

template<typename T>
struct SerializableTypeString<viskores::cont::ArrayHandleCountingModulus<T>> {
    static VISKORES_CONT const std::string &Get()
    {
        static std::string name = "AH_CountingModulus<" + SerializableTypeString<T>::Get() + ">";
        return name;
    }
};

template<typename T>
struct SerializableTypeString<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagCountingModulus>>
: SerializableTypeString<viskores::cont::ArrayHandleCountingModulus<T>> {};
} // namespace cont
} // namespace viskores

namespace mangled_diy_namespace {

template<typename T>
struct Serialization<viskores::cont::ArrayHandleCountingModulus<T>> {
private:
    using Type = viskores::cont::ArrayHandleCountingModulus<T>;
    using BaseType = viskores::cont::ArrayHandle<typename Type::ValueType, typename Type::StorageTag>;

public:
    static VISKORES_CONT void save(BinaryBuffer &bb, const BaseType &obj)
    {
        auto portal = obj.ReadPortal();
        viskoresdiy::save(bb, portal.GetStart());
        viskoresdiy::save(bb, portal.GetStep());
        viskoresdiy::save(bb, portal.GetNumberOfValues());
        viskoresdiy::save(bb, portal.GetModulus());
        viskoresdiy::save(bb, portal.GetRepetition());
    }

    static VISKORES_CONT void load(BinaryBuffer &bb, BaseType &obj)
    {
        T start{}, step{};
        viskores::Id count = 0, modulus = 1, repetition = 0;

        viskoresdiy::load(bb, start);
        viskoresdiy::load(bb, step);
        viskoresdiy::load(bb, count);
        viskoresdiy::load(bb, modulus);
        viskoresdiy::load(bb, repetition);

        obj = viskores::cont::make_ArrayHandleCountingModulus(start, step, count, modulus, repetition);
    }
};

template<typename T>
struct Serialization<viskores::cont::ArrayHandle<T, viskores::cont::StorageTagCountingModulus>>
: Serialization<viskores::cont::ArrayHandleCountingModulus<T>> {};
} // namespace mangled_diy_namespace
/// @endcond SERIALIZATION

#endif //viskores_cont_ArrayHandleCountingModulus_h
