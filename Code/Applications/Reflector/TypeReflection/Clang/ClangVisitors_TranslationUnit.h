#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace TypeSystem
    {
        namespace Reflection
        {
            CXChildVisitResult VisitTranslationUnit( CXCursor cr, CXCursor parent, CXClientData pClientData );
        }
    }
}