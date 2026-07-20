#include "ResourceCompilationDatabase.h"
#include "Base/FileSystem/FileSystem.h"
#include <sqlite3.h>

//-------------------------------------------------------------------------

namespace EE::Resource
{
    static uint64_t ReadUnsignedInt64( sqlite3_stmt* pStatement, int32_t col )
    {
        uint64_t value = 0;

        int32_t const numBytes = sqlite3_column_bytes( pStatement, col );
        if ( numBytes == 8 )
        {
            memcpy( &value, sqlite3_column_blob( pStatement, col ), numBytes );
        }

        return value;
    };

    //-------------------------------------------------------------------------

    CompiledResourceDatabase::CompiledResourceDatabase()
    {}

    CompiledResourceDatabase::~CompiledResourceDatabase()
    {
        if ( IsConnected() )
        {
            Disconnect();
        }
    }

    bool CompiledResourceDatabase::Connect( FileSystem::Path const& databasePath )
    {
        EE_ASSERT( !IsConnected() );
        EE_ASSERT( databasePath.IsFilePath() );

        databasePath.EnsureDirectoryExists();

        int32_t sqlFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_WAL;
        int32_t result = sqlite3_open_v2( databasePath, &m_pDatabase, sqlFlags, nullptr );
        sqlite3_busy_timeout( m_pDatabase, 2500 );

        if ( result != SQLITE_OK )
        {
            sqlite3_close( m_pDatabase );
            m_pDatabase = nullptr;
            m_errorMessage = String( "Couldn't open sqlite database: %s" ) + databasePath;
            return false;
        }

        // Check version
        //-------------------------------------------------------------------------

        int32_t currentVersion = 0;
        sqlite3_stmt* pStatement = nullptr;
        result = sqlite3_prepare_v2( m_pDatabase, "PRAGMA user_version;", -1, &pStatement, NULL );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        if ( sqlite3_step( pStatement ) == SQLITE_ROW )
        {
            currentVersion = sqlite3_column_int( pStatement, 0 );
        }
        sqlite3_finalize( pStatement );

        if ( currentVersion != s_version )
        {
            DropTables();
        }

        // Set version number
        //-------------------------------------------------------------------------

        InlineString const setVersionStatement( InlineString::CtorSprintf(), "PRAGMA user_version = %d;", s_version );
        result = sqlite3_exec( m_pDatabase, setVersionStatement.c_str(), nullptr, nullptr, nullptr );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        // Create/Update tables
        //-------------------------------------------------------------------------

        CreateTables();

        return true;
    }

