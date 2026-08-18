/***********************************************************************************************************************
**
** Library: cxxkit
**
** Copyright (C) 2026~Present ChengXueWen.
**
** License: MIT License
**
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all copies or substantial portions
** of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
** TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
** CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
** IN THE SOFTWARE.
**
***********************************************************************************************************************/

#pragma once

#include <cxxkit/tools/type_traits.hpp>

CXXKIT_BEGIN_NAMESPACE

template <typename T>
class TypeInfo
{
public:
    enum
    {
        isRelocatable = traits::is_relocatable<T>::value, // can copy/move
        isSpecialized = std::is_enum<T>::value,           // don't require every enum to be marked manually
        isIntegral = std::is_integral<T>::value,
        isComplex = !std::is_trivial<T>::value, // not pod type
        isPointer = false,
        isStatic = true
    };
};

template <>
class TypeInfo<void>
{
public:
    enum
    {
        isRelocatable = false,
        isSpecialized = true,
        isIntegral = false,
        isComplex = false,
        isPointer = false,
        isStatic = false,
    };
};

template <typename T>
class TypeInfo<T *>
{
public:
    enum
    {
        isRelocatable = true,
        isSpecialized = true,
        isIntegral = false,
        isComplex = false,
        isPointer = true,
        isStatic = false
    };
};

// apply defaults for a generic TypeInfo<T> that didn't provide the new values
/**
 * TypeInfoQuery is used to query the values of a given TypeInfo<T>
 *
 * We use it because there may be some TypeInfo<T> specializations in user code that don't provide certain flags
 * that we added after Qt 5.0. They are:
 * @list
 *  @li isRelocatable: defaults to !isStatic
 * @endlist
 *
 * DO NOT specialize this class elsewhere.
 * @tparam T
 */
template <typename T, typename = void>
struct TypeInfoQuery : public TypeInfo<T>
{
    enum
    {
        isRelocatable = !TypeInfo<T>::isStatic
    };
};

// if TypeInfo<T>::isRelocatable exists, use it
template <typename T>
struct TypeInfoQuery<T, typename std::enable_if<TypeInfo<T>::isRelocatable || true>::type> : public TypeInfo<T>
{
};

/**
 * TypeInfoMerger merges the TypeInfo flags of T1, T2... and presents them as a TypeInfo<T> would do.
 *
 * Let's assume that we have a simple set of structs:
 *  To create a proper TypeInfo specialization for A struct, we have to check all sub-components;
 *  B, C and D, then take the lowest common denominator and call CXXKIT_DECLARE_TYPEINFO with the resulting flags.
 *  An easier and less fragile approach is to use TypeInfoMerger, which does that automatically.
 *  So struct A would have the following TypeInfo definition:
 */
template <class T, class T1, class T2 = T1, class T3 = T1, class T4 = T1>
class TypeInfoMerger
{
public:
    enum
    {
        isRelocatable = TypeInfoQuery<T1>::isRelocatable && TypeInfoQuery<T2>::isRelocatable &&
                        TypeInfoQuery<T3>::isRelocatable && TypeInfoQuery<T4>::isRelocatable,
        isSpecialized = true,
        isComplex = TypeInfoQuery<T1>::isComplex || TypeInfoQuery<T2>::isComplex || TypeInfoQuery<T3>::isComplex ||
                    TypeInfoQuery<T4>::isComplex,
        isStatic = TypeInfoQuery<T1>::isStatic || TypeInfoQuery<T2>::isStatic || TypeInfoQuery<T3>::isStatic ||
                   TypeInfoQuery<T4>::isStatic,
        isPointer = false,
        isIntegral = false
    };
};

/**
 * Specialize a specific type with:
 *      CXXKIT_DECLARE_TYPEINFO(type, flags);
 * where 'type' is the name of the type to specialize and 'flags' is logically-OR'ed combination of the flags below.
 */
struct TypeInfoFlags
{
    CXXKIT_STATIC_CONSTANT_NUMBER(kComplexType, 0x00);
    CXXKIT_STATIC_CONSTANT_NUMBER(kMovableType, 0x02);
    CXXKIT_STATIC_CONSTANT_NUMBER(kPrimitiveType, 0x01);
    CXXKIT_STATIC_CONSTANT_NUMBER(kRelocatableType, 0x2);
};
#define CXXKIT_COMPLEX_TYPE     cxxkit::TypeInfoFlags::kComplexType
#define CXXKIT_MOVABLE_TYPE     cxxkit::TypeInfoFlags::kMovableType
#define CXXKIT_PRIMITIVE_TYPE   cxxkit::TypeInfoFlags::kPrimitiveType
#define CXXKIT_RELOCATABLE_TYPE cxxkit::TypeInfoFlags::kRelocatableType

#define CXXKIT_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)                                                                        \
    class cxxkit::TypeInfo<TYPE>                                                                                         \
    {                                                                                                                  \
    public:                                                                                                            \
        CXXKIT_WARNING_PUSH                                                                                              \
        CXXKIT_WARNING_DISABLE_CLANG("-Wdeprecated-anon-enum-enum-conversion")                                           \
        enum                                                                                                           \
        {                                                                                                              \
            isSpecialized = true,                                                                                      \
            isComplex = (((FLAGS) & CXXKIT_PRIMITIVE_TYPE) == 0) && !std::is_trivial<TYPE>::value,                       \
            isStatic = (((FLAGS) & (CXXKIT_MOVABLE_TYPE | CXXKIT_PRIMITIVE_TYPE)) == 0),                                   \
            isRelocatable = !isStatic || ((FLAGS) & CXXKIT_RELOCATABLE_TYPE) || cxxkit::traits::is_relocatable_v<TYPE>,    \
            isPointer = false,                                                                                         \
            isIntegral = std::is_integral<TYPE>::value,                                                                \
        };                                                                                                             \
        CXXKIT_WARNING_POP                                                                                               \
        static inline const char *name()                                                                               \
        {                                                                                                              \
            return #TYPE;                                                                                              \
        }                                                                                                              \
    }

#define CXXKIT_DECLARE_TYPEINFO(TYPE, FLAGS)                                                                             \
    template <>                                                                                                        \
    CXXKIT_DECLARE_TYPEINFO_BODY(TYPE, FLAGS)

CXXKIT_END_NAMESPACE

/*
   TypeInfo primitive specializations
*/
CXXKIT_DECLARE_TYPEINFO(bool, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(char, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(signed char, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(unsigned char, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(short, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(unsigned short, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(int, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(unsigned int, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(long, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(unsigned long, CXXKIT_PRIMITIVE_TYPE);
//CXXKIT_DECLARE_TYPEINFO(int64_t, CXXKIT_PRIMITIVE_TYPE);
//CXXKIT_DECLARE_TYPEINFO(uint64_t, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(float, CXXKIT_PRIMITIVE_TYPE);
CXXKIT_DECLARE_TYPEINFO(double, CXXKIT_PRIMITIVE_TYPE);

