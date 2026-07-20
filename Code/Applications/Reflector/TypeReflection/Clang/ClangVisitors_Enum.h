#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    CXChildVisitResult VisitEnum( ClangParserContext* pContext, CXCursor cr );
}