#include "SqliteHelpers.h"
#include "System/FileSystem/FileSystem.h"
#include <sqlite3.h>

//-------------------------------------------------------------------------
namespace EE::SQLite
{
    SQLiteDatabase::~SQLiteDatabase()
    {
        if ( IsConnected() )
        {
            Disconnect();
        }
    }

    bool SQLiteDatabase::Connect( FileSystem::Path const& databasePath, bool readOnlyAccess, bool useMutex )
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
            m_errorMessage = String( "Couldn't open sqlite database: %s" ) + databasePath;
            return false;
        }

        return true;
    }

    bool SQLiteDatabase::Disconnect()
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

    bool SQLiteDatabase::IsValidSQLiteResult( int result, char const* pErrorMessage ) const
    {
        if ( result != SQLITE_OK )
        {
            m_errorMessage = String( sqlite3_errstr( result ) ) + " (" + sqlite3_errmsg( m_pDatabase ) + ")";
            return false;
        }

        return true;
    }

    void SQLiteDatabase::FillStatementBuffer( char const* pFormat, ... ) const
    {
        EE_ASSERT( IsConnected() );

        // Create the statement using the sqlite printf so we can use the extra format specifiers i.e. %Q
        va_list args;
        va_start( args, pFormat );
        sqlite3_vsnprintf( s_defaultStatementBufferSize, m_statementBuffer, pFormat, args );
        va_end( args );
    }

    bool SQLiteDatabase::ExecuteSimpleQuery( char const* pFormat, ... ) const
    {
        EE_ASSERT( IsConnected() );

        // Create the statement using the sqlite printf so we can use the extra format specifiers i.e. %Q
        va_list args;
        va_start( args, pFormat );
        sqlite3_vsnprintf( s_defaultStatementBufferSize, m_statementBuffer, pFormat, args );
        va_end( args );

        return IsValidSQLiteResult( sqlite3_exec( m_pDatabase, m_statementBuffer, nullptr, nullptr, nullptr ) );
    }

    bool SQLiteDatabase::BeginTransaction() const
    {
        EE_ASSERT( IsConnected() );
        return IsValidSQLiteResult( sqlite3_exec( m_pDatabase, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr ) );
    }

    bool SQLiteDatabase::EndTransaction() const
    {
        EE_ASSERT( IsConnected() );
        return IsValidSQLiteResult( sqlite3_exec( m_pDatabase, "END TRANSACTION;", nullptr, nullptr, nullptr ) );
    }
}