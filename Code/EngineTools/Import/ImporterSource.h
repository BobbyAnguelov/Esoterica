#pragma once
#include "Base/FileSystem/FileSystemPath.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    struct Source
    {
        Source() = default;
        Source( FileSystem::Path const& path, Blob const* pFileData = nullptr ) : m_path( path ), m_pFileData( pFileData ) { EE_ASSERT( m_path.IsValid() ); }

        inline bool IsValid() const { return m_path.IsValid(); }

        inline FileSystem::Extension GetExtension() const { return m_path.GetLowercaseExtension(); }

    public:

        FileSystem::Path    m_path;
        Blob const*         m_pFileData = nullptr; // Optional: pre-loaded file-data
    };
}