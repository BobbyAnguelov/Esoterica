#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace TypeSystem
    {
        namespace Reflection
        {
            CXChildVisitResult VisitMacro( ClangParserContext* pContext, HeaderID headerID, CXCursor cr, String const& cursorName );
        }
    }
}