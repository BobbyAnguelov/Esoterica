#pragma once
#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Types/Percentage.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace TypeSystem { class TypeRegistry; }

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API DataFileResaver
    {
    public:

        DataFileResaver( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceDataDirectoryPath );

        void Reset();

        // Are we currently resaving
        inline bool IsResaving() const { return m_isResaving; }

        // Get the current progress of the resave
        Percentage GetProgress() const;

        // Start a resave operation
        bool BeginResave();

        // Update a resave operation - returns the number of files left to resave
        // User can optionally specify the number of files to resave
        int32_t UpdateResave( int32_t numFilesToResave = -1 );

        // Complete a resave operation
        void EndResave();

        // Get the total number of files that need to be resaved
        inline int32_t GetNumDataFiles() const { return (int32_t) m_filesToResave.size(); }

        // Get the number of files that still need to be resaved
        int32_t GetNumberOfFilesLeftToResave() const;

    private:

        TypeSystem::TypeRegistry const&     m_typeRegistry;
        FileSystem::Path const              m_sourceDataDirectoryPath;
        TVector<FileSystem::Path>           m_filesToResave;
        int32_t                             m_numFilesResaved = 0;
        bool                                m_isResaving = false;
    };
}