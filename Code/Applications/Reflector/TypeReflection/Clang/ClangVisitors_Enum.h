#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace TypeSystem
    {
        namespace Reflection
        {
            CXChildVisitResult VisitEnum( ClangParserContext* pContext, CXCursor cr, StringID const headerID );
        }
    }
}