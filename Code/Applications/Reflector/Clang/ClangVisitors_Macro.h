#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    CXChildVisitResult VisitMacro( ClangParserContext* pContext, HeaderInfo const* pHeaderInfo, CXCursor cr, String const& cursorName );
}