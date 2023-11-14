#pragma once

#include "EngineTools/_Module/API.h"
#include "Base/FileSystem/FileSystemPath.h"
#include "Base/Math/Matrix.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class EE_ENGINETOOLS_API ImportedData
    {

    public:

        ImportedData() = default;
        ImportedData( ImportedData const& ) = default;
        virtual ~ImportedData() = default;

        ImportedData& operator=( ImportedData const& rhs ) = default;

        virtual bool IsValid() const = 0;

        inline bool HasWarnings() const { return !m_warnings.empty(); }
        inline TVector<String> const& GetWarnings() const { return m_warnings; }

        inline bool HasErrors() const { return !m_errors.empty(); }
        inline TVector<String> const& GetErrors() const { return m_errors; }

        inline FileSystem::Path const& GetSourcePath() const { return m_sourcePath; }

    protected:

        void LogError( char const* pFormat, ... )
        {
            char buffer[512];
            va_list args;
            va_start( args, pFormat );
            VPrintf( buffer, 512, pFormat, args );
            va_end( args );
            m_errors.emplace_back( buffer );
        }

        void LogWarning( char const* pFormat, ... )
        {
            char buffer[512];
            va_list args;
            va_start( args, pFormat );
            VPrintf( buffer, 512, pFormat, args );
            va_end( args );
            m_warnings.emplace_back( buffer );
        }

    protected:

        FileSystem::Path                    m_sourcePath;
        TVector<String>                     m_warnings;
        TVector<String>                     m_errors;
    };
}