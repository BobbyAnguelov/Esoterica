#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    CXChildVisitResult VisitMacro( ClangParserContext* pContext, ReflectedHeader const* pReflectedHeader, CXCursor cr, String const& cursorName );
}