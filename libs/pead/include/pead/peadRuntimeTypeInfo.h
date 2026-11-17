/**
 * @file
 * @brief This is pead's own type identification, which stands in for C++ RTTI because the library builds without it. Classes that heaps, streams and print outputs hand around as base pointers use it to check their real type.
 */

#pragma once

namespace pead
{
    namespace RuntimeTypeInfo
    {
        /**
         * @brief Each class with runtime type info owns one static instance of this, and the instances link into a tree from derived to base.
         */
        class Interface
        {
        public:
            virtual bool isDerived(const Interface *typeInfo) const = 0;
        };

        class Root : public Interface
        {
        public:
            bool isDerived(const Interface *typeInfo) const override
            {
                return typeInfo == this;
            }
        };

        template<typename Super>
        class Derive : public Interface
        {
        public:
            bool isDerived(const Interface *typeInfo) const override
            {
                return typeInfo == this || Super::getRuntimeTypeInfoStatic()->isDerived(typeInfo);
            }
        };
    } // namespace RuntimeTypeInfo

    /**
     * @brief This tells whether obj is a T or derives from it.
     *
     * @param obj The object to test, which may be null.
     * @return True when obj is non-null and its dynamic type derives from T.
     */
    template<typename T, typename U>
    bool IsDerivedFrom(const U *obj)
    {
        return obj && obj->checkDerivedRuntimeTypeInfo(T::getRuntimeTypeInfoStatic());
    }

    template<typename T, typename U>
    T *DynamicCast(U *obj)
    {
        return IsDerivedFrom<T>(obj) ? (T *) obj : nullptr;
    }

} // namespace pead

// The root class of a hierarchy uses the first macro, and every class below it uses the second.
#define PEAD_RTTI_BASE(CLASS)                                                                                                                        \
public:                                                                                                                                              \
    static const ::pead::RuntimeTypeInfo::Interface *getRuntimeTypeInfoStatic()                                                                      \
    {                                                                                                                                                \
        static const ::pead::RuntimeTypeInfo::Root typeInfo;                                                                                         \
        return &typeInfo;                                                                                                                            \
    }                                                                                                                                                \
    virtual bool checkDerivedRuntimeTypeInfo(const ::pead::RuntimeTypeInfo::Interface *typeInfo) const                                               \
    {                                                                                                                                                \
        return getRuntimeTypeInfo()->isDerived(typeInfo);                                                                                            \
    }                                                                                                                                                \
    virtual const ::pead::RuntimeTypeInfo::Interface *getRuntimeTypeInfo() const                                                                     \
    {                                                                                                                                                \
        return getRuntimeTypeInfoStatic();                                                                                                           \
    }

#define PEAD_RTTI_OVERRIDE(CLASS, BASE)                                                                                                              \
public:                                                                                                                                              \
    static const ::pead::RuntimeTypeInfo::Interface *getRuntimeTypeInfoStatic()                                                                      \
    {                                                                                                                                                \
        static const ::pead::RuntimeTypeInfo::Derive<BASE> typeInfo;                                                                                 \
        return &typeInfo;                                                                                                                            \
    }                                                                                                                                                \
    bool checkDerivedRuntimeTypeInfo(const ::pead::RuntimeTypeInfo::Interface *typeInfo) const override                                              \
    {                                                                                                                                                \
        return getRuntimeTypeInfo()->isDerived(typeInfo);                                                                                            \
    }                                                                                                                                                \
    const ::pead::RuntimeTypeInfo::Interface *getRuntimeTypeInfo() const override                                                                    \
    {                                                                                                                                                \
        return getRuntimeTypeInfoStatic();                                                                                                           \
    }
