#pragma once

#include "EngineTools/ThirdParty/sqlite/SqliteHelpers.h"
#include "System/Resource/ResourceID.h"
#include "System/FileSystem/FileSystemPath.h"
#include "System/Threading/Threading.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace Resource
    {
        struct CompiledResourceRecord final
        {
            inline bool IsValid() const { return m_resourceID.IsValid(); }
            inline void Clear() { *this = CompiledResourceRecord(); }

            ResourceID            m_resourceID;
            int32_t               m_compilerVersion = -1;         // The compiler version used for the last compilation
            uint64_t              m_fileTimestamp = 0;            // The timestamp of the resource file
            uint64_t              m_sourceTimestampHash = 0;      // The timestamp hash of any source assets used in the compilation
        };

        //-------------------------------------------------------------------------

        class CompiledResourceDatabase final : public SQLite::SQLiteDatabase
        {
        public:

            bool TryConnect( FileSystem::Path const& databasePath );

            bool CleanDatabase( FileSystem::Path const& databasePath );

            // Database functions
            CompiledResourceRecord GetRecord( ResourceID resourceID ) const;
            bool WriteRecord( CompiledResourceRecord const& record );

        private:

            bool CreateTables();
            bool DropTables();

        private:

            mutable Threading::Mutex m_mutex;
        };
    }
}