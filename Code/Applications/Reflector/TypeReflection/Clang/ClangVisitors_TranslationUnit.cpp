#include "ClangVisitors_TranslationUnit.h"
#include "ClangVisitors_Macro.h"
#include "ClangVisitors_Enum.h"
#include "Clangvisitors_Structure.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    CXChildVisitResult VisitTranslationUnit( CXCursor cr, CXCursor parent, CXClientData pClientData )
    {
        auto pContext = reinterpret_cast<ClangParserContext*>( pClientData );
        if ( pContext->HasErrorOccured() )
        {
            return CXChildVisit_Break;
        }

        FileSystem::Path const headerFilePath( ClangUtils::GetHeaderPathForCursor( cr ) );
        if ( !headerFilePath.IsValid() )
        {
            return CXChildVisit_Continue;
        }

        // Dont parse non-solution files
        if ( !headerFilePath.IsUnderDirectory( pContext->m_solutionDirectoryPath ) )
        {
            return CXChildVisit_Continue;
        }

        // Ensure that the header file is part of the list of headers to visit
        StringID const headerID = ReflectedHeader::GetHeaderID( headerFilePath );
        ReflectedHeader const* pReflectedHeader = pContext->GetReflectedHeader( headerID );
        if ( pReflectedHeader == nullptr )
        {
            return CXChildVisit_Continue;
        }

        //-------------------------------------------------------------------------

        // Process Cursor
        CXCursorKind const kind = clang_getCursorKind( cr );
        String const cursorName = ClangUtils::GetCursorDisplayName( cr );
        switch ( kind )
        {
            // Classes / Structs
            case CXCursor_ClassTemplate:
            {
                ReflectionMacro macro;
                if ( pContext->GetReflectionMacroForType( headerID, cr, macro ) )
                {
                    pContext->LogError( "Cannot register template class (%s)", cursorName.c_str() );
                    return CXChildVisit_Break;
                }

                return CXChildVisit_Continue;
            }
            break;

            // Classes / Structs
            case CXCursor_ClassDecl:
            case CXCursor_StructDecl:
            {
                // Process children before the parent so that we can correctly handle the mapping between macro and types
                // We dont want an nested registration macro to cause an unwanted type to be registered
                pContext->PushNamespace( cursorName );
                clang_visitChildren( cr, VisitTranslationUnit, pClientData );
                pContext->PopNamespace();

                if ( pContext->HasErrorOccured() )
                {
                    return CXChildVisit_Break;
                }

                return VisitStructure( pContext, cr, headerFilePath, headerID );
            }
            break;

            // Enums
            case CXCursor_EnumDecl:
            {
                return VisitEnum( pContext, cr, headerID );
            }
            break;

            // Non-Type Cursors
            case CXCursor_Namespace:
            {
                if ( pContext->IsInEngineNamespace() || pContext->IsEngineNamespace( cursorName ) )
                {
                    pContext->PushNamespace( cursorName );
                    clang_visitChildren( cr, VisitTranslationUnit, pClientData );
                    pContext->PopNamespace();
                }

                return CXChildVisit_Continue;
            }
            break;

            // Macros
            case CXCursor_MacroExpansion:
            {
                return VisitMacro( pContext, pReflectedHeader, cr, cursorName );
            }
            break;

            // Irrelevant Cursors
            default:
            {
                return CXChildVisit_Continue;
            }
        }
    }
}