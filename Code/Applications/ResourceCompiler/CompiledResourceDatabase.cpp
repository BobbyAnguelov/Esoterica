#include "CompiledResourceDatabase.h"
#include "Base/FileSystem/FileSystem.h"
#include <sqlite3.h>

//-------------------------------------------------------------------------

namespace EE::Resource
{
    CompiledResourceDatabase::~CompiledResourceDatabase()
    {
        if ( IsConnected() )
        {
            Disconnect();
        }
    }

    bool CompiledResourceDatabase::Connect( FileSystem::Path const& databasePath )
    {
        EE_ASSERT( databasePath.IsFilePath() );

        databasePath.EnsureDirectoryExists();

        int32_t sqlFlags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_WAL;
        auto const result = sqlite3_open_v2( databasePath, &m_pDatabase, sqlFlags, nullptr );
        sqlite3_busy_timeout( m_pDatabase, 2500 );

        if ( result != SQLITE_OK )
        {
            sqlite3_close( m_pDatabase );
            m_pDatabase = nullptr;
            m_errorMessage = String( "Couldn't open sqlite database: %s" ) + databasePath;
            return false;
        }

        CreateTables();

        return true;
    }

    bool CompiledResourceDatabase::Disconnect()
    {
        if ( m_pDatabase != nullptr )
        {
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

        constexpr char const* const statement = "CREATE TABLE IF NOT EXISTS `CompiledResources` ( `DataPath` TEXT UNIQUE,`ResourceType` INTEGER,`CompilerVersion` INTEGER,`FileTimestamp` INTEGER, `SourceTimestampHash` INTEGER, `AdvancedUpToDateHash` INTEGER, PRIMARY KEY( DataPath, ResourceType ) );";
        sqlite3_snprintf( s_defaultStatementBufferSize, m_statementBuffer, statement );
        int32_t result = sqlite3_exec( m_pDatabase, m_statementBuffer, nullptr, nullptr, nullptr );

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

        constexpr char const* const statement = "DROP TABLE IF EXISTS `CompiledResources`;";
        sqlite3_snprintf( s_defaultStatementBufferSize, m_statementBuffer, statement );
        int32_t result = sqlite3_exec( m_pDatabase, m_statementBuffer, nullptr, nullptr, nullptr );

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

        // Prepare the statement
        //-------------------------------------------------------------------------

        constexpr char const* const statement = "SELECT * FROM `CompiledResources` WHERE `DataPath` = \"%s\" AND `ResourceType` = %d;";
        sqlite3_snprintf( s_defaultStatementBufferSize, m_statementBuffer, statement, resourceID.GetDataPath().c_str(), (uint32_t) resourceID.GetResourceTypeID() );

        sqlite3_stmt* pStatement = nullptr;
        int32_t result = sqlite3_prepare_v2( m_pDatabase, m_statementBuffer, -1, &pStatement, nullptr );
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        // Execute prepared statement
        //-------------------------------------------------------------------------

        while ( sqlite3_step( pStatement ) == SQLITE_ROW )
        {
            String const resourcePath = (char const*)sqlite3_column_text( pStatement, 0 );
            outRecord.m_resourceID = ResourceID( resourcePath );

            uint32_t const resourceType( sqlite3_column_int( pStatement, 1 ) );
            EE_ASSERT( outRecord.m_resourceID.GetResourceTypeID().m_ID == resourceType );

            outRecord.m_compilerVersion = sqlite3_column_int( pStatement, 2 );
            outRecord.m_fileTimestamp = sqlite3_column_int64( pStatement, 3 );
            outRecord.m_sourceTimestampHash = sqlite3_column_int64( pStatement, 4 );
            outRecord.m_advancedUpToDateHash = sqlite3_column_int64( pStatement, 5 );
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

        constexpr char const* const statement = "BEGIN TRANSACTION;INSERT OR REPLACE INTO `CompiledResources` ( `DataPath`, `ResourceType`, `CompilerVersion`, `FileTimestamp`, `SourceTimestampHash`, `AdvancedUpToDateHash` ) VALUES ( \"%s\", %d, %d, %llu, %llu, %llu );END TRANSACTION;";
        sqlite3_snprintf( s_defaultStatementBufferSize, m_statementBuffer, statement, record.m_resourceID.GetDataPath().c_str(), (uint32_t) record.m_resourceID.GetResourceTypeID(), record.m_compilerVersion, record.m_fileTimestamp, record.m_sourceTimestampHash, record.m_advancedUpToDateHash );
        int32_t result = sqlite3_exec( m_pDatabase, m_statementBuffer, nullptr, nullptr, nullptr );

        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        return true;
    }
}