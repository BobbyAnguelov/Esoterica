#include "ClangVisitors_Macro.h"
#include "Applications/Reflector/ReflectorSettingsAndUtils.h"
#include "clang-c/Documentation.h"
#include <iostream>

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    CXChildVisitResult VisitMacro( ClangParserContext * pContext, HeaderInfo const* pHeaderInfo, CXCursor cr, String const& cursorName )
    {
        CXSourceRange range = clang_getCursorExtent( cr );

        //-------------------------------------------------------------------------

        if ( cursorName == GetReflectionMacroText( ReflectionMacroType::ReflectProperty ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::ReflectProperty ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacroType::ReflectEnum ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::ReflectEnum ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacroType::ReflectType ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::ReflectType ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacroType::EntityComponent ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::EntityComponent ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacroType::SingletonEntityComponent ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::SingletonEntityComponent ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacroType::EntitySystem ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::EntitySystem ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacroType::EntityWorldSystem ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::EntityWorldSystem ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacroType::ReflectModule ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::ReflectModule ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacroType::Resource ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::Resource ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacroType::DataFile ) )
        {
            pContext->AddFoundReflectionMacro( ReflectionMacro( pHeaderInfo, cr, range, ReflectionMacroType::DataFile ) );
        }

        //-------------------------------------------------------------------------

        return CXChildVisit_Continue;
    }
}