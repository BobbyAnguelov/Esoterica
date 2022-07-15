#pragma once
#include "TypeID.h"
#include "System/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UUID;
    class Tag;
    struct Color;
    struct Float2;
    struct Float3;
    struct Float4;
    class Vector;
    class Quaternion;
    class Transform;
    class Matrix;
    class Quaternion;
    class Microseconds;
    class Milliseconds;
    class Seconds;
    class Percentage;
    struct Degrees;
    struct Radians;
    struct EulerAngles;

    struct IntRange;
    struct FloatRange;
    class FloatCurve;
    class BitFlags;

    class ResourceID;
    class ResourceTypeID;
    class ResourcePath;

    template<typename T>
    class TBitFlags;

    namespace Resource
    {
        class ResourcePtr;
    }

    template<typename T>
    class TResourcePtr;
}

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    enum class CoreTypeID
    {
        Invalid = -1,

        Bool = 0,
        Uint8,
        Int8,
        Uint16,
        Int16,
        Uint32,
        Int32,
        Uint64,
        Int64,
        Float,
        Double,
        UUID,
        StringID,
        Tag,
        TypeID,
        String,
        Color,
        Float2,
        Float3,
        Float4,
        Vector,
        Quaternion,
        Matrix,
        Transform,
        Microseconds,
        Milliseconds,
        Seconds,
        Percentage,
        Degrees,
        Radians,
        EulerAngles,
        IntRange,
        FloatRange,
        FloatCurve,

        BitFlags,
        TBitFlags,

        TVector,

        ResourcePath,
        ResourceTypeID,
        ResourceID,
        ResourcePtr,
        TResourcePtr,

        NumTypes,
    };

    //-------------------------------------------------------------------------
    // Core Type Registry
    //-------------------------------------------------------------------------

    class EE_SYSTEM_API CoreTypeRegistry
    {
        struct CoreTypeRecord
        {
            inline bool operator==( TypeID ID ) const { return m_ID == ID; }

            TypeID          m_ID;
            size_t          m_typeSize;
            size_t          m_typeAlignment;

            #if EE_DEVELOPMENT_TOOLS
            char            m_friendlyName[30];
            #endif
        };

    private:

        static CoreTypeRecord               s_coreTypeRecords[(uint8_t) CoreTypeID::NumTypes];
        static bool                         s_areCoreTypeRecordsInitialized;

    public:

        static void Initialize();
        static void Shutdown();

        static bool IsCoreType( TypeID typeID );
        static CoreTypeID GetType( TypeID typeID );
        static size_t GetTypeSize( TypeID typeID );
        static size_t GetTypeAlignment( TypeID typeID );

        EE_FORCE_INLINE static TypeID GetTypeID( CoreTypeID coreType )
        {
            return s_coreTypeRecords[(uint8_t) coreType].m_ID;
        }

        EE_FORCE_INLINE static size_t GetTypeSize( CoreTypeID coreType )
        {
            return s_coreTypeRecords[(uint8_t) coreType].m_typeSize;
        }

        EE_FORCE_INLINE static size_t GetTypeAlignment( CoreTypeID coreType )
        {
            return s_coreTypeRecords[(uint8_t) coreType].m_typeAlignment;
        }

        #if EE_DEVELOPMENT_TOOLS
        EE_FORCE_INLINE static char const* GetFriendlyName( CoreTypeID coreType )
        {
            return s_coreTypeRecords[(uint8_t) coreType].m_friendlyName;
        }
        #endif
    };

    //-------------------------------------------------------------------------
    // Query
    //-------------------------------------------------------------------------

    EE_FORCE_INLINE TypeID GetCoreTypeID( CoreTypeID coreType ) { return CoreTypeRegistry::GetTypeID( coreType ); }
    EE_FORCE_INLINE CoreTypeID GetCoreType( TypeID typeID ) { return CoreTypeRegistry::GetType( typeID ); }
    EE_FORCE_INLINE bool IsCoreType( TypeID typeID ){ return CoreTypeRegistry::IsCoreType( typeID ); }

    //-------------------------------------------------------------------------

    template<typename T> inline TypeID GetCoreTypeID() { return TypeID(); }
    template<template<typename> typename C> inline TypeID GetCoreTypeID() { return TypeID(); }

    template<> inline TypeID GetCoreTypeID<bool>() { return GetCoreTypeID( CoreTypeID::Bool ); }
    template<> inline TypeID GetCoreTypeID<int8_t>() { return GetCoreTypeID( CoreTypeID::Int8 ); }
    template<> inline TypeID GetCoreTypeID<int16_t>() { return GetCoreTypeID( CoreTypeID::Int16 ); }
    template<> inline TypeID GetCoreTypeID<int32_t>() { return GetCoreTypeID( CoreTypeID::Int32 ); }
    template<> inline TypeID GetCoreTypeID<int64_t>() { return GetCoreTypeID( CoreTypeID::Int64 ); }
    template<> inline TypeID GetCoreTypeID<uint8_t>() { return GetCoreTypeID( CoreTypeID::Uint8 ); }
    template<> inline TypeID GetCoreTypeID<uint16_t>() { return GetCoreTypeID( CoreTypeID::Uint16 ); }
    template<> inline TypeID GetCoreTypeID<uint32_t>() { return GetCoreTypeID( CoreTypeID::Uint32 ); }
    template<> inline TypeID GetCoreTypeID<uint64_t>() { return GetCoreTypeID( CoreTypeID::Uint64 ); }
    template<> inline TypeID GetCoreTypeID<float>() { return GetCoreTypeID( CoreTypeID::Float ); }
    template<> inline TypeID GetCoreTypeID<double>() { return GetCoreTypeID( CoreTypeID::Double ); }
    template<> inline TypeID GetCoreTypeID<UUID>() { return GetCoreTypeID( CoreTypeID::UUID ); }
    template<> inline TypeID GetCoreTypeID<StringID>() { return GetCoreTypeID( CoreTypeID::StringID ); }
    template<> inline TypeID GetCoreTypeID<Tag>() { return GetCoreTypeID( CoreTypeID::Tag ); }
    template<> inline TypeID GetCoreTypeID<TypeID>() { return GetCoreTypeID( CoreTypeID::TypeID ); }
    template<> inline TypeID GetCoreTypeID<String>() { return GetCoreTypeID( CoreTypeID::String ); }
    template<> inline TypeID GetCoreTypeID<Color>() { return GetCoreTypeID( CoreTypeID::Color ); }
    template<> inline TypeID GetCoreTypeID<Float2>() { return GetCoreTypeID( CoreTypeID::Float2 ); }
    template<> inline TypeID GetCoreTypeID<Float3>() { return GetCoreTypeID( CoreTypeID::Float3 ); }
    template<> inline TypeID GetCoreTypeID<Float4>() { return GetCoreTypeID( CoreTypeID::Float4 ); }
    template<> inline TypeID GetCoreTypeID<Vector>() { return GetCoreTypeID( CoreTypeID::Vector ); }
    template<> inline TypeID GetCoreTypeID<Quaternion>() { return GetCoreTypeID( CoreTypeID::Quaternion ); }
    template<> inline TypeID GetCoreTypeID<Matrix>() { return GetCoreTypeID( CoreTypeID::Matrix ); }
    template<> inline TypeID GetCoreTypeID<Transform>() { return GetCoreTypeID( CoreTypeID::Transform ); }
    template<> inline TypeID GetCoreTypeID<Microseconds>() { return GetCoreTypeID( CoreTypeID::Microseconds ); }
    template<> inline TypeID GetCoreTypeID<Milliseconds>() { return GetCoreTypeID( CoreTypeID::Milliseconds ); }
    template<> inline TypeID GetCoreTypeID<Seconds>() { return GetCoreTypeID( CoreTypeID::Seconds ); }
    template<> inline TypeID GetCoreTypeID<Percentage>() { return GetCoreTypeID( CoreTypeID::Percentage ); }
    template<> inline TypeID GetCoreTypeID<Degrees>() { return GetCoreTypeID( CoreTypeID::Degrees ); }
    template<> inline TypeID GetCoreTypeID<Radians>() { return GetCoreTypeID( CoreTypeID::Radians ); }
    template<> inline TypeID GetCoreTypeID<EulerAngles >() { return GetCoreTypeID( CoreTypeID::EulerAngles ); }
    template<> inline TypeID GetCoreTypeID<IntRange>() { return GetCoreTypeID( CoreTypeID::IntRange ); }
    template<> inline TypeID GetCoreTypeID<FloatRange>() { return GetCoreTypeID( CoreTypeID::FloatRange ); }
    template<> inline TypeID GetCoreTypeID<FloatCurve>() { return GetCoreTypeID( CoreTypeID::FloatCurve ); }
    template<> inline TypeID GetCoreTypeID<BitFlags>() { return GetCoreTypeID( CoreTypeID::BitFlags ); }
    template<> inline TypeID GetCoreTypeID<TBitFlags>() { return GetCoreTypeID( CoreTypeID::TBitFlags ); }
    template<> inline TypeID GetCoreTypeID<TVector>() { return GetCoreTypeID( CoreTypeID::TVector ); }
    template<> inline TypeID GetCoreTypeID<ResourcePath>() { return GetCoreTypeID( CoreTypeID::ResourcePath ); }
    template<> inline TypeID GetCoreTypeID<ResourceTypeID>() { return GetCoreTypeID( CoreTypeID::ResourceTypeID ); }
    template<> inline TypeID GetCoreTypeID<ResourceID>() { return GetCoreTypeID( CoreTypeID::ResourceID ); }
    template<> inline TypeID GetCoreTypeID<Resource::ResourcePtr>() { return GetCoreTypeID( CoreTypeID::ResourcePtr ); }
    template<> inline TypeID GetCoreTypeID<TResourcePtr>() { return GetCoreTypeID( CoreTypeID::TResourcePtr ); }

    //-------------------------------------------------------------------------
    // Validation for getters/setters
    //-------------------------------------------------------------------------

    template<typename T>
    inline bool ValidateTypeAgainstTypeID( TypeID typeID )
    {
        TypeID const nativeTypeID = GetCoreTypeID<T>();

        // Handle derivation of ResourcePtr to TResourcePtr
        if ( nativeTypeID == GetCoreTypeID( CoreTypeID::ResourcePtr ) )
        {
            return typeID == GetCoreTypeID( CoreTypeID::ResourcePtr ) || typeID == GetCoreTypeID( CoreTypeID::TResourcePtr );
        }

        // Handle derivation of BitFlags to TBitFlags
        if ( nativeTypeID == GetCoreTypeID( CoreTypeID::BitFlags ) )
        {
            return typeID == GetCoreTypeID( CoreTypeID::BitFlags ) || typeID == GetCoreTypeID( CoreTypeID::TBitFlags );
        }

        //-------------------------------------------------------------------------

        return GetCoreTypeID<T>() == typeID;
    }
}

//-------------------------------------------------------------------------
// Global conversion operators
//-------------------------------------------------------------------------

inline bool operator==( EE::TypeSystem::TypeID const& typeID, EE::TypeSystem::CoreTypeID coreType )
{
    return typeID == EE::TypeSystem::GetCoreTypeID( coreType );
}

inline bool operator!=( EE::TypeSystem::TypeID const& typeID, EE::TypeSystem::CoreTypeID coreType )
{
    return typeID != EE::TypeSystem::GetCoreTypeID( coreType );
}

inline bool operator==( EE::TypeSystem::CoreTypeID coreType, EE::TypeSystem::TypeID const& typeID )
{
    return typeID == EE::TypeSystem::GetCoreTypeID( coreType );
}

inline bool operator!=( EE::TypeSystem::CoreTypeID coreType, EE::TypeSystem::TypeID const& typeID )
{
    return typeID != EE::TypeSystem::GetCoreTypeID( coreType );
}