#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/Resource/ResourceID.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Threading/Threading.h"

//-------------------------------------------------------------------------

struct sqlite3;

//-------------------------------------------------------------------------

namespace EE::Resource
{
    struct CompiledResourceRecord final
    {
        inline bool IsValid() const { return m_resourceID.IsValid(); }
        inline void Clear() { *this = CompiledResourceRecord(); }

        ResourceID                      m_resourceID;
        uint64_t                        m_compilerVersion = UINT64_MAX; // The version used for the last compilation
        uint64_t                        m_sourceResourceHash = 0;       // The timestamp hash of any source assets used in the compilation
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API CompiledResourceDatabase final
    {
        constexpr static uint32_t const s_defaultStatementBufferSize = 8096;
        constexpr static int32_t const s_version = 1;

    public:

        CompiledResourceDatabase();
        ~CompiledResourceDatabase();

    public:

        bool Connect( FileSystem::Path const& databasePath );
        bool IsConnected() const { return m_pDatabase != nullptr; }
        bool Disconnect();

        inline bool HasErrorOccurred() const { return !m_errorMessage.empty(); }
        inline String const& GetError() const { return m_errorMessage; }

        void ResetDatabase();

        // Compiled Resources
        //-------------------------------------------------------------------------

        // Try to get a record for a given resource ID
        bool GetRecord( ResourceID resourceID, CompiledResourceRecord& outRecord ) const;

        // Update or create a record for a given ID
        bool WriteRecord( CompiledResourceRecord const& record );

        // Update or create a record for a given ID
        bool DeleteRecord( ResourceID resourceID );

        // Compile Dependencies
        //-------------------------------------------------------------------------

        // Write all compile dependencies for a given resource
        bool WriteCompileDependencies( ResourceID const& resourceID, TVector<DataPath> const& dependencies );

        // Get all the dependencies for a given resource
        bool GetAllCompileDependenciesForResource( ResourceID const& resourceID, TVector<DataPath>& outDependencies );

        // Get any resources that depend on a given file
        bool GetResourcesThatDependsOnPath( DataPath const& dependencyPath, TVector<ResourceID>& outResources );

        // Remove any written dependencies for a given resource
        bool DeleteAllCompileDependenciesForResource( ResourceID const& resourceID );

    private:

        bool CreateTables();

        bool DropTables();

    private:

        sqlite3*                                m_pDatabase = nullptr;
        mutable String                          m_errorMessage;
        mutable Threading::ReadWriteMutex       m_mutex;
    };
}