    bool CompiledResourceDatabase::Disconnect()
    {
        if ( m_pDatabase != nullptr )
        {
            Threading::ScopeLockWrite sl( m_mutex );
            auto result = sqlite3_close( m_pDatabase );
            EE_ASSERT( result == SQLITE_OK ); // If we get SQLITE_BUSY, this means we are leaking sqlite resources
            m_pDatabase = nullptr;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool CompiledResourceDatabase::CreateTables()
    {
        EE_ASSERT( m_pDatabase != nullptr );

        Threading::ScopeLockWrite sl( m_mutex );

        TInlineVector<char, s_defaultStatementBufferSize> statementBuffer( s_defaultStatementBufferSize );
        constexpr char const* const statementCR = "CREATE TABLE IF NOT EXISTS `CompiledResources` ( `DataPath` TEXT UNIQUE,`ResourceType` INTEGER,`CompilerVersion` INTEGER, `SourceResourceHash` BLOB, PRIMARY KEY( `DataPath`, `ResourceType` ) );";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statementCR );
        int32_t result = sqlite3_exec( m_pDatabase, statementBuffer.data(), nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        //-------------------------------------------------------------------------

        constexpr char const* const statementCD = "CREATE TABLE IF NOT EXISTS `CompileDependencies` ( `ResourceID` TEXT, `CompileDependencyPath` TEXT, PRIMARY KEY( `ResourceID`, `CompileDependencyPath` ) );";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statementCD );
        result = sqlite3_exec( m_pDatabase, statementBuffer.data(), nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        return true;
    }

    bool CompiledResourceDatabase::DropTables()
    {
        EE_ASSERT( m_pDatabase != nullptr );

        Threading::ScopeLockWrite sl( m_mutex );

        TInlineVector<char, s_defaultStatementBufferSize> statementBuffer( s_defaultStatementBufferSize );
        constexpr char const* const statementCR = "DROP TABLE IF EXISTS `CompiledResources`;";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statementCR );
        int32_t result = sqlite3_exec( m_pDatabase, statementBuffer.data(), nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        //-------------------------------------------------------------------------

        constexpr char const* const statementCD = "DROP TABLE IF EXISTS `CompileDependencies`;";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statementCD );
        result = sqlite3_exec( m_pDatabase, statementBuffer.data(), nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool CompiledResourceDatabase::GetRecord( ResourceID resourceID, CompiledResourceRecord& outRecord ) const
    {
        outRecord.Clear();

        Threading::ScopeLockRead sl( m_mutex );

        // Prepare the statement
        //-------------------------------------------------------------------------

        TInlineVector<char, s_defaultStatementBufferSize> statementBuffer( s_defaultStatementBufferSize );
        constexpr char const* const statement = "SELECT * FROM `CompiledResources` WHERE `DataPath` = \"%s\" AND `ResourceType` = ?;";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statement, resourceID.GetDataPath().c_str() );

        sqlite3_stmt* pStatement = nullptr;
        int32_t result = sqlite3_prepare_v2( m_pDatabase, statementBuffer.data(), -1, &pStatement, nullptr );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        uint64_t const resourceTypeID = resourceID.GetResourceTypeID().ToUInt64();
        sqlite3_bind_blob( pStatement, 1, &resourceTypeID, sizeof( uint64_t ), SQLITE_STATIC );

        // Execute prepared statement
        //-------------------------------------------------------------------------

        while ( sqlite3_step( pStatement ) == SQLITE_ROW )
        {
            String const resourcePath = (char const*) sqlite3_column_text( pStatement, 0 );
            outRecord.m_resourceID = ResourceID( resourcePath );

            uint64_t const expectedResourceTypeID = ReadUnsignedInt64( pStatement, 1 );
            EE_ASSERT( outRecord.m_resourceID.GetResourceTypeID().ToUInt64() == expectedResourceTypeID );

            outRecord.m_compilerVersion = ReadUnsignedInt64( pStatement, 2 );
            outRecord.m_sourceResourceHash = ReadUnsignedInt64( pStatement, 3 );
        }

        result = sqlite3_finalize( pStatement );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        return true;
    }

    bool CompiledResourceDatabase::WriteRecord( CompiledResourceRecord const& record )
    {
        EE_ASSERT( IsConnected() );

        Threading::ScopeLockWrite sl( m_mutex );

        sqlite3_exec( m_pDatabase, "BEGIN TRANSACTION", NULL, NULL, NULL );

        ResourceTypeID const typeID = record.m_resourceID.GetResourceTypeID();

        TInlineVector<char, s_defaultStatementBufferSize> statementBuffer( s_defaultStatementBufferSize );
        constexpr char const* const statementFormat = "INSERT OR REPLACE INTO `CompiledResources` ( `DataPath`, `ResourceType`, `CompilerVersion`, `SourceResourceHash` ) VALUES ( \"%s\", ?, ?, ? );";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statementFormat, record.m_resourceID.GetDataPath().c_str() );

        sqlite3_stmt* pStatement = nullptr;
        int32_t result = sqlite3_prepare_v2( m_pDatabase, statementBuffer.data(), -1, &pStatement, NULL );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        uint64_t const resourceTypeID = record.m_resourceID.GetResourceTypeID().ToUInt64();
        sqlite3_bind_blob( pStatement, 1, &resourceTypeID, sizeof( uint64_t ), SQLITE_STATIC );
        sqlite3_bind_blob( pStatement, 2, &record.m_compilerVersion, sizeof( uint64_t ), SQLITE_STATIC );
        sqlite3_bind_blob( pStatement, 3, &record.m_sourceResourceHash, sizeof( uint64_t ), SQLITE_STATIC );

        result = sqlite3_step( pStatement );
        if ( result != SQLITE_DONE )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        result = sqlite3_finalize( pStatement );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        sqlite3_exec( m_pDatabase, "END TRANSACTION", NULL, NULL, NULL );

        return true;
    }

    bool CompiledResourceDatabase::DeleteRecord( ResourceID resourceID )
    {
        EE_ASSERT( m_pDatabase != nullptr );
        EE_ASSERT( resourceID.IsValid() );

        Threading::ScopeLockWrite sl( m_mutex );

        TInlineVector<char, s_defaultStatementBufferSize> statementBuffer( s_defaultStatementBufferSize );
        constexpr char const* const statementFormat = "BEGIN TRANSACTION; DELETE FROM `CompiledResources` WHERE `DataPath` =  \"%s\"; END TRANSACTION;";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statementFormat, resourceID.GetDataPath().c_str() );
        int32_t result = sqlite3_exec( m_pDatabase, statementBuffer.data(), nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        return true;
    }

    //-------------------------------------------------------------------------

    bool CompiledResourceDatabase::WriteCompileDependencies( ResourceID const& resourceID, TVector<DataPath> const& dependencies )
    {
        EE_ASSERT( m_pDatabase != nullptr );
        EE_ASSERT( resourceID.IsValid() );

        if ( !DeleteAllCompileDependenciesForResource( resourceID ) )
        {
            return false;
        }

        //-------------------------------------------------------------------------

        if ( dependencies.empty() )
        {
            return true;
        }

        // Ensure we have no duplicate dependencies
        //-------------------------------------------------------------------------

        for ( int32_t i = 0; i < dependencies.size(); i++ )
        {
            for ( int32_t j = i+1; j < dependencies.size(); j++ )
            {
                if ( dependencies[i] == dependencies[j] )
                {
                    m_errorMessage = "WriteCompileDependencies - Duplicate dependencies detected!";
                    return false;
                }
            }
        }

        //-------------------------------------------------------------------------

        String statementStr = "INSERT INTO CompileDependencies(ResourceID, CompileDependencyPath) VALUES";

        for ( DataPath const& dataPath : dependencies )
        {
            statementStr.append_sprintf( "( \"%s\", \"%s\" ), ", resourceID.c_str(), dataPath.c_str() );
        }

        statementStr.pop_back();
        statementStr.pop_back();
        statementStr.append( ";" );

        // Resize buffer if needed (over-allocate to attempt to prevent further alloc)
        TInlineVector<char, s_defaultStatementBufferSize> statementBuffer( s_defaultStatementBufferSize );
        if ( statementStr.size() > statementBuffer.size() )
        {
            statementBuffer.resize( statementStr.size() * 2 );
        }

        //-------------------------------------------------------------------------

        Threading::ScopeLockWrite sl( m_mutex );

        sqlite3_exec( m_pDatabase, "BEGIN TRANSACTION", NULL, NULL, NULL );

        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statementStr.c_str() );

        sqlite3_stmt* pStatement = nullptr;
        int32_t result = sqlite3_prepare_v2( m_pDatabase, statementBuffer.data(), -1, &pStatement, NULL );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        result = sqlite3_step( pStatement );
        if ( result != SQLITE_DONE )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        result = sqlite3_finalize( pStatement );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        sqlite3_exec( m_pDatabase, "END TRANSACTION", NULL, NULL, NULL );

        return true;
    }

