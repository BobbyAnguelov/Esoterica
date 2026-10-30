#include "FormatFile.h"
#include "Base/FileSystem/FileSystem.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    File::File( Source const& source )
        : m_sourcePath( source.m_path )
    {
        EE_ASSERT( source.IsValid() );

        if ( source.m_pFileData == nullptr )
        {
            if ( !FileSystem::ReadBinaryFile( source.m_path, m_fileData ) )
            {
                m_error = String( String::CtorSprintf(), "Failed to read source file: %s", source.m_path.c_str() );
                return;
            }
        }
        else
        {
            m_fileData = *source.m_pFileData;
        }
    }

    //-------------------------------------------------------------------------

    SceneFile::SceneFile( Source const& source, FileSystem::Path const& sourceDataDirectoryPath )
        : File( source )
        , m_sourceDataDirectoryPath( sourceDataDirectoryPath )
    {
        EE_ASSERT( source.IsValid() );
        EE_ASSERT( m_sourcePath.IsValid() );
        EE_ASSERT( m_sourceDataDirectoryPath.IsValid() );
    }

    void SceneFile::Contents::Clear()
    {
        m_materials.clear();
        m_staticMeshes.clear();
        m_skeletalMeshes.clear();
        m_skeletons.clear();
        m_animations.clear();
    }
}