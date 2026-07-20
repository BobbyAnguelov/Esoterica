#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    CXChildVisitResult VisitStructure( ClangParserContext* pContext, CXCursor& cr );
}