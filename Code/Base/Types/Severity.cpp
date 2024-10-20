#include "Severity.h"

//-------------------------------------------------------------------------

namespace EE
{
    constexpr static char const* const g_severityLabels[] = { "Message", "Warning", "Error", "Fatal Error" };

    //-------------------------------------------------------------------------

    char const* GetSeverityAsString( Severity severity )
    {
        return g_severityLabels[(int) severity];
    }
}