#pragma once

#include "EngineTools/_Module/API.h"
#include "System/Types/Arrays.h"
#include "System/Math/Matrix.h"
#include "System/Types/String.h"
#include "System/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    class EE_ENGINETOOLS_API RawAsset
    {

    public:

        RawAsset() = default;
        virtual ~RawAsset() = default;

        virtual bool IsValid() const = 0;

        inline bool HasWarnings() const { return !m_warnings.empty(); }
        inline TVector<String> const& GetWarnings() const { return m_warnings; }

        inline bool HasErrors() const { return !m_errors.empty(); }
        inline TVector<String> const& GetErrors() const { return m_errors; }

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

        TVector<String>                     m_warnings;
        TVector<String>                     m_errors;
    };
}