#include "ClangVisitors_Macro.h"
#include "clang-c/Documentation.h"
#include <iostream>

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    CXChildVisitResult VisitMacro( ClangParserContext * pContext, ReflectedHeader const* pReflectedHeader, CXCursor cr, String const& cursorName )
    {
        CXSourceRange range = clang_getCursorExtent( cr );

        //-------------------------------------------------------------------------

        if ( cursorName == GetReflectionMacroText( ReflectionMacro::ReflectProperty ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::ReflectProperty ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::ReflectEnum ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::ReflectEnum ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::ReflectType ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::ReflectType ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::EntityComponent ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::EntityComponent ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::TransientEntityComponent ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::TransientEntityComponent ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::SingletonEntityComponent ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::SingletonEntityComponent ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::TransientSingletonEntityComponent ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::TransientSingletonEntityComponent ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::EntitySystem ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::EntitySystem ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::EntityWorldSystem ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::EntityWorldSystem ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::ReflectModule ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::ReflectModule ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::Resource ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::Resource ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::DataFile ) )
        {
            pContext->AddFoundReflectionMacro( ParsedMacro( pReflectedHeader, cr, range, ReflectionMacro::DataFile ) );
        }

        //-------------------------------------------------------------------------

        return CXChildVisit_Continue;
    }
}