#pragma once
#include "Base/Types/String.h"
#include "Base/Types/Severity.h"
#include "Base/Types/UUID.h"

//-------------------------------------------------------------------------

namespace EE::Log
{
    struct Entry
    {
        UUID        m_ID = UUID::GenerateID();
        String      m_timestamp;
        String      m_category;
        String      m_sourceInfo; // Extra optional information about the source of the log
        String      m_message;
        String      m_filename;
        uint32_t    m_lineNumber;
        Severity    m_severity;
    };
}