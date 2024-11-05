#include "ClangVisitors_Macro.h"
#include "clang-c/Documentation.h"
#include <iostream>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    CXChildVisitResult VisitMacro( ClangParserContext * pContext, ReflectedHeader const* pReflectedHeader, CXCursor cr, String const& cursorName )
    {
        CXSourceRange range = clang_getCursorExtent( cr );

        //-------------------------------------------------------------------------

        if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::ReflectProperty ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::ReflectProperty ) );
        }
        else if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::ReflectEnum ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::ReflectEnum ) );
        }
        else if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::ReflectType ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::ReflectType ) );
        }
        else if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::EntityComponent ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::EntityComponent ) );
        }
        else if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::SingletonEntityComponent ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::SingletonEntityComponent ) );
        }
        else if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::EntitySystem ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::EntitySystem ) );
        }
        else if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::EntityWorldSystem ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::EntityWorldSystem ) );
        }
        else if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::ReflectModule ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::ReflectModule ) );
        }
        else if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::Resource ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::Resource ) );
        }
        else if ( cursorName == ReflectedHeader::GetReflectionMacroText( ReflectedHeader::ReflectionMacro::DataFile ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pReflectedHeader, cr, range, ReflectedHeader::ReflectionMacro::DataFile ) );
        }

        //-------------------------------------------------------------------------

        return CXChildVisit_Continue;
    }
}