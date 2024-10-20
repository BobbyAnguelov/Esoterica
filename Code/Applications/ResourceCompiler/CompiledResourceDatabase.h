#pragma once

#include "Base/Resource/ResourceID.h"
#include "Base/FileSystem/FileSystemPath.h"

//-------------------------------------------------------------------------

struct sqlite3;

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct CompiledResourceRecord final
    {
        inline bool IsValid() const { return m_resourceID.IsValid(); }
        inline void Clear() { *this = CompiledResourceRecord(); }

        ResourceID            m_resourceID;
        int32_t               m_compilerVersion = -1;         // The compiler version used for the last compilation
        uint64_t              m_fileTimestamp = 0;            // The timestamp of the resource file
        uint64_t              m_sourceTimestampHash = 0;      // The timestamp hash of any source assets used in the compilation
        uint64_t              m_advancedUpToDateHash = 0;      // The advanced up to date hash of a resource compile step
    };

    //-------------------------------------------------------------------------

    class CompiledResourceDatabase final
    {
        constexpr static uint32_t const s_defaultStatementBufferSize = 8096;

    public:

        ~CompiledResourceDatabase();

    public:

        bool Connect( FileSystem::Path const& databasePath );
        bool IsConnected() const { return m_pDatabase != nullptr; }
        bool Disconnect();

        inline bool HasErrorOccurred() const { return !m_errorMessage.empty(); }
        inline String const& GetError() const { return m_errorMessage; }

        // Try to get a record for a given resource ID
        bool GetRecord( ResourceID resourceID, CompiledResourceRecord& outRecord ) const;

        // Update or create a record for a given ID
        bool WriteRecord( CompiledResourceRecord const& record );

    private:

        bool CreateTables();

        bool DropTables();

    private:

        sqlite3*                            m_pDatabase = nullptr;
        mutable String                      m_errorMessage;
        mutable char                        m_statementBuffer[s_defaultStatementBufferSize] = { 0 };
    };
}