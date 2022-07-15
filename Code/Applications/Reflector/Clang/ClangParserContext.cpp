#include "ClangParserContext.h"
#include "Applications/Reflector/ReflectorSettingsAndUtils.h"
#include "EASTL/algorithm.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    static void CalculateFullNamespace( TVector<String> const& namespaceStack, String& fullNamespace )
    {
        fullNamespace.clear();
        for ( auto& str : namespaceStack )
        {
            fullNamespace.append( str );
            fullNamespace.append( "::" );
        }
    }

    //-------------------------------------------------------------------------

    TypeRegistrationMacro::TypeRegistrationMacro( ReflectionMacro type, CXCursor cursor, CXSourceRange sourceRange )
        : m_type( type )
        , m_position( sourceRange.begin_int_data )
    {
        EE_ASSERT( m_type < ReflectionMacro::NumMacros );

        if ( type == ReflectionMacro::RegisterTypeResource || type == ReflectionMacro::RegisterResource || type == ReflectionMacro::RegisterVirtualResource )
        {
            CXTranslationUnit translationUnit = clang_Cursor_getTranslationUnit( cursor );

            // Read the entire source macro text
            //-------------------------------------------------------------------------

            CXToken* tokens = nullptr;
            uint32_t numTokens = 0;
            clang_tokenize( translationUnit, sourceRange, &tokens, &numTokens );
            for ( uint32_t n = 0; n < numTokens; n++ )
            {
                m_macroContents += ClangUtils::GetString( clang_getTokenSpelling( translationUnit, tokens[n] ) );
            }
            clang_disposeTokens( translationUnit, tokens, numTokens );

            // Only keep text between '(' && ')'
            //-------------------------------------------------------------------------

            auto const startIdx = m_macroContents.find_first_of( '(' );
            auto const endIdx = m_macroContents.find_last_of( ')' );
            if ( startIdx != String::npos && endIdx != String::npos && endIdx > startIdx )
            {
                m_macroContents = m_macroContents.substr( startIdx + 1, endIdx - startIdx - 1 );
            }
            else
            {
                m_macroContents.clear();
            }
        }
    }

    //-------------------------------------------------------------------------

    void ClangParserContext::LogError( char const* pErrorFormat, ... ) const
    {
        char buffer[1024];

        va_list args;
        va_start( args, pErrorFormat );
        VPrintf( buffer, 1024, pErrorFormat, args );
        va_end( args );

        m_errorMessage = buffer;
    }

    bool ClangParserContext::ShouldVisitHeader( HeaderID headerID ) const
    {
        return eastl::find( m_headersToVisit.begin(), m_headersToVisit.end(), headerID ) != m_headersToVisit.end();
    }

    void ClangParserContext::Reset( CXTranslationUnit* pTU )
    {
        EE_ASSERT( m_namespaceStack.empty() );
        EE_ASSERT( m_structureStack.empty() );

        m_pTU = pTU;
        m_registeredPropertyMacros.clear();
        m_typeRegistrationMacros.clear();
        m_inEngineNamespace = false;
        m_errorMessage.clear();
    }

    void ClangParserContext::PushNamespace( String const& name )
    {
        m_namespaceStack.push_back( name );
        CalculateFullNamespace( m_namespaceStack, m_currentNamespace );

        // On Root namespace
        if ( m_namespaceStack.size() == 1 )
        {
            m_inEngineNamespace = ( name == Reflection::Settings::g_engineNamespace );
        }
    }

    void ClangParserContext::PopNamespace()
    {
        m_namespaceStack.pop_back();
        CalculateFullNamespace( m_namespaceStack, m_currentNamespace );

        // Reset engine namespace flag
        if ( m_namespaceStack.empty() )
        {
            m_inEngineNamespace = false;
        }
    }

    bool ClangParserContext::SetModuleClassName( FileSystem::Path const& headerFilePath, String const& moduleClassName )
    {
        for ( auto& prj : m_pSolution->m_projects )
        {
            if ( headerFilePath.IsUnderDirectory( prj.m_path ) )
            {
                EE_ASSERT( prj.m_moduleClassName.empty() );
                prj.m_moduleClassName = moduleClassName;
                return true;
            }
        }

        return false;
    }

    bool ClangParserContext::IsEngineNamespace( String const& namespaceString ) const
    {
        return namespaceString == Reflection::Settings::g_engineNamespace;
    }

    //-------------------------------------------------------------------------

    bool ClangParserContext::ShouldRegisterType( CXCursor const& cr, TypeRegistrationMacro* pMacro )
    {
        CXSourceRange const typeRange = clang_getCursorExtent( cr );

        // Check the registration map for this type
        for ( auto iter = m_typeRegistrationMacros.begin(); iter != m_typeRegistrationMacros.end(); ++iter )
        {
            bool const macroWithinCursorExtents = iter->m_position > typeRange.begin_int_data && iter->m_position < typeRange.end_int_data;
            if ( macroWithinCursorExtents )
            {
                if ( pMacro != nullptr )
                {
                    *pMacro = *iter;
                }

                m_typeRegistrationMacros.erase( iter );
                return true;
            }
        }

        return false;
    }
}
