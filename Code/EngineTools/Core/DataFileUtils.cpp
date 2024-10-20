#include "DataFileUtils.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/FileSystem/FileSystem.h"
#include "Base/FileSystem/IDataFile.h"
#include "Base/FileSystem/FileSystemUtils.h"

//-------------------------------------------------------------------------

namespace EE
{
    DataFileResaver::DataFileResaver( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceDataDirectoryPath )
        : m_typeRegistry( typeRegistry )
        , m_sourceDataDirectoryPath( sourceDataDirectoryPath )
    {
        EE_ASSERT( sourceDataDirectoryPath.IsValid() && sourceDataDirectoryPath.IsDirectoryPath() && sourceDataDirectoryPath.Exists() );
    }

    void DataFileResaver::Reset()
    {
        m_filesToResave.clear();
        m_numFilesResaved = 0;
        m_isResaving = false;
    }

    bool DataFileResaver::BeginResave()
    {
        EE_ASSERT( !m_isResaving ); // Dont call begin without calling end
        Reset();

        // Get all data file extensions
        //-------------------------------------------------------------------------

        TVector<FileSystem::Extension> extensions;
        TVector<TypeSystem::TypeInfo const*> dataFileTypes = m_typeRegistry.GetAllDerivedTypes( IDataFile::GetStaticTypeID(), false, false, false );
        for ( TypeSystem::TypeInfo const* pDataFileTypeInfo : dataFileTypes )
        {
            FileSystem::Extension const extension = Cast<IDataFile>( pDataFileTypeInfo->m_pDefaultInstance )->GetExtension();
            if ( !extension.empty() )
            {
                extensions.emplace_back( extension );
            }
        }

        // Get all files in the source data dir
        //-------------------------------------------------------------------------

        if ( !FileSystem::GetDirectoryContents( m_sourceDataDirectoryPath, m_filesToResave, FileSystem::DirectoryReaderOutput::OnlyFiles, FileSystem::DirectoryReaderMode::Recursive, extensions ) )
        {
            EE_LOG_ERROR( "DataFile", "Resave Data Files", "Failed to enumerator source data directory (%s).", m_sourceDataDirectoryPath.c_str() );
            EndResave();
            return false;
        }

        m_isResaving = true;
        return true;
    }

    int32_t DataFileResaver::UpdateResave( int32_t numFilesToResave )
    {
        EE_ASSERT( m_isResaving );

        //-------------------------------------------------------------------------

        int32_t const numFiles = int32_t( m_filesToResave.size() );
        int32_t const numFilesRemaining = numFiles - m_numFilesResaved;

        // Resave all
        if ( numFilesToResave < 0 )
        {
            numFilesToResave = numFilesRemaining;
            EE_ASSERT( numFilesToResave >= 0 );
        }

        // Clamp to remaining files
        numFilesToResave = Math::Min( numFilesToResave, numFilesRemaining );
        EE_ASSERT( numFilesToResave >= 0 );

        // Nothing to do
        if ( numFilesToResave == 0 )
        {
            return 0;
        }

        // Resave requested number of files
        int32_t const startIdx = m_numFilesResaved;
        int32_t const endIdx = startIdx + numFilesToResave - 1;
        EE_ASSERT( endIdx < numFiles );
        for ( int32_t i = startIdx; i <= endIdx; i++ )
        {
            IDataFile* pDatafile = IDataFile::TryReadFromFile( m_typeRegistry, m_filesToResave[i] );
            if ( pDatafile != nullptr )
            {
                IDataFile::TryWriteToFile( m_typeRegistry, m_filesToResave[i], pDatafile, false );
                EE::Delete( pDatafile );
            }
        }

        // Update file counter
        m_numFilesResaved += numFilesToResave;
        EE_ASSERT( m_numFilesResaved <= numFiles );
        return numFiles - m_numFilesResaved;
    }

    void DataFileResaver::EndResave()
    {
        EE_ASSERT( m_isResaving ); // Dont call end without begin
        Reset();
    }

    int32_t DataFileResaver::GetNumberOfFilesLeftToResave() const
    {
        if ( !m_isResaving )
        {
            return 0;
        }

        return int32_t( m_filesToResave.size() );
    }

    Percentage DataFileResaver::GetProgress() const
    {
        if ( !m_isResaving )
        {
            return 1.0f;
        }

        return Percentage( float( m_numFilesResaved ) / m_filesToResave.size() );
    }
}