    bool CompiledResourceDatabase::GetAllCompileDependenciesForResource( ResourceID const& resourceID, TVector<DataPath>& outDependencies )
    {
        EE_ASSERT( m_pDatabase != nullptr );
        EE_ASSERT( resourceID.IsValid() );

        Threading::ScopeLockRead sl( m_mutex );

        // Prepare the statement
        //-------------------------------------------------------------------------

        TInlineVector<char, s_defaultStatementBufferSize> statementBuffer( s_defaultStatementBufferSize );
        constexpr char const* const statement = "SELECT `CompileDependencyPath` FROM `CompileDependencies` WHERE `ResourceID` = \"%s\";";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statement, resourceID.GetDataPath().c_str() );

        sqlite3_stmt* pStatement = nullptr;
        int32_t result = sqlite3_prepare_v2( m_pDatabase, statementBuffer.data(), -1, &pStatement, nullptr );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        // Execute prepared statement
        //-------------------------------------------------------------------------

        while ( sqlite3_step( pStatement ) == SQLITE_ROW )
        {
            DataPath const dataPath( (char const*) sqlite3_column_text( pStatement, 0 ) );
            outDependencies.emplace_back( dataPath );
        }

        result = sqlite3_finalize( pStatement );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        return true;
    }

    bool CompiledResourceDatabase::GetResourcesThatDependsOnPath( DataPath const& dependencyPath, TVector<ResourceID>& outResources )
    {
        EE_ASSERT( m_pDatabase != nullptr );
        EE_ASSERT( dependencyPath.IsValid() );

        Threading::ScopeLockRead sl( m_mutex );

        // Prepare the statement
        //-------------------------------------------------------------------------

        TInlineVector<char, s_defaultStatementBufferSize> statementBuffer( s_defaultStatementBufferSize );
        constexpr char const* const statement = "SELECT `ResourceID` FROM `CompileDependencies` WHERE `CompileDependencyPath` = \"%s\";";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statement, dependencyPath.c_str() );

        sqlite3_stmt* pStatement = nullptr;
        int32_t result = sqlite3_prepare_v2( m_pDatabase, statementBuffer.data(), -1, &pStatement, nullptr );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        // Execute prepared statement
        //-------------------------------------------------------------------------

        while ( sqlite3_step( pStatement ) == SQLITE_ROW )
        {
            ResourceID const resourceID( (char const*) sqlite3_column_text( pStatement, 0 ) );
            outResources.emplace_back( resourceID );
        }

        result = sqlite3_finalize( pStatement );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        return true;
    }

    bool CompiledResourceDatabase::DeleteAllCompileDependenciesForResource( ResourceID const& resourceID )
    {
        EE_ASSERT( m_pDatabase != nullptr );
        EE_ASSERT( resourceID.IsValid() );

        Threading::ScopeLockWrite sl( m_mutex );

        TInlineVector<char, s_defaultStatementBufferSize> statementBuffer( s_defaultStatementBufferSize );
        constexpr char const* const statementFormat = "BEGIN TRANSACTION; DELETE FROM `CompileDependencies` WHERE `ResourceID` =  \"%s\"; END TRANSACTION;";
        sqlite3_snprintf( (int32_t) statementBuffer.size(), statementBuffer.data(), statementFormat, resourceID.GetDataPath().c_str() );
        int32_t result = sqlite3_exec( m_pDatabase, statementBuffer.data(), nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        return true;
    }

    void CompiledResourceDatabase::ResetDatabase()
    {
        DropTables();
        CreateTables();
    }
}