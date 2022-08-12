#include "ReflectionDatabase.h"
#include "Applications/Reflector/ReflectorSettingsAndUtils.h"
#include "System/FileSystem/FileSystem.h"
#include "System/TypeSystem/TypeRegistry.h"
#include "System/Log.h"
#include <eastl/sort.h>
#include <sqlite3.h>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    ReflectionDatabase::ReflectionDatabase()
    {
        // Create the base class for all registered engine types
        //-------------------------------------------------------------------------

        TInlineString<100> str;

        str.sprintf( "%s::%s", Settings::g_engineNamespace, Settings::g_registeredTypeInterfaceClassName );
        m_registeredTypeBase = ReflectedType( TypeID( str.c_str() ), Settings::g_registeredTypeInterfaceClassName );
        m_registeredTypeBase.m_flags.SetFlag( ReflectedType::Flags::IsAbstract );

        str.sprintf( "%s::", Settings::g_engineNamespace );
        m_registeredTypeBase.m_namespace = str.c_str();
    }

    ReflectionDatabase::~ReflectionDatabase()
    {
        if ( IsConnected() )
        {
            Disconnect();
        }
    }

    //-------------------------------------------------------------------------

    bool ReflectionDatabase::Connect( FileSystem::Path const& databasePath, bool readOnlyAccess, bool useMutex )
    {
        EE_ASSERT( databasePath.IsFilePath() );

        databasePath.EnsureDirectoryExists();

        int32_t sqlFlags = 0;

        if ( readOnlyAccess )
        {
            sqlFlags = SQLITE_OPEN_READONLY;
        }
        else
        {
            sqlFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_WAL;
        }

        if ( useMutex )
        {
            sqlFlags |= SQLITE_OPEN_FULLMUTEX;
        }

        //-------------------------------------------------------------------------

        auto const result = sqlite3_open_v2( databasePath, &m_pDatabase, sqlFlags, nullptr );
        sqlite3_busy_timeout( m_pDatabase, 2500 );

        if ( result != SQLITE_OK )
        {
            sqlite3_close( m_pDatabase );
            m_pDatabase = nullptr;
            m_errorMessage.sprintf( "Couldn't open sqlite database: %s", databasePath.c_str() );
            return false;
        }

        return true;
    }

    bool ReflectionDatabase::Disconnect()
    {
        if ( m_pDatabase != nullptr )
        {
            // Close connection
            auto result = sqlite3_close( m_pDatabase );
            EE_ASSERT( result == SQLITE_OK ); // If we get SQLITE_BUSY, this means we are leaking sqlite resources
            m_pDatabase = nullptr;
            return IsValidSQLiteResult( result );
        }

        return true;
    }

    bool ReflectionDatabase::IsValidSQLiteResult( int result, char const* pErrorMessage ) const
    {
        if ( result != SQLITE_OK )
        {
            m_errorMessage.sprintf( "%s ( %s )", sqlite3_errstr(result), sqlite3_errmsg(m_pDatabase) );
            return false;
        }

        return true;
    }

    void ReflectionDatabase::FillStatementBuffer( char const* pFormat, ... ) const
    {
        EE_ASSERT( IsConnected() );

        // Create the statement using the sqlite printf so we can use the extra format specifiers i.e. %Q
        va_list args;
        va_start( args, pFormat );
        sqlite3_vsnprintf( s_defaultStatementBufferSize, m_statementBuffer, pFormat, args );
        va_end( args );
    }

    bool ReflectionDatabase::ExecuteSimpleQuery( char const* pFormat, ... ) const
    {
        EE_ASSERT( IsConnected() );

        // Create the statement using the sqlite printf so we can use the extra format specifiers i.e. %Q
        va_list args;
        va_start( args, pFormat );
        sqlite3_vsnprintf( s_defaultStatementBufferSize, m_statementBuffer, pFormat, args );
        va_end( args );

        return IsValidSQLiteResult( sqlite3_exec( m_pDatabase, m_statementBuffer, nullptr, nullptr, nullptr ) );
    }

    bool ReflectionDatabase::BeginTransaction() const
    {
        EE_ASSERT( IsConnected() );
        return IsValidSQLiteResult( sqlite3_exec( m_pDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr ) );
    }

    bool ReflectionDatabase::EndTransaction() const
    {
        EE_ASSERT( IsConnected() );
        return IsValidSQLiteResult( sqlite3_exec( m_pDatabase, "END TRANSACTION;", nullptr, nullptr, nullptr ) );
    }

    //-------------------------------------------------------------------------

    bool ReflectionDatabase::IsProjectRegistered( ProjectID projectID ) const
    {
        for ( auto const& prj : m_reflectedProjects )
        {
            if ( prj.m_ID == projectID )
            {
                return true;
            }
        }

        return nullptr;
    }

    ProjectInfo const* ReflectionDatabase::GetProjectDesc( ProjectID projectID ) const
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

    void ReflectionDatabase::UpdateProjectList( TVector<ProjectInfo> const& registeredProjects )
    {
        DeleteObseleteProjects( registeredProjects );

        for ( auto& prj : registeredProjects )
        {
            auto const prjID = prj.m_ID;
            auto existingProj = eastl::find_if( m_reflectedProjects.begin(), m_reflectedProjects.end(), [prjID] ( ProjectInfo const& desc )->bool { return desc.m_ID == prjID; } );
            if ( existingProj == m_reflectedProjects.end() )
            {
                m_reflectedProjects.push_back( prj );
            }
            else
            {
                *existingProj = prj;
            }
        }
    }

    bool ReflectionDatabase::IsHeaderRegistered( HeaderID headerID ) const
    {
        for ( auto const& hdr : m_reflectedHeaders )
        {
            if ( hdr.m_ID == headerID )
            {
                return true;
            }
        }

        return false;
    }

    HeaderInfo const* ReflectionDatabase::GetHeaderDesc( HeaderID headerID ) const
    {
        for ( auto& hdr : m_reflectedHeaders )
        {
            if ( hdr.m_ID == headerID )
            {
                return &hdr;
            }
        }

        return nullptr;
    }

    void ReflectionDatabase::UpdateHeaderRecord( HeaderInfo const& header )
    {
        HeaderInfo* pHdr = const_cast<HeaderInfo*>( GetHeaderDesc( header.m_ID ) );
        if ( pHdr != nullptr )
        {
            *pHdr = header;
        }
        else
        {
            m_reflectedHeaders.push_back( header );
        }
    }

    //-------------------------------------------------------------------------

    ReflectedType const* ReflectionDatabase::GetType( TypeID typeID ) const
    {
        if ( m_registeredTypeBase.m_ID == typeID )
        {
            return &m_registeredTypeBase;
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

    bool ReflectionDatabase::IsTypeRegistered( TypeID typeID ) const
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

    bool ReflectionDatabase::IsTypeDerivedFrom( TypeID typeID, TypeID parentTypeID ) const
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
        for ( auto parentTypeIDToCheck : pTypeDesc->m_parents )
        {
            if ( parentTypeIDToCheck == parentTypeID )
            {
                return true;
            }
        }

        // Recursively check parents
        for ( auto parentTypeIDToCheck : pTypeDesc->m_parents )
        {
            if ( IsTypeDerivedFrom( parentTypeIDToCheck, parentTypeID ) )
            {
                return true;
            }
        }

        return false;
    }

    void ReflectionDatabase::GetAllTypesForHeader( HeaderID headerID, TVector<ReflectedType>& types ) const
    {
        for ( auto const& type : m_reflectedTypes )
        {
            if ( type.m_headerID == headerID )
            {
                types.push_back( type );
            }
        }
    }

    void ReflectionDatabase::GetAllTypesForProject( ProjectID projectID, TVector<ReflectedType>& types ) const
    {
        for ( auto const& hdr : m_reflectedHeaders )
        {
            if ( hdr.m_projectID == projectID )
            {
                GetAllTypesForHeader( hdr.m_ID, types );
            }
        }
    }

    void ReflectionDatabase::RegisterType( ReflectedType const* pType, bool onlyUpdateDevFlag )
    {
        if ( onlyUpdateDevFlag )
        {
            auto pRegisteredType = GetType( pType->m_ID );
            EE_ASSERT( pRegisteredType != nullptr );
            pRegisteredType->m_isDevOnly = false;

            for ( auto const& property : pType->m_properties )
            {
                auto foundIter = VectorFind( pRegisteredType->m_properties, property );
                if ( foundIter != pRegisteredType->m_properties.end() )
                {
                    foundIter->m_isDevOnly = false;
                }
            }
        }
        else
        {
            EE_ASSERT( pType != nullptr && !IsTypeRegistered( pType->m_ID ) );
            m_reflectedTypes.push_back( *pType );
        }
    }

    ReflectedProperty const* ReflectionDatabase::GetPropertyTypeDescriptor( TypeID typeID, PropertyPath const& pathID ) const
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
            pResolvedPropertyTypeDesc = pCurrentTypeDesc->GetPropertyDescriptor( pathID[i].m_propertyID );
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
                    EE_LOG_ERROR( "Type System", "ReflectionDatabase", "Cant resolve property path since it contains an unknown type" );
                    return nullptr;
                }
            }
        }

        return pResolvedPropertyTypeDesc;
    }

    //-------------------------------------------------------------------------

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

    void ReflectionDatabase::RegisterResource( ReflectedResourceType const* pResource )
    {
        EE_ASSERT( pResource != nullptr && !IsResourceRegistered( pResource->m_resourceTypeID ) );
        m_reflectedResourceTypes.push_back( *pResource );
    }

    //-------------------------------------------------------------------------

    void ReflectionDatabase::DeleteTypesForHeader( HeaderID headerID )
    {
        for ( auto j = (int32_t) m_reflectedTypes.size() - 1; j >= 0; j-- )
        {
            if ( m_reflectedTypes[j].m_headerID == headerID )
            {
                m_reflectedTypes.erase_unsorted( m_reflectedTypes.begin() + j );
            }
        }

        for ( auto j = (int32_t) m_reflectedResourceTypes.size() - 1; j >= 0; j-- )
        {
            if ( m_reflectedResourceTypes[j].m_headerID == headerID )
            {
                m_reflectedResourceTypes.erase_unsorted( m_reflectedResourceTypes.begin() + j );
            }
        }
    }

    void ReflectionDatabase::DeleteObseleteHeadersAndTypes( TVector<HeaderID> const& registeredHeaders )
    {
        for ( auto i = (int32_t) m_reflectedHeaders.size() - 1; i >= 0; i-- )
        {
            auto const hdrID = m_reflectedHeaders[i].m_ID;
            if ( eastl::find( registeredHeaders.begin(), registeredHeaders.end(), hdrID ) == registeredHeaders.end() )
            {
                DeleteTypesForHeader( hdrID );
                m_reflectedHeaders.erase_unsorted( m_reflectedHeaders.begin() + i );
            }
        }
    }

    void ReflectionDatabase::DeleteObseleteProjects( TVector<ProjectInfo> const& registeredProjects )
    {
        for ( auto i = (int32_t) m_reflectedProjects.size() - 1; i >= 0; i-- )
        {
            auto const prjID = m_reflectedProjects[i].m_ID;
            if ( eastl::find_if( registeredProjects.begin(), registeredProjects.end(), [prjID] ( ProjectInfo const& desc )->bool { return desc.m_ID == prjID; } ) == registeredProjects.end() )
            {
                m_reflectedProjects.erase_unsorted( m_reflectedProjects.begin() + i );
            }
        }
    }

    //-------------------------------------------------------------------------

    bool ReflectionDatabase::ReadDatabase( FileSystem::Path const& databasePath )
    {
        if ( !Connect( databasePath, false ) )
        {
            return false;
        }

        if ( !CreateTables() )
        {
            return false;
        }

        m_reflectedProjects.clear();
        m_reflectedHeaders.clear();
        m_reflectedTypes.clear();

        //-------------------------------------------------------------------------
        // Read all projects

        sqlite3_stmt* pStatement = nullptr;
        FillStatementBuffer( "SELECT * FROM `Modules` ORDER BY `DependencyCount` ASC;" );
        if ( IsValidSQLiteResult( sqlite3_prepare_v2( m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr ) ) )
        {
            while ( sqlite3_step( pStatement ) == SQLITE_ROW )
            {
                ProjectInfo project;
                project.m_ID = StringID( sqlite3_column_int( pStatement, 0 ) );
                project.m_name = (char const*) sqlite3_column_text( pStatement, 1 );
                project.m_path = (char const*) sqlite3_column_text( pStatement, 2 );
                project.m_exportMacro = (char const*) sqlite3_column_text( pStatement, 3 );
                project.m_moduleClassName = (char const*) sqlite3_column_text( pStatement, 4 );
                project.m_moduleHeaderID = StringID( sqlite3_column_int( pStatement, 5 ) );
                project.m_dependencyCount = sqlite3_column_int( pStatement, 6 );
                m_reflectedProjects.push_back( project );
            }

            if ( !IsValidSQLiteResult( sqlite3_finalize( pStatement ) ) )
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        //-------------------------------------------------------------------------
        // Read all headers

        FillStatementBuffer( "SELECT * FROM `HeaderFiles`;" );
        if ( IsValidSQLiteResult( sqlite3_prepare_v2( m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr ) ) )
        {
            while ( sqlite3_step( pStatement ) == SQLITE_ROW )
            {
                HeaderInfo header;
                header.m_ID = StringID( sqlite3_column_int( pStatement, 0 ) );
                header.m_projectID = StringID( sqlite3_column_int( pStatement, 1 ) );
                header.m_filePath = (char const*) sqlite3_column_text( pStatement, 2 );
                header.m_timestamp = sqlite3_column_int64( pStatement, 3 );
                header.m_checksum = sqlite3_column_int64( pStatement, 4 );
                m_reflectedHeaders.push_back( header );
            }

            if ( !IsValidSQLiteResult( sqlite3_finalize( pStatement ) ) )
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        //-------------------------------------------------------------------------
        // Read all types

        FillStatementBuffer( "SELECT * FROM `Types`;" );
        if ( IsValidSQLiteResult( sqlite3_prepare_v2( m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr ) ) )
        {
            while ( sqlite3_step( pStatement ) == SQLITE_ROW )
            {
                ReflectedType type;
                type.m_ID = sqlite3_column_int( pStatement, 0 );
                type.m_headerID = StringID( sqlite3_column_int( pStatement, 1 ) );
                type.m_name = (char const*) sqlite3_column_text( pStatement, 2 );
                type.m_namespace = (char const*) sqlite3_column_text( pStatement, 3 );
                type.m_flags.Set( (uint32_t) sqlite3_column_int( pStatement, 4 ) );

                // Read additional type data
                if ( type.IsEnum() )
                {
                    if ( !ReadAdditionalEnumData( type ) )
                    {
                        return false;
                    }
                }
                else
                {
                    if ( !ReadAdditionalTypeData( type ) )
                    {
                        return false;
                    }
                }

                // Add type to list
                m_reflectedTypes.push_back( type );
            }

            if ( !IsValidSQLiteResult( sqlite3_finalize( pStatement ) ) )
            {
                return false;
            }
            pStatement = nullptr;
        }
        else
        {
            return false;
        }

        //-------------------------------------------------------------------------
        // Read all resource types

        FillStatementBuffer( "SELECT * FROM `ResourceTypes`;" );
        if ( IsValidSQLiteResult( sqlite3_prepare_v2( m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr ) ) )
        {
            while ( sqlite3_step( pStatement ) == SQLITE_ROW )
            {
                ReflectedResourceType header;
                header.m_typeID = TypeID( (char const*) sqlite3_column_text( pStatement, 0 ) );
                header.m_resourceTypeID = sqlite3_column_int( pStatement, 1 );
                header.m_friendlyName = (char const*) sqlite3_column_text( pStatement, 2 );
                header.m_headerID = StringID( sqlite3_column_int( pStatement, 3 ) );
                header.m_className = (char const*) sqlite3_column_text( pStatement, 4 );
                header.m_namespace = (char const*) sqlite3_column_text( pStatement, 5 );
                header.m_isVirtual = sqlite3_column_int( pStatement, 6 ) != 0;
                m_reflectedResourceTypes.push_back( header );
            }
            if ( !IsValidSQLiteResult( sqlite3_finalize( pStatement ) ) )
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        return Disconnect();
    }

    bool ReflectionDatabase::WriteDatabase( FileSystem::Path const& databasePath )
    {
        EE_ASSERT( databasePath.IsFilePath() );

        char* pErrorMessage = nullptr;
        char statement[s_defaultStatementBufferSize] = { 0 };
        int result = 0;

        databasePath.EnsureDirectoryExists();
        if ( !Connect( databasePath, false ) )
        {
            return false;
        }

        BeginTransaction();

        //-------------------------------------------------------------------------

        if ( !DropTables() )
        {
            return false;
        }

        if ( !CreateTables() )
        {
            return false;
        }

        // Write all projects
        //-------------------------------------------------------------------------

        for ( auto const& project : m_reflectedProjects )
        {
            if ( !ExecuteSimpleQuery( "INSERT OR REPLACE INTO `Modules`(`ModuleID`, `Name`, `Path`, `ExportMacro`, `ModuleClassName`, `ModuleHeaderID`, `DependencyCount`) VALUES ( %u, \"%s\", \"%s\",\"%s\",\"%s\", %u, %u);", (uint32_t) project.m_ID, project.m_name.c_str(), project.m_path.c_str(), project.m_exportMacro.c_str(), project.m_moduleClassName.c_str(), (uint32_t) project.m_moduleHeaderID, project.m_dependencyCount ) )
            {
                return false;
            }
        }

        // Write all headers
        //-------------------------------------------------------------------------

        for ( auto const& header : m_reflectedHeaders )
        {
            if ( !ExecuteSimpleQuery( "INSERT OR REPLACE INTO `HeaderFiles`(`HeaderID`,`ModuleID`,`FilePath`,`TimeStamp`,`Checksum`) VALUES ( %u, %u, \"%s\",%llu,%llu);", (uint32_t) header.m_ID, (uint32_t) header.m_projectID, header.m_filePath.c_str(), header.m_timestamp, header.m_checksum ) )
            {
                return false;
            }
        }

        // Write all types
        //-------------------------------------------------------------------------

        for ( auto const& type : m_reflectedTypes )
        {
            if ( !ExecuteSimpleQuery( "INSERT OR REPLACE INTO `Types`(`TypeID`, `HeaderID`,`Name`,`Namespace`,`TypeFlags`) VALUES ( %u, %u, \"%s\", \"%s\", %u );", (uint32_t) type.m_ID, (uint32_t) type.m_headerID, type.m_name.c_str(), type.m_namespace.c_str(), (uint32_t) type.m_flags ) )
            {
                return false;
            }

            if ( type.IsEnum() )
            {
                if ( !WriteAdditionalEnumData( type ) )
                {
                    return false;
                }
            }
            else
            {
                if ( !WriteAdditionalTypeData( type ) )
                {
                    return false;
                }
            }
        }

        // Write all resources types
        //-------------------------------------------------------------------------

        for ( auto const& resourceType : m_reflectedResourceTypes )
        {
            if ( !ExecuteSimpleQuery( "INSERT OR REPLACE INTO `ResourceTypes`( `TypeID`, `ResourceTypeID`, `FriendlyName`, `HeaderID`,`ClassName`,`Namespace`,`IsVirtual`) VALUES ( \"%s\", %u, \"%s\", %u, \"%s\",\"%s\",%d);", resourceType.m_typeID.c_str(), (uint32_t) resourceType.m_resourceTypeID, resourceType.m_friendlyName.c_str(), (uint32_t) resourceType.m_headerID, resourceType.m_className.c_str(), resourceType.m_namespace.c_str(), resourceType.m_isVirtual ? 1 : 0 ) )
            {
                return false;
            }
        }

        // Update database info
        //-------------------------------------------------------------------------

        if ( !ExecuteSimpleQuery( "DELETE FROM `DatabaseInfo`; INSERT OR REPLACE INTO `DatabaseInfo`(`LastUpdated` ) VALUES( CURRENT_TIMESTAMP );" ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        EndTransaction();

        return Disconnect();
    }

    bool ReflectionDatabase::CreateTables()
    {
        EE_ASSERT( m_pDatabase != nullptr );

        // Project / Header tables
        //-------------------------------------------------------------------------

        if ( !ExecuteSimpleQuery( "CREATE TABLE IF NOT EXISTS `Modules` ( `ModuleID` INTEGER UNIQUE, `Name` TEXT UNIQUE, `Path` TEXT, `ExportMacro` TEXT, `ModuleClassName` TEXT NOT NULL, `ModuleHeaderID` INTEGER UNIQUE NOT NULL, `DependencyCount` INTEGER, PRIMARY KEY( `ModuleID` ) );" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "CREATE TABLE IF NOT EXISTS `HeaderFiles` ( `HeaderID` INTEGER NOT NULL UNIQUE, `ModuleID` INTEGER NOT NULL, `FilePath` TEXT NOT NULL, `TimeStamp` INTEGER NOT NULL, `Checksum` INTEGER NOT NULL, PRIMARY KEY( `HeaderID` ) );" ) )
        {
            return false;
        }

        // Type registration tables
        //-------------------------------------------------------------------------

        if ( !ExecuteSimpleQuery( "CREATE TABLE IF NOT EXISTS `Types` ( `TypeID` INTEGER UNIQUE, `HeaderID` INTEGER, `Name` TEXT, `Namespace` TEXT, `TypeFlags` INTEGER, PRIMARY KEY( `TypeID` ) );" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "CREATE TABLE IF NOT EXISTS `Properties` ( `PropertyID` INTEGER, `LineNumber` INTEGER, `OwnerTypeID` INTEGER, `TypeID` INTEGER, `Name` TEXT, `Description` TEXT, `TypeName` TEXT, `TemplateTypeName` TEXT, `PropertyFlags` INTEGER, `ArraySize` INTEGER DEFAULT -1, PRIMARY KEY( PropertyID, OwnerTypeID ) );" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "CREATE TABLE IF NOT EXISTS `TypeParents` ( `TypeID` INTEGER, `ParentTypeID` INTEGER, PRIMARY KEY( TypeID, ParentTypeID ) );" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "CREATE TABLE IF NOT EXISTS `EnumConstants` ( `TypeID` INTEGER NOT NULL, `Label` TEXT NOT NULL, `Value` INTEGER, `Description` TEXT, PRIMARY KEY(TypeID, Label) );" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "CREATE TABLE IF NOT EXISTS `ResourceTypes` ( `TypeID` TEXT, `ResourceTypeID` INTEGER, `FriendlyName` TEXT, `HeaderID` INTEGER, `ClassName` TEXT, `Namespace` TEXT, `IsVirtual` INTEGER, PRIMARY KEY( `ResourceTypeID`) );" ) )
        {
            return false;
        }

        // Database info table
        //-------------------------------------------------------------------------

        if ( !ExecuteSimpleQuery( "CREATE TABLE IF NOT EXISTS `DatabaseInfo` ( `LastUpdated` NUMERIC, PRIMARY KEY( `LastUpdated`) );" ) )
        {
            return false;
        }

        return true;
    }

    bool ReflectionDatabase::DropTables()
    {
        EE_ASSERT( m_pDatabase != nullptr );
        char* pErrorMessage = nullptr;

        if ( !ExecuteSimpleQuery( "DROP TABLE IF EXISTS `Modules`;" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "DROP TABLE IF EXISTS `HeaderFiles`;" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "DROP TABLE IF EXISTS `Types`;" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "DROP TABLE IF EXISTS `Properties`;" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "DROP TABLE IF EXISTS `TypeParents`;" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "DROP TABLE IF EXISTS `EnumConstants`;" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "DROP TABLE IF EXISTS `ResourceTypes`;" ) )
        {
            return false;
        }

        if ( !ExecuteSimpleQuery( "DROP TABLE IF EXISTS `DatabaseInfo`;" ) )
        {
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool ReflectionDatabase::ReadAdditionalTypeData( ReflectedType& type )
    {
        EE_ASSERT( type.m_ID.IsValid() && !type.IsEnum() );

        sqlite3_stmt* pStatement = nullptr;

        // Get all parent types
        //-------------------------------------------------------------------------

        FillStatementBuffer( "SELECT `TypeParents`.ParentTypeID FROM `TypeParents` INNER JOIN `Types` ON `Types`.TypeID = `TypeParents`.ParentTypeID WHERE `TypeParents`.TypeID = %u;", (uint32_t) type.m_ID );
        if ( IsValidSQLiteResult( sqlite3_prepare_v2( m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr ) ) )
        {
            while ( sqlite3_step( pStatement ) == SQLITE_ROW )
            {
                type.m_parents.push_back( sqlite3_column_int( pStatement, 0 ) );
            }

            if ( !IsValidSQLiteResult( sqlite3_finalize( pStatement ) ) )
            {
                return false;
            }
            pStatement = nullptr;
        }
        else
        {
            return false;
        }

        // Get all properties
        //-------------------------------------------------------------------------

        FillStatementBuffer( "SELECT * FROM `Properties` WHERE `OwnerTypeID` = %u;", (uint32_t) type.m_ID );
        if ( IsValidSQLiteResult( sqlite3_prepare_v2( m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr ) ) )
        {
            char arrayQueryStatement[s_defaultStatementBufferSize];
            sqlite3_stmt* pArrayQueryStatement = nullptr;

            while ( sqlite3_step( pStatement ) == SQLITE_ROW )
            {
                ReflectedProperty propDesc;
                propDesc.m_lineNumber = sqlite3_column_int( pStatement, 1 );
                propDesc.m_typeID = sqlite3_column_int( pStatement, 3 );
                propDesc.m_name = (char const*) sqlite3_column_text( pStatement, 4 );
                propDesc.m_description = (char const*) sqlite3_column_text( pStatement, 5 );
                propDesc.m_typeName = (char const*) sqlite3_column_text( pStatement, 6 );
                propDesc.m_templateArgTypeName = (char const*) sqlite3_column_text( pStatement, 7 );
                propDesc.m_flags.Set( (uint32_t) sqlite3_column_int( pStatement, 8 ) );
                propDesc.m_arraySize = sqlite3_column_int( pStatement, 9 );
                propDesc.m_propertyID = StringID( propDesc.m_name );
                EE_ASSERT( propDesc.m_propertyID == (uint32_t) sqlite3_column_int( pStatement, 0 ) ); // Ensure the property ID matches the recorded one
                type.m_properties.push_back( propDesc );
            }

            if ( !IsValidSQLiteResult( sqlite3_finalize( pStatement ) ) )
            {
                return false;
            }

            //-------------------------------------------------------------------------

            eastl::sort( type.m_properties.begin(), type.m_properties.end(), [] ( ReflectedProperty const& a, ReflectedProperty const& b ) { return a.m_lineNumber < b.m_lineNumber; } );

            //-------------------------------------------------------------------------

            pStatement = nullptr;
            return true;
        }

        return false;
    }

    bool ReflectionDatabase::ReadAdditionalEnumData( ReflectedType& type )
    {
        EE_ASSERT( type.m_ID.IsValid() && type.IsEnum() );

        sqlite3_stmt* pStatement = nullptr;
        FillStatementBuffer( "SELECT `Label`, `Value`, `Description` FROM `EnumConstants` WHERE `EnumConstants`.TypeID = %u;", (uint32_t) type.m_ID );
        if ( IsValidSQLiteResult( sqlite3_prepare_v2( m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr ) ) )
        {
            while ( sqlite3_step( pStatement ) == SQLITE_ROW )
            {
                ReflectedEnumConstant constantDesc;
                constantDesc.m_label = (char const*) sqlite3_column_text( pStatement, 0 );
                constantDesc.m_value = sqlite3_column_int( pStatement, 1 );
                constantDesc.m_description = (char const*) sqlite3_column_text( pStatement, 2 );
                type.AddEnumConstant( constantDesc );
            }

            if ( !IsValidSQLiteResult( sqlite3_finalize( pStatement ) ) )
            {
                return false;
            }

            pStatement = nullptr;
            return true;
        }

        return false;
    }

    bool ReflectionDatabase::WriteAdditionalTypeData( ReflectedType const& type )
    {
        // Update Type Parents
        for ( auto& parent : type.m_parents )
        {
            // Delete old parents
            if ( !ExecuteSimpleQuery( "DELETE FROM `TypeParents` WHERE `TypeID` = %u;", (uint32_t) type.m_ID ) )
            {
                return false;
            }

            if ( !ExecuteSimpleQuery( "INSERT INTO `TypeParents`(`TypeID`, `ParentTypeID`) VALUES ( %u, %u );", (uint32_t) type.m_ID, (uint32_t) parent ) )
            {
                return false;
            }
        }

        // Delete old properties
        if ( !ExecuteSimpleQuery( "DELETE FROM `Properties` WHERE `OwnerTypeID` = %u;", (uint32_t) type.m_ID ) )
        {
            return false;
        }

        // Update properties
        for ( auto& propertyDesc : type.m_properties )
        {
            if ( !ExecuteSimpleQuery( "INSERT OR REPLACE INTO `Properties`(`PropertyID`, `LineNumber`, `OwnerTypeID`,`TypeID`,`Name`,`Description`,`TypeName`,`TemplateTypeName`,`PropertyFlags`,`ArraySize`) VALUES ( %u, %d, %u, %u, \"%s\", \"%s\", \"%s\", \"%s\", %u, %d );", (uint32_t) propertyDesc.m_propertyID, propertyDesc.m_lineNumber, (uint32_t) type.m_ID, (uint32_t) propertyDesc.m_typeID, propertyDesc.m_name.c_str(), propertyDesc.m_description.c_str(), propertyDesc.m_typeName.c_str(), propertyDesc.m_templateArgTypeName.c_str(), (uint32_t) propertyDesc.m_flags, propertyDesc.m_arraySize ) )
            {
                return false;
            }
        }

        return true;
    }

    bool ReflectionDatabase::WriteAdditionalEnumData( ReflectedType const& type )
    {
        // Fill enum values table with all constants
        if ( !ExecuteSimpleQuery( "DELETE FROM `EnumConstants` WHERE `TypeID` = %u;", (uint32_t) type.m_ID ) )
        {
            return false;
        }

        for ( auto const& enumConstant : type.m_enumElements )
        {
            if ( !ExecuteSimpleQuery( "INSERT INTO `EnumConstants`(`TypeID`,`Label`,`Value`, `Description`) VALUES ( %u, \"%s\", %u, \"%s\" );", (uint32_t) type.m_ID, enumConstant.second.m_label.c_str(), enumConstant.second.m_value, enumConstant.second.m_description.c_str() ) )
            {
                return false;
            }
        }

        return true;
    }
}