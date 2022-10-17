#include "CoreTypeConversions.h"
#include "TypeRegistry.h"
#include "TypeInfo.h"
#include "System/Resource/ResourcePtr.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/Time/Time.h"
#include "System/Types/Tag.h"
#include "System/Types/Color.h"
#include "System/Types/UUID.h"
#include "System/Types/StringID.h"
#include "System/Types/BitFlags.h"
#include "System/Types/Percentage.h"
#include "System/Math/Transform.h"
#include "System/Math/FloatCurve.h"
#include "System/Math/NumericRange.h"
#include "System/Log.h"
#include "EnumInfo.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Conversion
{
    void StringToFloatArray( String const& str, int32_t const numFloats, float* pFloats )
    {
        char substr[128] = { 0 };
        int32_t resIdx = 0;

        bool complete = false;
        size_t startIdx = 0;
        while ( resIdx < numFloats && !complete )
        {
            size_t endIdx = str.find_first_of( ',', startIdx );
            if ( endIdx == String::npos )
            {
                endIdx = str.length();
                complete = true;
            }

            size_t sizeToCopy = endIdx - startIdx;
            EE_ASSERT( sizeToCopy < 128 );
            memcpy( substr, &str.c_str()[startIdx], sizeToCopy );
            substr[sizeToCopy] = '\0';

            pFloats[resIdx++] = std::strtof( substr, nullptr );
            startIdx = endIdx + 1;
        }
    }

    void FloatArrayToString( float const* pFloats, int32_t const numFloats, String& strValue )
    {
        strValue.clear();

        for ( int32_t i = 0; i < numFloats; i++ )
        {
            strValue += eastl::to_string( pFloats[i] );

            if ( i != ( numFloats - 1 ) )
            {
                strValue += ',';
            }
        }
    }

    void StringToIntArray( String const& str, int32_t const numInts, int32_t* pInts )
    {
        char substr[128] = { 0 };
        int32_t resIdx = 0;

        bool complete = false;
        size_t startIdx = 0;
        while ( resIdx < numInts && !complete )
        {
            size_t endIdx = str.find_first_of( ',', startIdx );
            if ( endIdx == String::npos )
            {
                endIdx = str.length();
                complete = true;
            }

            size_t sizeToCopy = endIdx - startIdx;
            EE_ASSERT( sizeToCopy < 128 );
            memcpy( substr, &str.c_str()[startIdx], sizeToCopy );
            substr[sizeToCopy] = '\0';

            pInts[resIdx++] = std::strtoul( substr, nullptr, 0 );
            startIdx = endIdx + 1;
        }
    }

    void IntArrayToString( int32_t const* pInts, int32_t const numInts, String& strValue )
    {
        strValue.clear();

        for ( int32_t i = 0; i < numInts; i++ )
        {
            strValue += eastl::to_string( pInts[i] );

            if ( i != ( numInts - 1 ) )
            {
                strValue += ',';
            }
        }
    }

    inline static bool ConvertStringToBitFlags( EnumInfo const& enumInfo, String const& str, BitFlags& outFlags )
    {
        outFlags.ClearAllFlags();

        if ( str.empty() )
        {
            return true;
        }

        // Handle hex format explicitly
        //-------------------------------------------------------------------------

        if ( str.find_first_of( "0x" ) == 0 || str.find_first_of( "0X" ) == 0 )
        {
            uint32_t value = std::strtoul( str.c_str(), nullptr, 16 );
            outFlags.Set( value );
            return true;
        }

        //-------------------------------------------------------------------------

        char buffer[256] = { 0 };
        size_t bufferIdx = 0;
        bool isReadingEnumValue = false;
        bool hasReadWhiteSpace = false;

        //-------------------------------------------------------------------------

        auto ProcessReadValue = [&] ()
        {
            if ( isReadingEnumValue )
            {
                buffer[bufferIdx] = 0;
                bufferIdx = 0;
                isReadingEnumValue = false;

                int64_t flag;
                if ( enumInfo.TryGetConstantValue( StringID( buffer ), flag ) )
                {
                    outFlags.SetFlag( (uint8_t) flag, true );
                    return true;
                }
            }

            return false;
        };

        //-------------------------------------------------------------------------

        size_t const length = str.length();
        for ( auto i = 0u; i < length; i++ )
        {
            if ( str[i] == '|' )
            {
                if ( !ProcessReadValue() )
                {
                    outFlags.ClearAllFlags();
                    return false;
                }
                hasReadWhiteSpace = false;
            }
            else if ( str[i] == ' ' )
            {
                if ( isReadingEnumValue )
                {
                    hasReadWhiteSpace = true;
                }
            }
            else
            {
                // We read a white space while reading an enum value
                if ( hasReadWhiteSpace )
                {
                    outFlags.ClearAllFlags();
                    return false;
                }

                isReadingEnumValue = true;
                buffer[bufferIdx] = str[i];
                bufferIdx++;
                EE_ASSERT( bufferIdx < 256 );
            }
        }

        //-------------------------------------------------------------------------

        if ( !ProcessReadValue() )
        {
            outFlags.ClearAllFlags();
            return false;
        }

        //-------------------------------------------------------------------------

        return true;
    }

    inline static bool ConvertBitFlagsToString( EnumInfo const& enumInfo, BitFlags const& flags, String& strValue )
    {
        strValue.clear();

        for ( auto i = 0u; i < 32; i++ )
        {
            if ( flags.IsFlagSet( (uint8_t) i ) )
            {
                StringID label;
                if ( !enumInfo.TryGetConstantLabel( i, label ) )
                {
                    strValue.clear();
                    return false;
                }

                strValue += label.c_str();
                strValue += "|";
            }
        }

        //-------------------------------------------------------------------------

        if ( !strValue.empty() )
        {
            strValue.erase( strValue.end() - 1, strValue.end() );
        }

        return true;
    }

    //-------------------------------------------------------------------------

    template<typename T>
    bool ConvertToBinary( TypeRegistry const& typeRegistry, TypeID typeID, TypeID templateArgumentTypeID, String const& strValue, Blob& byteArray )
    {
        T value;
        if ( !ConvertStringToNativeType( typeRegistry, typeID, templateArgumentTypeID, strValue, &value ) )
        {
            return false;
        }

        return ConvertNativeTypeToBinary( typeRegistry, typeID, templateArgumentTypeID, &value, byteArray );
    }

    //-------------------------------------------------------------------------

    bool ConvertStringToNativeType( TypeRegistry const& typeRegistry, TypeID typeID, TypeID templateArgumentTypeID, String const& str, void* pValue )
    {
        // Enums
        if( !IsCoreType( typeID ) ) 
        {
            EnumInfo const* pEnumInfo = typeRegistry.GetEnumInfo( typeID );
            EE_ASSERT( pEnumInfo != nullptr );
            
            StringID const enumID( str );
            int64_t const enumValue = pEnumInfo->GetConstantValue( enumID );

            // We only support up to 32 bit enum types...
            switch ( pEnumInfo->m_underlyingType )
            {
                case CoreTypeID::Uint8:
                {
                    *( (uint8_t*) pValue ) = (uint8_t) enumValue;
                }
                break;

                case CoreTypeID::Int8:
                {
                    *( (int8_t*) pValue ) = (int8_t) enumValue;
                }
                break;

                case CoreTypeID::Uint16:
                {
                    *( (uint16_t*) pValue ) = (uint16_t) enumValue;
                }
                break;

                case CoreTypeID::Int16:
                {
                    *( (int16_t*) pValue ) = (int16_t) enumValue;
                }
                break;

                case CoreTypeID::Uint32:
                {
                    *( (uint32_t*) pValue ) = (uint32_t) enumValue;
                }
                break;

                case CoreTypeID::Int32:
                {
                    *( (int32_t*) pValue ) = (int32_t) enumValue;
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return false;
                }
                break;
            }
        }
        else // Real core types
        {
            CoreTypeID const typeToConvert = GetCoreType( typeID );
            switch ( typeToConvert )
            {
                case CoreTypeID::Bool :
                {
                    String lowerString = str;
                    lowerString.make_lower();
                    *reinterpret_cast<bool*>( pValue ) = ( lowerString == "true" );
                }
                break;

                case CoreTypeID::Uint8 :
                {
                    *reinterpret_cast<uint8_t*>( pValue ) = (uint8_t) std::strtoul( str.c_str(), nullptr, 0 );
                }
                break;

                case CoreTypeID::Int8 :
                {
                    *reinterpret_cast<int8_t*>( pValue ) = (int8_t) std::strtol( str.c_str(), nullptr, 0 );
                }
                break;

                case CoreTypeID::Uint16 :
                {
                    *reinterpret_cast<uint16_t*>( pValue ) = (uint16_t) std::strtoul( str.c_str(), nullptr, 0 );
                }
                break;

                case CoreTypeID::Int16 :
                {
                    *reinterpret_cast<int16_t*>( pValue ) = (int16_t) std::strtol( str.c_str(), nullptr, 0 );
                }
                break;

                case CoreTypeID::Uint32 :
                {
                    *reinterpret_cast<uint32_t*>( pValue ) = std::strtoul( str.c_str(), nullptr, 0 );
                }
                break;

                case CoreTypeID::Int32 :
                {
                    *reinterpret_cast<int32_t*>( pValue ) = std::strtol( str.c_str(), nullptr, 0 );
                }
                break;

                case CoreTypeID::Uint64 :
                {
                    *reinterpret_cast<uint64_t*>( pValue ) = std::strtoull( str.c_str(), nullptr, 0 );
                }
                break;

                case CoreTypeID::Int64:
                {
                    *reinterpret_cast<int64_t*>( pValue ) = std::strtol( str.c_str(), nullptr, 0 );
                }
                break;

                case CoreTypeID::Float :
                {
                    *reinterpret_cast<float*>( pValue ) = std::strtof( str.c_str(), nullptr );
                }
                break;

                case CoreTypeID::Double :
                {
                    *reinterpret_cast<double*>( pValue ) = std::strtod( str.c_str(), nullptr );
                }
                break;

                case CoreTypeID::String :
                {
                    *reinterpret_cast<String*>( pValue ) = str;
                }
                break;

                case CoreTypeID::StringID :
                {
                    *reinterpret_cast<StringID*>( pValue ) = StringID( str.c_str() );
                }
                break;

                case CoreTypeID::Tag:
                {
                    *reinterpret_cast<Tag*>( pValue ) = Tag::FromTagFormatString( str );
                }
                break;

                case CoreTypeID::TypeID:
                {
                    *reinterpret_cast<TypeID*>( pValue ) = TypeID( str.c_str() );
                }
                break;

                case CoreTypeID::UUID :
                {
                    *reinterpret_cast<UUID*>( pValue ) = UUID( str );
                }
                break;

                case CoreTypeID::Color :
                {
                    uint32_t const colorType = std::strtoul( str.c_str(), nullptr, 16 );
                    *reinterpret_cast<Color*>( pValue ) = Color( colorType );
                }
                break;

                case CoreTypeID::Float2 :
                {
                    StringToFloatArray( str, 2, &reinterpret_cast<Float2*>( pValue )->m_x );
                }
                break;

                case CoreTypeID::Float3:
                {
                    StringToFloatArray( str, 3, &reinterpret_cast<Float3*>( pValue )->m_x );
                }
                break;

                case CoreTypeID::Float4:
                {
                    StringToFloatArray( str, 4, &reinterpret_cast<Float4*>( pValue )->m_x );
                }
                break;

                case CoreTypeID::Vector:
                {
                    StringToFloatArray( str, 4, &reinterpret_cast<Vector*>( pValue )->m_x );
                }
                break;

                case CoreTypeID::Quaternion:
                {
                    StringToFloatArray( str, 4, &reinterpret_cast<Quaternion*>( pValue )->m_x );
                }
                break;

                case CoreTypeID::Matrix:
                {
                    float floatData[7];
                    StringToFloatArray( str, 7, floatData );

                    EulerAngles const rotation( floatData[0], floatData[1], floatData[2] );
                    Vector const translation = Vector( floatData[3], floatData[4], floatData[5] );
                    *reinterpret_cast<Matrix*>( pValue ) = Matrix( Quaternion( rotation ), translation, floatData[6] );
                }
                break;

                case CoreTypeID::Transform:
                {
                    float floatData[7];
                    StringToFloatArray( str, 7, floatData );

                    EulerAngles const rotation( floatData[0], floatData[1], floatData[2] );
                    Vector const translation( floatData[3], floatData[4], floatData[5] );
                    *reinterpret_cast<Transform*>( pValue ) = Transform( Quaternion( rotation ), translation, floatData[6] );
                }
                break;

                case CoreTypeID::EulerAngles:
                {
                    Float3 angles;
                    StringToFloatArray( str, 3, &angles.m_x );
                    *reinterpret_cast<EulerAngles*>( pValue ) = EulerAngles( angles.m_x, angles.m_y, angles.m_z );
                }
                break;

                case CoreTypeID::Microseconds:
                {
                    *reinterpret_cast<Microseconds*>( pValue ) = Microseconds( std::strtof( str.c_str(), nullptr ) );
                }
                break;

                case CoreTypeID::Milliseconds:
                {
                    *reinterpret_cast<Milliseconds*>( pValue ) = Milliseconds( std::strtof( str.c_str(), nullptr ) );
                }
                break;

                case CoreTypeID::Seconds:
                {
                    *reinterpret_cast<Seconds*>( pValue ) = Seconds( std::strtof( str.c_str(), nullptr ) );
                }
                break;

                case CoreTypeID::Percentage:
                {
                    *reinterpret_cast<Percentage*>( pValue ) = Percentage( std::strtof( str.c_str(), nullptr ) );
                }
                break;

                case CoreTypeID::Degrees:
                {
                    *reinterpret_cast<Degrees*>( pValue ) = Degrees( std::strtof( str.c_str(), nullptr ) );
                }
                break;

                case CoreTypeID::Radians:
                {
                    *reinterpret_cast<Radians*>( pValue ) = Radians( std::strtof( str.c_str(), nullptr ) );
                }
                break;

                case CoreTypeID::ResourcePath:
                {
                    *reinterpret_cast<ResourcePath*>( pValue ) = ResourcePath( str );
                }
                break;

                case CoreTypeID::IntRange:
                {
                    int32_t intData[2];
                    StringToIntArray( str, 2, intData );
                    *reinterpret_cast<IntRange*>( pValue ) = IntRange( intData[0], intData[1] );
                }
                break;

                case CoreTypeID::FloatRange:
                {
                    float floatData[2];
                    StringToFloatArray( str, 2, floatData );
                    *reinterpret_cast<FloatRange*>( pValue ) = FloatRange( floatData[0], floatData[1] );
                }
                break;

                case CoreTypeID::FloatCurve:
                {
                    FloatCurve& outCurve = *reinterpret_cast<FloatCurve*>( pValue );
                    if ( !FloatCurve::FromString( str, outCurve ) )
                    {
                        return false;
                    }
                }
                break;

                case CoreTypeID::ResourceTypeID:
                {
                    *reinterpret_cast<ResourceTypeID*>( pValue ) = ResourceTypeID( str );
                }
                break;

                case CoreTypeID::ResourcePtr:
                case CoreTypeID::TResourcePtr:
                {
                    Resource::ResourcePtr& resourcePtr = *reinterpret_cast<Resource::ResourcePtr*>( pValue );
                    if ( str.empty() )
                    {
                        resourcePtr = Resource::ResourcePtr();
                    }
                    else
                    {
                        if ( !ResourceID::IsValidResourceIDString( str ) )
                        {
                            EE_LOG_WARNING( "TypeSystem", "Core Type Conversions", "Invalid resource ID string encountered: %s", str.c_str());
                            return false;
                        }
                        ResourceID const ID( str );
                        resourcePtr = Resource::ResourcePtr( ID );
                    }
                }
                break;

                case CoreTypeID::ResourceID:
                {
                    if ( str.empty() )
                    {
                        *reinterpret_cast<ResourceID*>( pValue ) = ResourceID();
                    }
                    else
                    {
                        if ( !ResourceID::IsValidResourceIDString( str ) )
                        {
                            EE_LOG_WARNING( "TypeSystem", "Core Type Conversions", "Invalid resource ID string encountered: %s", str.c_str() );
                            return false;
                        }
                        *reinterpret_cast<ResourceID*>( pValue ) = ResourceID( str );
                    }
                }
                break;

                case CoreTypeID::BitFlags:
                {
                    reinterpret_cast<BitFlags*>( pValue )->Set( std::strtol( str.c_str(), nullptr, 0 ) );
                }
                break;

                case CoreTypeID::TBitFlags:
                {
                    EnumInfo const* pEnumInfo = typeRegistry.GetEnumInfo( templateArgumentTypeID );
                    if ( pEnumInfo == nullptr )
                    {
                        EE_LOG_WARNING( "TypeSystem", "Core Type Conversions", "Unknown enum class (%s) for TBitFlags", templateArgumentTypeID.ToStringID().c_str() );
                        return false;
                    }

                    if ( !ConvertStringToBitFlags( *pEnumInfo, str, *reinterpret_cast<BitFlags*>( pValue ) ) )
                    {
                        EE_LOG_WARNING( "TypeSystem", "Core Type Conversions", "Failed to convert string (%s) into valid TBitFlags<%s>", str.c_str(), templateArgumentTypeID.ToStringID().c_str() );
                        return false;
                    }
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return false;
                }
                break;
            }
        }

        return true;
    }

    bool ConvertNativeTypeToString( TypeRegistry const& typeRegistry, TypeID typeID, TypeID templateArgumentTypeID, void const* pValue, String & strValue )
    {
        // Enums
        if ( !IsCoreType( typeID ) )
        {
            EnumInfo const* pEnumInfo = typeRegistry.GetEnumInfo( typeID );
            EE_ASSERT( pEnumInfo != nullptr );

            // We only support up to 32 bit enum types...
            switch ( pEnumInfo->m_underlyingType )
            {
                case CoreTypeID::Uint8:
                {
                    strValue = pEnumInfo->GetConstantLabel( *( (uint8_t*) pValue ) ).c_str();
                }
                break;

                case CoreTypeID::Int8:
                {
                    strValue = pEnumInfo->GetConstantLabel( *( (int8_t*) pValue ) ).c_str();
                }
                break;

                case CoreTypeID::Uint16:
                {
                    strValue = pEnumInfo->GetConstantLabel( *( (uint16_t*) pValue ) ).c_str();
                }
                break;

                case CoreTypeID::Int16:
                {
                    strValue = pEnumInfo->GetConstantLabel( *( (int16_t*) pValue ) ).c_str();
                }
                break;

                case CoreTypeID::Uint32:
                {
                    strValue = pEnumInfo->GetConstantLabel( *( (uint32_t*) pValue ) ).c_str();
                }
                break;

                case CoreTypeID::Int32:
                {
                    strValue = pEnumInfo->GetConstantLabel( *( (int32_t*) pValue ) ).c_str();
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return false;
                }
                break;
            }
        }
        else  // Real core types
        {
            CoreTypeID const typeToConvert = GetCoreType( typeID );
            switch ( typeToConvert )
            {
                case CoreTypeID::Bool:
                {
                    strValue = *reinterpret_cast<bool const*>( pValue ) ? "True" : "False";
                }
                break;

                case CoreTypeID::Uint8:
                {
                    strValue = eastl::to_string( *reinterpret_cast<uint8_t const*>( pValue ) );
                }
                break;

                case CoreTypeID::Int8:
                {
                    strValue = eastl::to_string( *reinterpret_cast<int8_t const*>( pValue ) );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    strValue = eastl::to_string( *reinterpret_cast<uint16_t const*>( pValue ) );
                }
                break;

                case CoreTypeID::Int16:
                {
                    strValue = eastl::to_string( *reinterpret_cast<int16_t const*>( pValue ) );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    strValue = eastl::to_string( *reinterpret_cast<uint32_t const*>( pValue ) );
                }
                break;

                case CoreTypeID::Int32:
                {
                    strValue = eastl::to_string( *reinterpret_cast<int32_t const*>( pValue ) );
                }
                break;

                case CoreTypeID::Uint64:
                {
                    strValue = eastl::to_string( *reinterpret_cast<uint64_t const*>( pValue ) );
                }
                break;

                case CoreTypeID::Int64:
                {
                    strValue = eastl::to_string( *reinterpret_cast<int64_t const*>( pValue ) );
                }
                break;

                case CoreTypeID::Float:
                {
                    strValue = eastl::to_string( *reinterpret_cast<float const*>( pValue ) );
                }
                break;

                case CoreTypeID::Double:
                {
                    strValue = eastl::to_string( *reinterpret_cast<double const*>( pValue ) );
                }
                break;

                case CoreTypeID::String:
                {
                    strValue = *reinterpret_cast<String const*>( pValue );
                }
                break;

                case CoreTypeID::StringID:
                {
                    char const* pStr = reinterpret_cast<StringID const*>( pValue )->c_str();
                    if ( pStr != nullptr )
                    {
                        strValue = pStr;
                    }
                    else
                    {
                        strValue.clear();
                    }
                }
                break;

                case CoreTypeID::Tag:
                {
                    strValue = reinterpret_cast<Tag const*>( pValue )->ToString();
                }
                break;

                case CoreTypeID::TypeID:
                {
                    char const* pStr = reinterpret_cast<TypeID const*>( pValue )->ToStringID().c_str();
                    if (pStr != nullptr)
                    {
                        strValue = pStr;
                    }
                    else
                    {
                        strValue.clear();
                    }
                }
                break;

                case CoreTypeID::UUID:
                {
                    strValue = reinterpret_cast<UUID const*>( pValue )->ToString();
                }
                break;

                case CoreTypeID::Color:
                {
                    Color const& value = *reinterpret_cast<Color const*>( pValue );
                    strValue.sprintf( "%02X%02X%02X%02X", value.m_byteColor.m_r, value.m_byteColor.m_g, value.m_byteColor.m_b, value.m_byteColor.m_a );
                }
                break;

                case CoreTypeID::Float2:
                {
                    FloatArrayToString( &reinterpret_cast<Float2 const*>( pValue )->m_x, 2, strValue );
                }
                break;

                case CoreTypeID::Float3:
                {
                    FloatArrayToString( &reinterpret_cast<Float3 const*>( pValue )->m_x, 3, strValue );
                }
                break;

                case CoreTypeID::Float4:
                {
                    FloatArrayToString( &reinterpret_cast<Float4 const*>( pValue )->m_x, 4, strValue );
                }
                break;

                case CoreTypeID::Vector:
                {
                    FloatArrayToString( &reinterpret_cast<Vector const*>( pValue )->m_x, 4, strValue );
                }
                break;

                case CoreTypeID::Quaternion:
                {
                    FloatArrayToString( &reinterpret_cast<Quaternion const*>( pValue )->m_x, 4, strValue );
                }
                break;

                case CoreTypeID::Matrix:
                {
                    Matrix const& value = *reinterpret_cast<Matrix const*>( pValue );

                    // Handle uninitialized matrix
                    if ( value.IsOrthonormal() )
                    {
                        auto eulerAngles = value.ToEulerAngles();

                        float floatData[7];
                        (Float3&) floatData = Float3( (float) eulerAngles.m_x.ToDegrees(), (float) eulerAngles.m_y.ToDegrees(), (float) eulerAngles.m_z.ToDegrees() );
                        (Float3&) floatData[3] = value.GetTranslation().ToFloat3();
                        floatData[6] = value.GetScale().m_x;

                        FloatArrayToString( floatData, 7, strValue );
                    }
                    else
                    {
                        strValue = "0,0,0,0,0,0,0";
                    }
                }
                break;

                case CoreTypeID::Transform:
                {
                    Transform const& transform = *reinterpret_cast<Transform const*>( pValue );
                    auto eulerAngles = transform.GetRotation().ToEulerAngles();

                    float floatData[7];

                    floatData[0] = (float) eulerAngles.m_x.ToDegrees();
                    floatData[1] = (float) eulerAngles.m_y.ToDegrees();
                    floatData[2] = (float) eulerAngles.m_z.ToDegrees();

                    floatData[3] = transform.GetTranslation().m_x;
                    floatData[4] = transform.GetTranslation().m_y;
                    floatData[5] = transform.GetTranslation().m_z;

                    floatData[6] = transform.GetScale();

                    //-------------------------------------------------------------------------

                    FloatArrayToString( floatData, 7, strValue );
                }
                break;

                case CoreTypeID::EulerAngles:
                {
                    Float3 const angles = reinterpret_cast<EulerAngles const*>( pValue )->GetAsDegrees();
                    FloatArrayToString( &angles.m_x, 3, strValue );
                }
                break;

                case CoreTypeID::Microseconds:
                {
                    strValue = eastl::to_string( reinterpret_cast<Microseconds const*>( pValue )->ToFloat() );
                }
                break;

                case CoreTypeID::Milliseconds:
                {
                    strValue = eastl::to_string( reinterpret_cast<Milliseconds const*>( pValue )->ToFloat() );
                }
                break;

                case CoreTypeID::Seconds:
                {
                    strValue = eastl::to_string( reinterpret_cast<Seconds const*>( pValue )->ToFloat() );
                }
                break;

                case CoreTypeID::Percentage:
                {
                    strValue = eastl::to_string( reinterpret_cast<Percentage const*>( pValue )->ToFloat() );
                }
                break;

                case CoreTypeID::Degrees:
                {
                    strValue = eastl::to_string( reinterpret_cast<Degrees const*>( pValue )->ToFloat() );
                }
                break;

                case CoreTypeID::Radians:
                {
                    strValue = eastl::to_string( reinterpret_cast<Radians const*>( pValue )->ToFloat() );
                }
                break;

                case CoreTypeID::ResourcePath:
                {
                    strValue = reinterpret_cast<ResourcePath const*>( pValue )->c_str();
                }
                break;

                case CoreTypeID::IntRange:
                {
                    IntRange const* pRange = reinterpret_cast<IntRange const*>( pValue );
                    IntArrayToString( &pRange->m_begin, 3, strValue );
                }
                break;

                case CoreTypeID::FloatRange:
                {
                    FloatRange const* pRange = reinterpret_cast<FloatRange const*>( pValue );
                    FloatArrayToString( &pRange->m_begin, 3, strValue );
                }
                break;

                case CoreTypeID::FloatCurve:
                {
                    FloatCurve const* pCurve = reinterpret_cast<FloatCurve const*>( pValue );
                    strValue = pCurve->ToString();
                }
                break;

                case CoreTypeID::ResourceTypeID:
                {
                    strValue = reinterpret_cast<ResourceTypeID const*>( pValue )->ToString();
                }
                break;

                case CoreTypeID::ResourcePtr:
                case CoreTypeID::TResourcePtr:
                {
                    strValue = reinterpret_cast<Resource::ResourcePtr const*>( pValue )->GetResourceID().ToString();
                }
                break;

                case CoreTypeID::ResourceID:
                {
                    strValue = reinterpret_cast<ResourceID const*>( pValue )->ToString();
                }
                break;

                case CoreTypeID::BitFlags:
                {
                    strValue = eastl::to_string( reinterpret_cast<BitFlags const*>( pValue )->Get() );
                }
                break;

                case CoreTypeID::TBitFlags:
                {
                    EnumInfo const* pEnumInfo = typeRegistry.GetEnumInfo( templateArgumentTypeID );
                    if ( pEnumInfo == nullptr )
                    {
                        EE_LOG_WARNING( "TypeSystem", "Core Type Conversions", "Unknown enum class (%s) for TBitFlags", templateArgumentTypeID.ToStringID().c_str() );
                        return false;
                    }

                    if ( !ConvertBitFlagsToString( *pEnumInfo, *reinterpret_cast<BitFlags const*>( pValue ), strValue ) )
                    {
                        EE_LOG_WARNING( "TypeSystem", "Core Type Conversions", "Failed to convert string (%s) into valid TBitFlags<%s>", strValue.c_str(), templateArgumentTypeID.ToStringID().c_str() );
                        return false;
                    }
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return false;
                }
                break;
            }
        }

        return false;
    }

    bool ConvertNativeTypeToBinary( TypeRegistry const& typeRegistry, TypeID typeID, TypeID templateArgumentTypeID, void const* pValue, Blob& byteArray )
    {
        Serialization::BinaryOutputArchive archive;

        // Enums
        if ( !IsCoreType( typeID ) )
        {
            EnumInfo const* pEnumInfo = typeRegistry.GetEnumInfo( typeID );
            EE_ASSERT( pEnumInfo != nullptr );

            // We only support up to 32 bit enum types...
            switch ( pEnumInfo->m_underlyingType )
            {
                case CoreTypeID::Uint8:
                {
                    archive << *reinterpret_cast<uint8_t const*>( pValue );
                }
                break;

                case CoreTypeID::Int8:
                {
                    archive << *reinterpret_cast<int8_t const*>( pValue );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    archive << *reinterpret_cast<uint16_t const*>( pValue );
                }
                break;

                case CoreTypeID::Int16:
                {
                    archive << *reinterpret_cast<int16_t const*>( pValue );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    archive << *reinterpret_cast<uint32_t const*>( pValue );
                }
                break;

                case CoreTypeID::Int32:
                {
                    archive << *reinterpret_cast<int32_t const*>( pValue );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return false;
                }
                break;
            }
        }
        else // Real core types
        {
            CoreTypeID const typeToConvert = GetCoreType( typeID );
            switch ( typeToConvert )
            {
                case CoreTypeID::Bool:
                {
                    archive << *reinterpret_cast<bool const*>( pValue );
                }
                break;

                case CoreTypeID::Uint8:
                {
                    archive << *reinterpret_cast<uint8_t const*>( pValue );
                }
                break;

                case CoreTypeID::Int8:
                {
                    archive << *reinterpret_cast<int8_t const*>( pValue );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    archive << *reinterpret_cast<uint16_t const*>( pValue );
                }
                break;

                case CoreTypeID::Int16:
                {
                    archive << *reinterpret_cast<int16_t const*>( pValue );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    archive << *reinterpret_cast<uint32_t const*>( pValue );
                }
                break;

                case CoreTypeID::Int32:
                {
                    archive << *reinterpret_cast<int32_t const*>( pValue );
                }
                break;

                case CoreTypeID::Uint64:
                {
                    archive << *reinterpret_cast<uint64_t const*>( pValue );
                }
                break;

                case CoreTypeID::Int64:
                {
                    archive << *reinterpret_cast<int64_t const*>( pValue );
                }
                break;

                case CoreTypeID::Float:
                {
                    archive << *reinterpret_cast<float const*>( pValue );
                }
                break;

                case CoreTypeID::Double:
                {
                    archive << *reinterpret_cast<double const*>( pValue );
                }
                break;

                case CoreTypeID::String:
                {
                    archive << *reinterpret_cast<String const*>( pValue );
                }
                break;

                case CoreTypeID::StringID:
                {
                    archive << *reinterpret_cast<StringID const*>( pValue );
                }
                break;

                case CoreTypeID::Tag:
                {
                    archive << *reinterpret_cast<Tag const*>( pValue );
                }
                break;

                case CoreTypeID::TypeID:
                {
                    archive << reinterpret_cast<TypeID const*>( pValue )->ToStringID();
                }
                break;

                case CoreTypeID::UUID:
                {
                    archive << *reinterpret_cast<UUID const*>( pValue );
                }
                break;

                case CoreTypeID::Color:
                {
                    archive << *reinterpret_cast<Color const*>( pValue );
                }
                break;

                case CoreTypeID::Float2:
                {
                    archive << *reinterpret_cast<Float2 const*>( pValue );
                }
                break;

                case CoreTypeID::Float3:
                {
                    archive << *reinterpret_cast<Float3 const*>( pValue );
                }
                break;

                case CoreTypeID::Float4:
                {
                    archive << *reinterpret_cast<Float4 const*>( pValue );
                }
                break;

                case CoreTypeID::Vector:
                {
                    archive << *reinterpret_cast<Vector const*>( pValue );
                }
                break;

                case CoreTypeID::Quaternion:
                {
                    archive << *reinterpret_cast<Quaternion const*>( pValue );
                }
                break;

                case CoreTypeID::Matrix:
                {
                    archive << *reinterpret_cast<Matrix const*>( pValue );
                }
                break;

                case CoreTypeID::Transform:
                {
                    archive << *reinterpret_cast<Transform const*>( pValue );
                }
                break;

                case CoreTypeID::EulerAngles:
                {
                    archive << *reinterpret_cast<EulerAngles const*>( pValue );
                }
                break;

                case CoreTypeID::Microseconds:
                {
                    archive << *reinterpret_cast<Microseconds const*>( pValue );
                }
                break;

                case CoreTypeID::Milliseconds:
                {
                    archive << *reinterpret_cast<Milliseconds const*>( pValue );
                }
                break;

                case CoreTypeID::Seconds:
                {
                    archive << *reinterpret_cast<Seconds const*>( pValue );
                }
                break;

                case CoreTypeID::Percentage:
                {
                    archive << *reinterpret_cast<Percentage const*>( pValue );
                }
                break;

                case CoreTypeID::Degrees:
                {
                    archive << *reinterpret_cast<Degrees const*>( pValue );
                }
                break;

                case CoreTypeID::Radians:
                {
                    archive << *reinterpret_cast<Radians const*>( pValue );
                }
                break;

                case CoreTypeID::ResourcePath:
                {
                    archive << *reinterpret_cast<ResourcePath const*>( pValue );
                }
                break;

                case CoreTypeID::IntRange:
                {
                    archive << *reinterpret_cast<IntRange const*>( pValue );
                }
                break;

                case CoreTypeID::FloatRange:
                {
                    archive << *reinterpret_cast<FloatRange const*>( pValue );
                }
                break;

                case CoreTypeID::FloatCurve:
                {
                    archive << *reinterpret_cast<FloatCurve const*>( pValue );
                }
                break;

                case CoreTypeID::ResourceTypeID:
                {
                    archive << *reinterpret_cast<ResourceTypeID const*>( pValue );
                }
                break;

                case CoreTypeID::ResourcePtr:
                case CoreTypeID::TResourcePtr:
                {
                    archive << *reinterpret_cast<Resource::ResourcePtr const*>( pValue );
                }
                break;

                case CoreTypeID::ResourceID:
                {
                    archive << *reinterpret_cast<ResourceID const*>( pValue );
                }
                break;

                case CoreTypeID::BitFlags:
                case CoreTypeID::TBitFlags:
                {
                    archive << *reinterpret_cast<BitFlags const*>( pValue );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return false;
                }
                break;
            }
        }

        archive.GetAsBinaryBlob( byteArray );
        return true;
    }

    bool ConvertBinaryToNativeType( TypeRegistry const& typeRegistry, TypeID typeID, TypeID templateArgumentTypeID, Blob const& byteArray, void* pValue )
    {
        Serialization::BinaryInputArchive archive;
        archive.ReadFromBlob( byteArray );

        // Enums
        if ( !IsCoreType( typeID ) )
        {
            EnumInfo const* pEnumInfo = typeRegistry.GetEnumInfo( typeID );
            EE_ASSERT( pEnumInfo != nullptr );

            // We only support up to 32 bit enum types...
            switch ( pEnumInfo->m_underlyingType )
            {
                case CoreTypeID::Uint8:
                {
                    archive << *reinterpret_cast<uint8_t*>( pValue );
                }
                break;

                case CoreTypeID::Int8:
                {
                    archive << *reinterpret_cast<int8_t*>( pValue );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    archive << *reinterpret_cast<uint16_t*>( pValue );
                }
                break;

                case CoreTypeID::Int16:
                {
                    archive << *reinterpret_cast<int16_t*>( pValue );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    archive << *reinterpret_cast<uint32_t*>( pValue );
                }
                break;

                case CoreTypeID::Int32:
                {
                    archive << *reinterpret_cast<int32_t*>( pValue );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return false;
                }
                break;
            }
        }
        else // Real core types
        {
            CoreTypeID const typeToConvert = GetCoreType( typeID );
            switch ( typeToConvert )
            {
                case CoreTypeID::Bool:
                {
                    archive << *reinterpret_cast<bool*>( pValue );
                }
                break;

                case CoreTypeID::Uint8:
                {
                    archive << *reinterpret_cast<uint8_t*>( pValue );
                }
                break;

                case CoreTypeID::Int8:
                {
                    archive << *reinterpret_cast<int8_t*>( pValue );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    archive << *reinterpret_cast<uint16_t*>( pValue );
                }
                break;

                case CoreTypeID::Int16:
                {
                    archive << *reinterpret_cast<int16_t*>( pValue );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    archive << *reinterpret_cast<uint32_t*>( pValue );
                }
                break;

                case CoreTypeID::Int32:
                {
                    archive << *reinterpret_cast<int32_t*>( pValue );
                }
                break;

                case CoreTypeID::Uint64:
                {
                    archive << *reinterpret_cast<uint64_t*>( pValue );
                }
                break;

                case CoreTypeID::Int64:
                {
                    archive << *reinterpret_cast<int64_t*>( pValue );
                }
                break;

                case CoreTypeID::Float:
                {
                    archive << *reinterpret_cast<float*>( pValue );
                }
                break;

                case CoreTypeID::Double:
                {
                    archive << *reinterpret_cast<double*>( pValue );
                }
                break;

                case CoreTypeID::String:
                {
                    archive << *reinterpret_cast<String*>( pValue );
                }
                break;

                case CoreTypeID::StringID:
                {
                    archive << *reinterpret_cast<StringID*>( pValue );
                }
                break;

                case CoreTypeID::Tag:
                {
                   archive << *reinterpret_cast<Tag*>( pValue );
                }
                break;

                case CoreTypeID::TypeID:
                {
                    StringID ID;
                    archive << ID;
                    *reinterpret_cast<TypeID*>( pValue ) = TypeID( ID );
                }
                break;

                case CoreTypeID::UUID:
                {
                    archive << *reinterpret_cast<UUID*>( pValue );
                }
                break;

                case CoreTypeID::Color:
                {
                    archive << *reinterpret_cast<Color*>( pValue );
                }
                break;

                case CoreTypeID::Float2:
                {
                    archive << *reinterpret_cast<Float2*>( pValue );
                }
                break;

                case CoreTypeID::Float3:
                {
                    archive << *reinterpret_cast<Float3*>( pValue );
                }
                break;

                case CoreTypeID::Float4:
                {
                    archive << *reinterpret_cast<Float4*>( pValue );
                }
                break;

                case CoreTypeID::Vector:
                {
                    archive << *reinterpret_cast<Vector*>( pValue );
                }
                break;

                case CoreTypeID::Quaternion:
                {
                    archive << *reinterpret_cast<Quaternion*>( pValue );
                }
                break;

                case CoreTypeID::Matrix:
                {
                    archive << *reinterpret_cast<Matrix*>( pValue );
                }
                break;

                case CoreTypeID::Transform:
                {
                    archive << *reinterpret_cast<Transform*>( pValue );
                }
                break;

                case CoreTypeID::EulerAngles:
                {
                    archive << *reinterpret_cast<EulerAngles*>( pValue );
                }
                break;

                case CoreTypeID::Microseconds:
                {
                    archive << *reinterpret_cast<Microseconds*>( pValue );
                }
                break;

                case CoreTypeID::Milliseconds:
                {
                    archive << *reinterpret_cast<Milliseconds*>( pValue );
                }
                break;

                case CoreTypeID::Seconds:
                {
                    archive << *reinterpret_cast<Seconds*>( pValue );
                }
                break;

                case CoreTypeID::Percentage:
                {
                    archive << *reinterpret_cast<Percentage*>( pValue );
                }
                break;

                case CoreTypeID::Degrees:
                {
                    archive << *reinterpret_cast<Degrees*>( pValue );
                }
                break;

                case CoreTypeID::Radians:
                {
                    archive << *reinterpret_cast<Radians*>( pValue );
                }
                break;

                case CoreTypeID::ResourcePath:
                {
                    archive << *reinterpret_cast<ResourcePath*>( pValue );
                }
                break;

                case CoreTypeID::IntRange:
                {
                    archive << *reinterpret_cast<IntRange*>( pValue );
                }
                break;

                case CoreTypeID::FloatRange:
                {
                    archive << *reinterpret_cast<FloatRange*>( pValue );
                }
                break;

                case CoreTypeID::FloatCurve:
                {
                    archive << *reinterpret_cast<FloatCurve*>( pValue );
                }
                break;

                case CoreTypeID::ResourceTypeID:
                {
                    archive << *reinterpret_cast<ResourceTypeID*>( pValue );
                }
                break;

                case CoreTypeID::ResourcePtr:
                case CoreTypeID::TResourcePtr:
                {
                    archive << *reinterpret_cast<Resource::ResourcePtr*>( pValue );
                }
                break;

                case CoreTypeID::ResourceID:
                {
                    archive << *reinterpret_cast<ResourceID*>( pValue );
                }
                break;

                case CoreTypeID::BitFlags:
                case CoreTypeID::TBitFlags:
                {
                    archive << *reinterpret_cast<BitFlags*>( pValue );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                    return false;
                }
                break;
            }
        }

        return true;
    }

    bool ConvertStringToBinary( TypeRegistry const& typeRegistry, TypeID typeID, TypeID templateArgumentTypeID, String const& strValue, Blob& byteArray )
    {
        byteArray.clear();

        //-------------------------------------------------------------------------

        // Enums
        if ( !IsCoreType( typeID ) )
        {
            EnumInfo const* pEnumInfo = typeRegistry.GetEnumInfo( typeID );
            EE_ASSERT( pEnumInfo != nullptr );

            // We only support up to 32 bit enum types...
            switch ( pEnumInfo->m_underlyingType )
            {
                case CoreTypeID::Uint8:
                {
                    return ConvertToBinary<bool>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Int8:
                {
                    return ConvertToBinary<int8_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    return ConvertToBinary<uint16_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Int16:
                {
                    return ConvertToBinary<int16_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    return ConvertToBinary<uint32_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Int32:
                {
                    return ConvertToBinary<int32_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }
        else // Real core types
        {
            CoreTypeID const typeToConvert = GetCoreType( typeID );
            switch ( typeToConvert )
            {
                case CoreTypeID::Bool:
                {
                    return ConvertToBinary<bool>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Uint8:
                {
                    return ConvertToBinary<uint8_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Int8:
                {
                    return ConvertToBinary<int8_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Uint16:
                {
                    return ConvertToBinary<uint16_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Int16:
                {
                    return ConvertToBinary<int16_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Uint32:
                {
                    return ConvertToBinary<uint32_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Int32:
                {
                    return ConvertToBinary<int32_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Uint64:
                {
                    return ConvertToBinary<uint64_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Int64:
                {
                    return ConvertToBinary<int64_t>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Float:
                {
                    return ConvertToBinary<float>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Double:
                {
                    return ConvertToBinary<double>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::String:
                {
                    return ConvertToBinary<String>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::StringID:
                {
                    return ConvertToBinary<StringID>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Tag:
                {
                    return ConvertToBinary<Tag>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::TypeID:
                {
                    return ConvertToBinary<TypeID>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::UUID:
                {
                    return ConvertToBinary<UUID>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Color:
                {
                    return ConvertToBinary<Color>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Float2:
                {
                    return ConvertToBinary<Float2>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Float3:
                {
                    return ConvertToBinary<Float3>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Float4:
                {
                    return ConvertToBinary<Float4>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Vector:
                {
                    return ConvertToBinary<Vector>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Quaternion:
                {
                    return ConvertToBinary<Quaternion>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Matrix:
                {
                    return ConvertToBinary<Matrix>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Transform:
                {
                    return ConvertToBinary<Transform>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::EulerAngles:
                {
                    return ConvertToBinary<EulerAngles>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Microseconds:
                {
                    return ConvertToBinary<Microseconds>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Milliseconds:
                {
                    return ConvertToBinary<Milliseconds>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Seconds:
                {
                    return ConvertToBinary<Seconds>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Percentage:
                {
                    return ConvertToBinary<Percentage>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Degrees:
                {
                    return ConvertToBinary<Degrees>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::Radians:
                {
                    return ConvertToBinary<Radians>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::ResourcePath:
                {
                    return ConvertToBinary<ResourcePath>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::IntRange:
                {
                    return ConvertToBinary<IntRange>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::FloatRange:
                {
                    return ConvertToBinary<FloatRange>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::FloatCurve:
                {
                    return ConvertToBinary<FloatCurve>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::ResourceTypeID:
                {
                    return ConvertToBinary<ResourceTypeID>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::ResourcePtr:
                case CoreTypeID::TResourcePtr:
                {
                    return ConvertToBinary<Resource::ResourcePtr>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::ResourceID:
                {
                    return ConvertToBinary<ResourceID>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                case CoreTypeID::BitFlags:
                case CoreTypeID::TBitFlags:
                {
                    return ConvertToBinary<BitFlags>( typeRegistry, typeID, templateArgumentTypeID, strValue, byteArray );
                }
                break;

                default:
                {
                    EE_UNREACHABLE_CODE();
                }
                break;
            }
        }

        return false;
    }

    //-------------------------------------------------------------------------

    bool IsValidStringValueForType( TypeRegistry const& typeRegistry, TypeID typeID, TypeID templateArgumentTypeID, String const& strValue )
    {
        // Special cases: Enums and TBitFlags
        if ( typeID == CoreTypeID::TBitFlags )
        {
            EnumInfo const* pEnumInfo = typeRegistry.GetEnumInfo( templateArgumentTypeID );
            EE_ASSERT( pEnumInfo != nullptr );
            BitFlags flags;
            return ConvertStringToBitFlags( *pEnumInfo, strValue, flags );
        }
        else if ( !IsCoreType( typeID ) ) // Enums has unique typeIDs
        {
            auto const pEnumInfo = typeRegistry.GetEnumInfo( typeID );
            EE_ASSERT( pEnumInfo != nullptr );
            return pEnumInfo->IsValidValue( StringID( strValue ) );
        }
        else // Real core types
        {
            // TODO
            return true;
        }
    }
}