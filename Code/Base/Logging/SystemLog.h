#pragma once
#include "Base/Types/String.h"
#include "Base/Types/Severity.h"
#include "Base/Types/UUID.h"
#include "Base/Types/StringID.h"
#include "Base/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

namespace EE::FileSystem { class Path; }

//-------------------------------------------------------------------------

namespace EE::SystemLog
{
    struct Entry
    {
        UUID                        m_ID = UUID::GenerateID();
        TInlineString<10>           m_timestamp;
        StringID                    m_category;
        InlineString                m_sourceInfoStr; // Extra optional information about the source of the log
        TInlineVector<StringID,3>   m_sourceInfo; // Extra optional information about the source of the log (split into IDs)
        InlineString                m_message;
        InlineString                m_filename;
        uint32_t                    m_lineNumber;
        Severity                    m_severity;
    };

    // Lifetime
    //-------------------------------------------------------------------------

    EE_BASE_API void Initialize();
    EE_BASE_API void Shutdown();
    EE_BASE_API bool WasInitialized();

    // Accessors
    //-------------------------------------------------------------------------

    EE_BASE_API int32_t GetNumEntries();
    EE_BASE_API int32_t GetNumWarnings();
    EE_BASE_API int32_t GetNumErrors();
    EE_BASE_API int32_t GetNumMessages();

    // Get all the entries for the specified range
    EE_BASE_API void GetLogEntries( int32_t startIdx, int32_t numEntries, TVector<Entry const*>& outEntries );

    EE_BASE_API bool HasFatalErrorOccurred();
    EE_BASE_API Entry const& GetFatalError();

    // Output
    //-------------------------------------------------------------------------

    EE_BASE_API void SaveToFile( FileSystem::Path const& logFilePath );
}