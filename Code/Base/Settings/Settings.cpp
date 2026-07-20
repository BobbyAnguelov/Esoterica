#include "Settings.h"
#include "IniFile.h"
#include "Base/TypeSystem/CoreTypeIDs.h"
#include "Base/TypeSystem/CoreTypeConversions.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

namespace EE
{
    static void GenerateValueName( char const* pSectionName, TypeSystem::PropertyInfo const& propertyInfo, InlineString& outName )
    {
        auto SanitizeStringForIni = [] ( InlineString& str )
        {
            StringUtils::ReplaceAllOccurrencesInPlace( str, " ", "_" );
        };

        // Append section
        EE_ASSERT( pSectionName != nullptr && strlen( pSectionName ) > 0 );
        outName = pSectionName;
        outName.append( ":" );

        // Append category
        InlineString tempStr = propertyInfo.m_metadata.GetValue( TypeSystem::PropertyMetadata::Category ).c_str();
        if ( !tempStr.empty() )
        {
            SanitizeStringForIni( tempStr );
            outName.append( tempStr );
            outName.append( ":" );
        }

        // Append name
        tempStr = propertyInfo.m_metadata.GetValue( TypeSystem::PropertyMetadata::FriendlyName ).c_str();
        SanitizeStringForIni( tempStr );
        outName.append( tempStr.c_str() );
    }

    //-------------------------------------------------------------------------

    bool Settings::IsSupportedSettingType( TypeSystem::PropertyInfo const& propertyInfo )
    {
        if ( propertyInfo.IsArrayProperty() || propertyInfo.IsBitFlagsProperty() || propertyInfo.IsStructureProperty() || propertyInfo.IsTypeInstanceProperty() )
        {
            return false;
        }

        if ( !IsCoreType( propertyInfo.m_typeID ) && !propertyInfo.IsEnumProperty() )
        {
            return false;
        }

        bool isSupportedType = false;
        TypeSystem::CoreTypeID const coreTypeID = TypeSystem::GetCoreType( propertyInfo.m_typeID );
        switch ( coreTypeID )
        {
            case TypeSystem::CoreTypeID::Bool:
            case TypeSystem::CoreTypeID::Int8:
            case TypeSystem::CoreTypeID::Int16:
            case TypeSystem::CoreTypeID::Int32:
            case TypeSystem::CoreTypeID::Uint8:
            case TypeSystem::CoreTypeID::Uint16:
            case TypeSystem::CoreTypeID::Uint32:
            case TypeSystem::CoreTypeID::Float:
            case TypeSystem::CoreTypeID::String:
            case TypeSystem::CoreTypeID::StringID:
            {
                isSupportedType = true;
            }
            break;

            default:
            {
                isSupportedType = false;
            }
        }

        return isSupportedType;
    }

    bool Settings::LoadSettings( TypeSystem::TypeRegistry const& typeRegistry, IniFile const& ini )
    {
        #if EE_DEVELOPMENT_TOOLS
        m_categorizedConsoleEditableProperties.Clear();
        TInlineVector<TypeSystem::PropertyInfo const*, 20> consoleEditableProperties;
        #endif

        InlineString nameStr;
        String valueStr;

        auto const pTypeInfo = GetTypeInfo();
        for ( auto const& propertyInfo : pTypeInfo->m_properties )
        {
            if ( !IsSupportedSettingType( propertyInfo ) )
            {
                EE_LOG_WARNING( LogCategory::System, "Save Global Settings", "Unsupported property type encountered for property '%s'", propertyInfo.m_ID.c_str() );
                continue;
            }

            // Add to editable properties
            //-------------------------------------------------------------------------

            #if EE_DEVELOPMENT_TOOLS
            consoleEditableProperties.emplace_back( &propertyInfo );
            #endif

            // Read property value
            //-------------------------------------------------------------------------

            GenerateValueName( GetSectionName(), propertyInfo, nameStr );
            valueStr = ini.GetInlineStringOrDefault( nameStr.c_str(), "" );
            if ( !valueStr.empty() )
            {
                auto pPropertyInstance = propertyInfo.GetPropertyAddress( this );
                if ( !TypeSystem::Conversion::ConvertStringToNativeType( typeRegistry, propertyInfo, valueStr.c_str(), pPropertyInstance ) )
                {
                    return false;
                }
            }
            else
            {
                pTypeInfo->ResetToDefault( this, propertyInfo.m_ID );
            }
        }

        // Categorize Properties
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        for ( auto pPropertyInfo : consoleEditableProperties )
        {
            String path = GetSectionName();
            String category = pPropertyInfo->m_metadata.GetValue( TypeSystem::PropertyMetadata::Category ).c_str();
            if ( !category.empty() )
            {
                path.append( "/" );
                path.append( category );
            }

            m_categorizedConsoleEditableProperties.AddItem( path, pPropertyInfo->m_metadata.GetValue( TypeSystem::PropertyMetadata::FriendlyName ).c_str(), pPropertyInfo );
        }
        #endif

        //-------------------------------------------------------------------------

        if ( !LoadCustomSettings( typeRegistry, ini ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        PostLoadOrModify();

        //-------------------------------------------------------------------------

        return true;
    }

    bool Settings::SaveSettings( TypeSystem::TypeRegistry const& typeRegistry, IniFile& ini ) const
    {
        ini.CreateSection( GetSectionName() );

        //-------------------------------------------------------------------------

        InlineString nameStr;
        String valueStr;

        auto const pTypeInfo = GetTypeInfo();
        for ( auto const& propertyInfo : pTypeInfo->m_properties )
        {
            if ( !IsSupportedSettingType( propertyInfo ) )
            {
                EE_LOG_WARNING( LogCategory::System, "Save Global Settings", "Unsupported property type encountered for property '%s'", propertyInfo.m_ID.c_str() );
                continue;
            }

            //-------------------------------------------------------------------------

            if ( pTypeInfo->IsPropertyValueSetToDefault( this, propertyInfo.m_ID.ToUint() ) )
            {
                continue;
            }

            // Save property value
            //-------------------------------------------------------------------------

            auto pPropertyInstance = propertyInfo.GetPropertyAddress( this );
            if ( TypeSystem::Conversion::ConvertNativeTypeToString( typeRegistry, propertyInfo, pPropertyInstance, valueStr ) )
            {
                GenerateValueName( GetSectionName(), propertyInfo, nameStr );
                ini.SetString( nameStr.c_str(), valueStr.c_str() );
            }
        }

        //-------------------------------------------------------------------------

        if ( !SaveCustomSettings( typeRegistry, ini ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        return true;
    }
}