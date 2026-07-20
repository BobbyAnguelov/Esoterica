#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    CXChildVisitResult VisitTranslationUnit( CXCursor cr, CXCursor parent, CXClientData pClientData );
}