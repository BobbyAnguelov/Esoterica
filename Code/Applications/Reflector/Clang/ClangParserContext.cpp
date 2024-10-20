#include "ClangParserContext.h"
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

    // TODO: Support block comments
    static String TryToParseMacro( TVector<String> const& fileContents, int32_t parsedMacroLineNumber )
    {
        // Clang seems to have one less line of code in its parsed data
        int32_t const lineNumber = parsedMacroLineNumber - 1;

        //-------------------------------------------------------------------------

        String macroComment;

        auto SanitizeCommentString = [] ( String& comment )
        {
            StringUtils::RemoveAllOccurrencesInPlace( comment, "\r" );
            comment.ltrim();
            comment.rtrim();
        };

        // Check same line as the macro
        //-------------------------------------------------------------------------
        // This takes precedence over comments placed above

        size_t const sameLineFoundPos = fileContents[lineNumber].find_first_of( "//" );
        if ( sameLineFoundPos != String::npos )
        {
            if ( !macroComment.empty() )
            {
                macroComment += "\\n";
            }

            macroComment = fileContents[lineNumber].substr( sameLineFoundPos + 2, fileContents[lineNumber].length() - sameLineFoundPos - 2 );
            SanitizeCommentString( macroComment );
            return macroComment;
        }

        // Check lines directly above the macro
        //-------------------------------------------------------------------------
        // TODO: check for other macros, etc....

        if ( lineNumber > 0 )
        {
            size_t const foundCommentPos = fileContents[lineNumber - 1].find_first_of( "//" );
            if ( foundCommentPos != String::npos )
            {
                macroComment = fileContents[lineNumber - 1].substr( foundCommentPos + 2, fileContents[lineNumber - 1].length() - foundCommentPos - 2 );
                SanitizeCommentString( macroComment );
                return macroComment;
            }
        }

        //-------------------------------------------------------------------------

        return macroComment;
    }

    //-------------------------------------------------------------------------

    ReflectionMacro::ReflectionMacro( HeaderInfo const* pHeaderInfo, CXCursor cursor, CXSourceRange sourceRange, ReflectionMacroType type )
        : m_headerID( pHeaderInfo->m_ID )
        , m_type( type )
        , m_position( sourceRange.begin_int_data )
    {
        EE_ASSERT( m_type < ReflectionMacroType::NumMacros );

        clang_getExpansionLocation( clang_getRangeStart( sourceRange ), nullptr, &m_lineNumber, nullptr, nullptr );

        //-------------------------------------------------------------------------

        m_macroComment = TryToParseMacro( pHeaderInfo->m_fileContents, m_lineNumber );

        //-------------------------------------------------------------------------

        if ( m_type == ReflectionMacroType::ReflectType || m_type == ReflectionMacroType::ReflectProperty || type == ReflectionMacroType::Resource || type == ReflectionMacroType::DataFile )
        {
            // Read the contents of the macro
            //-------------------------------------------------------------------------

            CXToken* tokens = nullptr;
            uint32_t numTokens = 0;
            CXTranslationUnit translationUnit = clang_Cursor_getTranslationUnit( cursor );
            clang_tokenize( translationUnit, sourceRange, &tokens, &numTokens );
            for ( uint32_t n = 0; n < numTokens; n++ )
            {
                m_macroContents += ClangUtils::GetString( clang_getTokenSpelling( translationUnit, tokens[n] ) );
            }
            clang_disposeTokens( translationUnit, tokens, numTokens );

            // Check if we have a metadata macro
            //-------------------------------------------------------------------------

            size_t const startIdx = m_macroContents.find_first_of( "(" );
            size_t const endIdx = m_macroContents.find_last_of( ')' );
            if ( startIdx != String::npos && endIdx != String::npos && endIdx > startIdx )
            {
                // Property macro contents are JSON, so apply some enclosing formatting
                if ( type == ReflectionMacroType::ReflectProperty )
                {
                    m_macroContents = m_macroContents.substr( startIdx + 1, endIdx - startIdx - 1 );
                }
                else // Just keep the contents without the braces
                {
                    m_macroContents = m_macroContents.substr( startIdx + 1, endIdx - startIdx - 1 );
                }
            }
            else
            {
                m_macroContents.clear();
            }
        }
    }

    String ReflectionMacro::GetReflectedTypeName() const
    {
        String typeName;

        switch ( m_type )
        {
            case ReflectionMacroType::ReflectProperty:
            case ReflectionMacroType::ReflectType:
            case ReflectionMacroType::EntityComponent:
            case ReflectionMacroType::SingletonEntityComponent:
            case ReflectionMacroType::EntitySystem:
            case ReflectionMacroType::EntityWorldSystem:
            {
                typeName = m_macroContents;
            }
            break;

            case ReflectionMacroType::DataFile:
            {
                size_t const endIdx = m_macroContents.find( ',', 0 );
                typeName = m_macroContents.substr( 0, endIdx );
            }
            break;

            default:
            {
                EE_UNREACHABLE_CODE();
            }
            break;
        }

        return typeName;
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

    HeaderInfo const* ClangParserContext::GetHeaderInfo( StringID headerID ) const
    {
        auto foundIter = eastl::find( m_headersToVisit.begin(), m_headersToVisit.end(), headerID );
        if ( foundIter != m_headersToVisit.end() )
        {
            return foundIter->m_pHeaderInfo;
        }

        return nullptr;
    }

    void ClangParserContext::Reset( CXTranslationUnit* pTU )
    {
        EE_ASSERT( m_namespaceStack.empty() );
        EE_ASSERT( m_structureStack.empty() );

        m_pTU = pTU;
        m_propertyReflectionMacros.clear();
        m_typeReflectionMacros.clear();
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
        for ( auto& prj : m_pDatabase->GetReflectedProjects() )
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

    void ClangParserContext::AddFoundReflectionMacro( ReflectionMacro const& foundMacro )
    {
        EE_ASSERT( foundMacro.m_headerID.IsValid() );
        EE_ASSERT( foundMacro.m_type != ReflectionMacroType::Unknown );

        if ( foundMacro.m_type == ReflectionMacroType::ReflectProperty )
        {
            TVector<ReflectionMacro>& macrosForHeader = m_propertyReflectionMacros[foundMacro.m_headerID];
            macrosForHeader.push_back( foundMacro );
        }
        else // All other types
        {
            TVector<ReflectionMacro>& macrosForHeader = m_typeReflectionMacros[foundMacro.m_headerID];
            macrosForHeader.push_back( foundMacro );
        }
    }

    bool ClangParserContext::GetReflectionMacroForType( StringID headerID, CXCursor const& cr, ReflectionMacro& macro )
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------

        auto headerIter = m_typeReflectionMacros.find( headerID );
        if ( headerIter == m_typeReflectionMacros.end() )
        {
            return false;
        }

        TVector<ReflectionMacro>& macrosForHeader = headerIter->second;

        // Check the header macros
        //-------------------------------------------------------------------------

        CXSourceRange const typeRange = clang_getCursorExtent( cr );

        for ( auto iter = macrosForHeader.begin(); iter != macrosForHeader.end(); ++iter )
        {
            bool const macroWithinCursorExtents = iter->m_position > typeRange.begin_int_data && iter->m_position < typeRange.end_int_data;
            if ( macroWithinCursorExtents )
            {
                macro = *iter;
                macrosForHeader.erase( iter );
                return true;
            }
        }

        return false;
    }

    bool ClangParserContext::FindReflectionMacroForProperty( StringID headerID, uint32_t lineNumber, ReflectionMacro& reflectionMacro )
    {
        // Try get macros for this header
        //-------------------------------------------------------------------------

        auto headerIter = m_propertyReflectionMacros.find( headerID );
        if ( headerIter == m_propertyReflectionMacros.end() )
        {
            return false;
        }

        TVector<ReflectionMacro>& macrosForHeader = headerIter->second;

        // Try to find the macro
        //-------------------------------------------------------------------------

        TVector<ReflectionMacro>::iterator foundMacros[2] = { macrosForHeader.end(), macrosForHeader.end() };

        bool hasMacroOnLineAbove = false;
        bool hasMacroOnSameLine = false;

        for ( auto iter = macrosForHeader.begin(); iter != macrosForHeader.end(); ++iter )
        {
            if ( iter->m_lineNumber == lineNumber - 1 )
            {
                foundMacros[0] = iter;
                hasMacroOnLineAbove = true;
            }
            else if ( iter->m_lineNumber == lineNumber )
            {
                foundMacros[1] = iter;
                hasMacroOnSameLine = true;
                break; // Macros on the same line take priority!
            }
        }

        // If we have a macro on the same line as well as the line above this is a mistake in the source file
        if ( hasMacroOnLineAbove && hasMacroOnSameLine )
        {
            LogError( "Multiple reflection macros detected for the same property on line %d in file: %s", lineNumber, headerID.c_str() );
            return false;
        }

        // We didnt find a macro
        if ( !hasMacroOnLineAbove && !hasMacroOnSameLine )
        {
            return false;
        }

        TVector<ReflectionMacro>::iterator foundMacroIter = hasMacroOnLineAbove ? foundMacros[0] : foundMacros[1];
        reflectionMacro = *foundMacroIter;
        macrosForHeader.erase( foundMacroIter );
        return true;
    }

    bool ClangParserContext::CheckForUnhandledReflectionMacros() const
    {
        EE_ASSERT( !HasErrorOccured() );

        bool hasUnprocessedMacro = false;

        //-------------------------------------------------------------------------

        for ( auto& macroHeaderPair : m_typeReflectionMacros )
        {
            for ( auto& macro : macroHeaderPair.second )
            {
                m_errorMessage += String( String::CtorSprintf(), "    Unprocessed Macro Detected: %s:%d\n", macro.m_headerID.c_str(), macro.m_lineNumber );
                hasUnprocessedMacro = true;
            }
        }

        //-------------------------------------------------------------------------

        for ( auto& macroHeaderPair : m_propertyReflectionMacros )
        {
            for ( auto& macro : macroHeaderPair.second )
            {
                m_errorMessage += String( String::CtorSprintf(), "    Unprocessed Macro Detected: %s:%d\n", macro.m_headerID.c_str(), macro.m_lineNumber );
                hasUnprocessedMacro = true;
            }
        }

        //-------------------------------------------------------------------------

        return hasUnprocessedMacro;
    }
}
