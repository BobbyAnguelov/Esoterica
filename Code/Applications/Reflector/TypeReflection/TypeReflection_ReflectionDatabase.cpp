#include "TypeReflection_ReflectionDatabase.h"
#include "Base/TypeSystem/PropertyPath.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    ReflectionDatabase::ReflectionDatabase( TVector<ReflectedProject> const& projects )
    {
        // Copy relevant projects
        //-------------------------------------------------------------------------

        m_reflectedProjects.reserve( projects.size() );

        for ( ReflectedProject const& prj : projects )
        {
            if ( !prj.m_isExcludedFromTypeInfoReflection )
            {
                m_reflectedProjects.emplace_back( prj );
            }
        }

        // Create the base class for all registered engine types
        //-------------------------------------------------------------------------

        TInlineString<100> str;

        str.sprintf( "%s::%s", Settings::g_engineNamespace, ReflectedType::s_reflectedTypeInterfaceClassName );
        m_reflectedTypeBase = ReflectedType( TypeSystem::TypeID( str.c_str() ), ReflectedType::s_reflectedTypeInterfaceClassName );
        m_reflectedTypeBase.m_flags.SetFlag( ReflectedType::Flags::IsAbstract );

        str.sprintf( "%s::", Settings::g_engineNamespace );
        m_reflectedTypeBase.m_namespace = str.c_str();
    }

    //-------------------------------------------------------------------------

    bool ReflectionDatabase::IsProjectRegistered( StringID projectID ) const
    {
        for ( auto const& prj : m_reflectedProjects )
        {
            if ( prj.m_ID == projectID )
            {
                return true;
            }
        }

        return false;
    }

    ReflectedProject const* ReflectionDatabase::GetProjectDesc( StringID projectID ) const
    {
        for ( auto& prj : m_reflectedProjects )
        {
            if ( prj.m_ID == projectID )
            {
                return &prj;
            }
        }

        return nullptr;
    }

    bool ReflectionDatabase::IsHeaderRegistered( StringID headerID ) const
    {
        for ( auto const& prj : m_reflectedProjects )
        {
            for ( auto const& hdr : prj.m_headerFiles )
            {
                if ( hdr.m_ID == headerID )
                {
                    return true;
                }
            }
        }

        return false;
    }

    ReflectedHeader const* ReflectionDatabase::GetReflectedHeader( StringID headerID ) const
    {
        for ( auto const& prj : m_reflectedProjects )
        {
            for ( auto const& hdr : prj.m_headerFiles )
            {
                if ( hdr.m_ID == headerID )
                {
                    return &hdr;
                }
            }
        }

        return nullptr;
    }

    //-------------------------------------------------------------------------

    ReflectedType const* ReflectionDatabase::GetType( TypeSystem::TypeID typeID ) const
    {
        if ( m_reflectedTypeBase.m_ID == typeID )
        {
            return &m_reflectedTypeBase;
        }

        for ( auto const& type : m_reflectedTypes )
        {
            if ( type.m_ID == typeID )
            {
                return &type;
            }
        }

        return nullptr;
    }

    bool ReflectionDatabase::IsTypeRegistered( TypeSystem::TypeID typeID ) const
    {
        for ( auto const& type : m_reflectedTypes )
        {
            if ( type.m_ID == typeID )
            {
                return true;
            }
        }

        return false;
    }

    bool ReflectionDatabase::IsTypeDerivedFrom( TypeSystem::TypeID typeID, TypeSystem::TypeID parentTypeID ) const
    {
        auto pTypeDesc = GetType( typeID );
        EE_ASSERT( pTypeDesc != nullptr ); // Unknown Type
        EE_ASSERT( IsTypeRegistered( parentTypeID ) ); // Unknown Type

        // Check for same type
        if ( typeID == parentTypeID )
        {
            return true;
        }

        // Check for immediate parents
        if ( pTypeDesc->m_parentID == parentTypeID )
        {
            return true;
        }

        // Recursively check parents
        if ( IsTypeDerivedFrom( pTypeDesc->m_parentID, parentTypeID ) )
        {
            return true;
        }

        return false;
    }

    void ReflectionDatabase::GetAllTypesForHeader( StringID headerID, TVector<ReflectedType>& types ) const
    {
        for ( auto const& type : m_reflectedTypes )
        {
            if ( type.m_headerID == headerID )
            {
                types.push_back( type );
            }
        }
    }

    void ReflectionDatabase::GetAllTypesForProject( StringID projectID, TVector<ReflectedType>& types ) const
    {
        for ( auto const& prj : m_reflectedProjects )
        {
            for ( auto const& hdr : prj.m_headerFiles )
            {
                if ( hdr.m_projectID == projectID )
                {
                    GetAllTypesForHeader( hdr.m_ID, types );
                }
            }
        }
    }

    void ReflectionDatabase::RegisterType( ReflectedType const* pType )
    {
        EE_ASSERT( pType != nullptr && !IsTypeRegistered( pType->m_ID ) );
        m_reflectedTypes.emplace_back( *pType );
    }

    void ReflectionDatabase::UpdateRegisteredType( ReflectedType const* pType )
    {
        auto pReflectedType = GetType( pType->m_ID );
        EE_ASSERT( pReflectedType != nullptr );
        pReflectedType->m_isDevOnly = false;

        for ( auto const& property : pType->m_properties )
        {
            auto foundIter = VectorFind( pReflectedType->m_properties, property );
            if ( foundIter != pReflectedType->m_properties.end() )
            {
                foundIter->m_isDevOnly = false;
            }
        }
    }

    ReflectedProperty const* ReflectionDatabase::GetPropertyInfo( TypeSystem::TypeID typeID, TypeSystem::PropertyPath const& pathID ) const
    {
        ReflectedProperty const* pResolvedPropertyTypeDesc = nullptr;

        ReflectedType const* pCurrentTypeDesc = GetType( typeID );
        if ( pCurrentTypeDesc == nullptr )
        {
            return pResolvedPropertyTypeDesc;
        }

        // For each path element, get the type desc for that type
        size_t const numPathElements = pathID.GetNumElements();
        size_t const lastElementIdx = numPathElements - 1;
        for ( size_t i = 0; i < numPathElements; i++ )
        {
            pResolvedPropertyTypeDesc = pCurrentTypeDesc->GetPropertyDescriptor( pathID[i].m_ID );
            if ( pResolvedPropertyTypeDesc == nullptr )
            {
                break;
            }

            if ( i != lastElementIdx )
            {
                // Get the type desc of the property
                EE_ASSERT( !IsCoreType( pResolvedPropertyTypeDesc->m_typeID ) && !pResolvedPropertyTypeDesc->IsEnumProperty() );
                pCurrentTypeDesc = GetType( pResolvedPropertyTypeDesc->m_typeID );
                if ( pCurrentTypeDesc == nullptr )
                {
                    EE_LOG_ERROR( LogCategory::TypeSystem, "ReflectionDatabase", "Cant resolve property path since it contains an unknown type" );
                    return nullptr;
                }
            }
        }

        return pResolvedPropertyTypeDesc;
    }

    bool ReflectionDatabase::ProcessAndValidateReflectedProperties()
    {
        InlineString error;

        for ( auto& type : m_reflectedTypes )
        {
            for ( auto& property : type.m_properties )
            {
                Log log;
                property.GenerateMetaData( &log );

                if ( log.HasWarnings() )
                {
                    InlineString warningStr;
                    for ( auto const& entry : log.GetLogEntries() )
                    {
                        if ( entry.m_severity == Severity::Warning )
                        {
                            warningStr.append_sprintf( "%s\n", entry.m_message.c_str() );
                        }
                    }

                    LogWarning( "Metadata Warnings for property '%s' in type '%s%s':\n%s\n", property.m_name.c_str(), type.m_namespace.c_str(), type.m_name.c_str(), warningStr.c_str() );
                }

                if ( property.m_typeID == TypeSystem::GetCoreTypeID<TDataFilePath>() && !property.m_isDevOnly )
                {
                    LogError( "Runtime data file path property detected: '%s' in type: '%s%s'. Data files are a tools only concept!", property.m_name.c_str(), type.m_namespace.c_str(), type.m_name.c_str() );
                    return false;
                }
            }
        }

        return true;
    }

    //-------------------------------------------------------------------------

    ReflectedResourceType const* ReflectionDatabase::GetResourceType( ResourceTypeID typeID ) const
    {
        for ( auto const& type : m_reflectedResourceTypes )
        {
            if ( type.m_resourceTypeID == typeID )
            {
                return &type;
            }
        }

        return nullptr;
    }

    bool ReflectionDatabase::IsResourceRegistered( ResourceTypeID typeID ) const
    {
        for ( auto const& type : m_reflectedResourceTypes )
        {
            if ( type.m_resourceTypeID == typeID )
            {
                return true;
            }
        }

        return false;
    }

    bool ReflectionDatabase::IsResourceRegistered( TypeSystem::TypeID typeID ) const
    {
        for ( auto const& type : m_reflectedResourceTypes )
        {
            if ( type.m_typeID == typeID )
            {
                return true;
            }
        }

        return false;
    }

    void ReflectionDatabase::RegisterResource( ReflectedResourceType const* pResource )
    {
        EE_ASSERT( pResource != nullptr && !IsResourceRegistered( pResource->m_resourceTypeID ) );
        m_reflectedResourceTypes.push_back( *pResource );
    }

    void ReflectionDatabase::UpdateRegisteredResource( ReflectedResourceType const* pResource )
    {
        auto pReflectedResource = GetResourceType( pResource->m_resourceTypeID );
        EE_ASSERT( pReflectedResource != nullptr );
        pReflectedResource->m_isDevOnly = false;
    }

    void ReflectionDatabase::CleanupResourceHierarchy()
    {
        static TypeSystem::TypeID const baseResourceTypeID( ReflectedResourceType::s_baseResourceFullTypeName );

        for ( auto& resourceType : m_reflectedResourceTypes )
        {
            for ( int32_t i = (int32_t) resourceType.m_parentTypes.size() - 1; i >= 0; i-- )
            {
                if ( resourceType.m_parentTypes[i] == baseResourceTypeID || !IsResourceRegistered( resourceType.m_parentTypes[i] ) )
                {
                    resourceType.m_parentTypes.erase( resourceType.m_parentTypes.begin() + i );
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    ReflectedDataFileType const* ReflectionDatabase::GetDataFileType( TypeSystem::TypeID typeID ) const
    {
        for ( auto const& type : m_reflectedDataFileTypes )
        {
            if ( type.m_typeID == typeID )
            {
                return &type;
            }
        }

        return nullptr;
    }

    bool ReflectionDatabase::IsDataFileRegistered( TypeSystem::TypeID typeID ) const
    {
        for ( auto const& type : m_reflectedDataFileTypes )
        {
            if ( type.m_typeID == typeID )
            {
                return true;
            }
        }

        return false;
    }

    void ReflectionDatabase::RegisterDataFile( ReflectedDataFileType const* pDataFile )
    {
        EE_ASSERT( pDataFile != nullptr && !IsDataFileRegistered( pDataFile->m_typeID ) );
        m_reflectedDataFileTypes.push_back( *pDataFile );
    }

    void ReflectionDatabase::UpdateRegisteredDataFile( ReflectedDataFileType const* pDataFile )
    {
        auto pReflectedDataFile = GetDataFileType( pDataFile->m_typeID );
        EE_ASSERT( pReflectedDataFile != nullptr );
        pReflectedDataFile->m_isDevOnly = false;
    }

    bool ReflectionDatabase::ValidateDataFileRegistrations()
    {
        for ( auto& type : m_reflectedDataFileTypes )
        {
            if ( !type.m_isDevOnly )
            {
                LogError( "Runtime data file declaration detected (%s)! Data files are a tools only concept and can only be declared in tools modules!", type.m_typeID.c_str() );
                return false;
            }
        }

        return true;
    }
}