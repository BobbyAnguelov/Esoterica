#include "ClangVisitors_Macro.h"
#include "Applications/Reflector/ReflectorSettingsAndUtils.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    CXChildVisitResult VisitMacro( ClangParserContext * pContext, HeaderID headerID, CXCursor cr, String const& cursorName )
    {
        CXSourceRange range = clang_getCursorExtent( cr );

        if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterProperty ) )
        {
            uint32_t lineNumber;
            clang_getExpansionLocation( clang_getRangeStart( range ), nullptr, &lineNumber, nullptr, nullptr );
            pContext->AddFoundRegisteredPropertyMacro( RegisteredPropertyMacro( headerID, lineNumber, false ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::ExposeProperty ) )
        {
            uint32_t lineNumber;
            clang_getExpansionLocation( clang_getRangeStart( range ), nullptr, &lineNumber, nullptr, nullptr );
            pContext->AddFoundRegisteredPropertyMacro( RegisteredPropertyMacro( headerID, lineNumber, true ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterEnum ) )
        {
            pContext->AddFoundTypeRegistrationMacro( TypeRegistrationMacro( ReflectionMacro::RegisterEnum, cr, range ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterType ) )
        {
            pContext->AddFoundTypeRegistrationMacro( TypeRegistrationMacro( ReflectionMacro::RegisterType, cr, range ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterEntityComponent ) )
        {
            pContext->AddFoundTypeRegistrationMacro( TypeRegistrationMacro( ReflectionMacro::RegisterEntityComponent, cr, range ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterSingletonEntityComponent ) )
        {
            pContext->AddFoundTypeRegistrationMacro( TypeRegistrationMacro( ReflectionMacro::RegisterSingletonEntityComponent, cr, range ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterEntitySystem ) )
        {
            pContext->AddFoundTypeRegistrationMacro( TypeRegistrationMacro( ReflectionMacro::RegisterEntitySystem, cr, range ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterModule ) )
        {
            pContext->AddFoundTypeRegistrationMacro( TypeRegistrationMacro( ReflectionMacro::RegisterModule, cr, range ) );
        }

        // Resources
        //-------------------------------------------------------------------------

        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterResource ) )
        {
            pContext->AddFoundTypeRegistrationMacro( TypeRegistrationMacro( ReflectionMacro::RegisterResource, cr, range ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterTypeResource ) )
        {
            pContext->AddFoundTypeRegistrationMacro( TypeRegistrationMacro( ReflectionMacro::RegisterTypeResource, cr, range ) );
        }
        else if ( cursorName == GetReflectionMacroText( ReflectionMacro::RegisterVirtualResource ) )
        {
            pContext->AddFoundTypeRegistrationMacro( TypeRegistrationMacro( ReflectionMacro::RegisterVirtualResource, cr, range ) );
        }

        //-------------------------------------------------------------------------

        return CXChildVisit_Continue;
    }
}