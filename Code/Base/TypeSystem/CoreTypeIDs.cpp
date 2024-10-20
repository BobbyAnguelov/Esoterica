#include "CoreTypeIDs.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Types/Percentage.h"
#include "Base/Types/Color.h"
#include "Base/Types/BitFlags.h"
#include "Base/Types/Tag.h"
#include "Base/Time/Time.h"
#include "Base/Math/Transform.h"
#include "Base/Math/NumericRange.h"
#include "Base/Math/FloatCurve.h"
#include "TypeInstance.h"

//-------------------------------------------------------------------------

#define REGISTER_TYPE_RECORD( coreTypeEnum, fullyQualifiedTypeName ) \
ID = TypeID( #fullyQualifiedTypeName );\
EE_ASSERT( !CoreTypeRegistry::s_coreTypeRecords[(uint8_t)coreTypeEnum].m_ID.IsValid() );\
CoreTypeRegistry::s_coreTypeRecords[(uint8_t)coreTypeEnum] = { ID };\
EE_DEVELOPMENT_TOOLS_ONLY( Printf( CoreTypeRegistry::s_coreTypeRecords[(uint8_t)coreTypeEnum].m_friendlyName, 30, &#coreTypeEnum[12] ) );

//-------------------------------------------------------------------------

namespace EE::TypeSystem
{
    CoreTypeRegistry::CoreTypeRecord CoreTypeRegistry::s_coreTypeRecords[(uint8_t) CoreTypeID::NumTypes];
    bool CoreTypeRegistry::s_areCoreTypeRecordsInitialized = false;

    //-------------------------------------------------------------------------

    void CoreTypeRegistry::Initialize()
    {
        EE_ASSERT( !s_areCoreTypeRecordsInitialized );

        TypeID ID;

        REGISTER_TYPE_RECORD( CoreTypeID::Bool, bool );
        REGISTER_TYPE_RECORD( CoreTypeID::Uint8, uint8_t );
        REGISTER_TYPE_RECORD( CoreTypeID::Int8, int8_t );
        REGISTER_TYPE_RECORD( CoreTypeID::Uint16, uint16_t );
        REGISTER_TYPE_RECORD( CoreTypeID::Int16, int16_t );
        REGISTER_TYPE_RECORD( CoreTypeID::Uint32, uint32_t );
        REGISTER_TYPE_RECORD( CoreTypeID::Int32, int32_t );
        REGISTER_TYPE_RECORD( CoreTypeID::Uint64, uint64_t );
        REGISTER_TYPE_RECORD( CoreTypeID::Int64, int64_t );
        REGISTER_TYPE_RECORD( CoreTypeID::Float, float );
        REGISTER_TYPE_RECORD( CoreTypeID::Double, double );
        REGISTER_TYPE_RECORD( CoreTypeID::UUID, EE::UUID );
        REGISTER_TYPE_RECORD( CoreTypeID::StringID, EE::StringID );
        REGISTER_TYPE_RECORD( CoreTypeID::TypeID, EE::TypeSystem::TypeID );
        REGISTER_TYPE_RECORD( CoreTypeID::String, EE::String );
        REGISTER_TYPE_RECORD( CoreTypeID::Color, EE::Color );
        REGISTER_TYPE_RECORD( CoreTypeID::Float2, EE::Float2 );
        REGISTER_TYPE_RECORD( CoreTypeID::Float3, EE::Float3 );
        REGISTER_TYPE_RECORD( CoreTypeID::Float4, EE::Float4 );
        REGISTER_TYPE_RECORD( CoreTypeID::Vector, EE::Vector );
        REGISTER_TYPE_RECORD( CoreTypeID::Quaternion, EE::Quaternion );
        REGISTER_TYPE_RECORD( CoreTypeID::Matrix, EE::Matrix );
        REGISTER_TYPE_RECORD( CoreTypeID::Transform, EE::Transform );
        REGISTER_TYPE_RECORD( CoreTypeID::Microseconds, EE::Microseconds );
        REGISTER_TYPE_RECORD( CoreTypeID::Milliseconds, EE::Milliseconds );
        REGISTER_TYPE_RECORD( CoreTypeID::Seconds, EE::Seconds );
        REGISTER_TYPE_RECORD( CoreTypeID::Percentage, EE::Percentage );
        REGISTER_TYPE_RECORD( CoreTypeID::Degrees, EE::Degrees );
        REGISTER_TYPE_RECORD( CoreTypeID::Radians, EE::Radians );
        REGISTER_TYPE_RECORD( CoreTypeID::EulerAngles, EE::EulerAngles );
        REGISTER_TYPE_RECORD( CoreTypeID::IntRange, EE::IntRange );
        REGISTER_TYPE_RECORD( CoreTypeID::FloatRange, EE::FloatRange );
        REGISTER_TYPE_RECORD( CoreTypeID::FloatCurve, EE::FloatCurve );
        REGISTER_TYPE_RECORD( CoreTypeID::Tag2, EE::Tag2 );
        REGISTER_TYPE_RECORD( CoreTypeID::Tag4, EE::Tag4 );

        REGISTER_TYPE_RECORD( CoreTypeID::BitFlags, EE::BitFlags );
        REGISTER_TYPE_RECORD( CoreTypeID::TBitFlags, EE::TBitFlags );

        REGISTER_TYPE_RECORD( CoreTypeID::TVector, EE::TVector );
        REGISTER_TYPE_RECORD( CoreTypeID::TInlineVector, EE::TInlineVector );

        REGISTER_TYPE_RECORD( CoreTypeID::TypeInstance, EE::TypeInstance );
        REGISTER_TYPE_RECORD( CoreTypeID::TTypeInstance, EE::TTypeInstance );

        REGISTER_TYPE_RECORD( CoreTypeID::DataPath, EE::DataPath );
        REGISTER_TYPE_RECORD( CoreTypeID::TDataFilePath, EE::TDataFilePath );

        REGISTER_TYPE_RECORD( CoreTypeID::ResourceID, EE::ResourceID );
        REGISTER_TYPE_RECORD( CoreTypeID::ResourceTypeID, EE::ResourceTypeID );
        REGISTER_TYPE_RECORD( CoreTypeID::ResourcePtr, EE::Resource::ResourcePtr );
        REGISTER_TYPE_RECORD( CoreTypeID::TResourcePtr, EE::TResourcePtr );

        s_areCoreTypeRecordsInitialized = true;
    }

    void CoreTypeRegistry::Shutdown()
    {
        EE_ASSERT( s_areCoreTypeRecordsInitialized );
        s_areCoreTypeRecordsInitialized = false;
    }

    //-------------------------------------------------------------------------
 
    bool CoreTypeRegistry::IsCoreType( TypeID typeID )
    {
        EE_ASSERT( s_areCoreTypeRecordsInitialized );
        for ( auto i = 0; i < (uint8_t) CoreTypeID::NumTypes; i++ )
        {
            if ( s_coreTypeRecords[i].m_ID == typeID )
            {
                return true;
            }
        }

        return false;
    }

    CoreTypeID CoreTypeRegistry::GetType( TypeID typeID )
    {
        EE_ASSERT( s_areCoreTypeRecordsInitialized );

        for ( int32_t i = 0; i < (int32_t) CoreTypeID::NumTypes; i++ )
        {
            if ( CoreTypeRegistry::s_coreTypeRecords[i].m_ID == typeID )
            {
                return (CoreTypeID) i;
            }
        }

        return CoreTypeID::Invalid;
    }
}