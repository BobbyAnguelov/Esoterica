#pragma once

#include "Base/_Module/API.h"
#include <stdarg.h>

//-------------------------------------------------------------------------
// This is the global logging API included throughout the entire engine
//-------------------------------------------------------------------------
// If you want access to the logging system and the engine log include "LoggingSystem.h"

namespace EE::Log
{
    enum class Severity
    {
        Message = 0,
        Warning,
        Error,
        FatalError,
    };

    // Logging
    //-------------------------------------------------------------------------

    EE_BASE_API void AddEntry( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, ... );
    EE_BASE_API void AddEntryVarArgs( Severity severity, char const* pCategory, char const* pSourceInfo, char const* pFilename, int pLineNumber, char const* pMessageFormat, va_list args );

    // Asserts
    //-------------------------------------------------------------------------

    EE_BASE_API void LogAssert( char const* pFile, int line, char const* pAssertInfo );
    EE_BASE_API void LogAssertVarArgs( char const* pFile, int line, char const* pAssertInfoFormat, ... );

    // Trace to Output Log
    //-------------------------------------------------------------------------

    EE_BASE_API void TraceMessage( const char* format, ... );
}

//-------------------------------------------------------------------------

#define EE_LOG_MESSAGE( category, source, ... ) EE::Log::AddEntry( EE::Log::Severity::Message, category, source, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_WARNING( category, source, ... ) EE::Log::AddEntry( EE::Log::Severity::Warning, category, source, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_ERROR( category, source, ... ) EE::Log::AddEntry( EE::Log::Severity::Error, category, source, __FILE__, __LINE__, __VA_ARGS__ )
#define EE_LOG_FATAL_ERROR( category, source, ... ) EE::Log::AddEntry( EE::Log::Severity::FatalError, category, source, __FILE__, __LINE__, __VA_ARGS__ ); EE_HALT()