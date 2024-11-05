#pragma once

#include "ClangParserContext.h"

//-------------------------------------------------------------------------

namespace EE
{
    namespace TypeSystem
    {
        namespace Reflection
        {
            CXChildVisitResult VisitStructure( ClangParserContext* pContext, CXCursor& cr, FileSystem::Path const& headerFilePath, StringID const headerID );
        }
    }
}