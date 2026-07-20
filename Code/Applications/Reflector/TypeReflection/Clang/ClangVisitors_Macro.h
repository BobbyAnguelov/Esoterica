#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    CXChildVisitResult VisitMacro( ClangParserContext* pContext, ReflectedHeader const* pReflectedHeader, CXCursor cr, String const& cursorName );
}