#pragma once

#include "ClangUtils.h"
#include "Applications/Reflector/TypeReflection/TypeReflection_ReflectionDatabase.h"
#include "Applications/Reflector//ReflectedMacro.h"
#include "Base/Types/HashMap.h"
#include "Base/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::Reflection
{
    class ReflectionDatabase;

    //-------------------------------------------------------------------------

    struct ParsedMacro
    {

    public:

        ParsedMacro() = default;
        ParsedMacro( ReflectedHeader const* pReflectedHeader, CXCursor cursor, CXSourceRange sourceRange, ReflectionMacro type );

        inline bool IsValid() const { return m_type != ReflectionMacro::Unknown; }
        inline bool IsModuleMacro() const { return m_type == ReflectionMacro::ReflectModule; }
        inline bool IsEnumMacro() const { return m_type == ReflectionMacro::ReflectEnum; }
        inline bool IsEntitySystemMacro() const { return m_type == ReflectionMacro::EntitySystem; }
        inline bool IsEntityWorldSystemMacro() const { return m_type == ReflectionMacro::EntityWorldSystem; }
        inline bool IsEntityComponentMacro() const { return m_type >= ReflectionMacro::EntityComponent && m_type <= ReflectionMacro::TransientSingletonEntityComponent; }
        inline bool IsDataFileMacro() const { return m_type == ReflectionMacro::DataFile; }

        // Should be registered as a type
        inline bool IsReflectedTypeMacro() const 
        { 
            return m_type == ReflectionMacro::ReflectType || IsEntitySystemMacro() || IsEntityWorldSystemMacro() || IsEntityComponentMacro() || m_type == ReflectionMacro::DataFile;
        }
        
        // Should be registered as a resource
        inline bool IsRegisteredResourceMacro() const
        { 
            return m_type == ReflectionMacro::Resource;
        }

        String GetReflectedTypeName() const;

    public:

        StringID                            m_headerID;
        uint32_t                            m_lineNumber = 0;
        uint32_t                            m_position = 0xFFFFFFFF;
        ReflectionMacro                     m_type = ReflectionMacro::Unknown;
        String                              m_macroComment;
        String                              m_macroContents;
    };

    //-------------------------------------------------------------------------

    class ClangParserContext
    {

    public:

        struct HeaderToVisit
        {
            HeaderToVisit( StringID ID, ReflectedHeader const* pReflectedHeader ) : m_ID( ID ), m_pReflectedHeader( pReflectedHeader ) {}

            inline bool operator==( StringID const& ID ) const { return m_ID == ID; }

        public:

            StringID                    m_ID;
            ReflectedHeader const*      m_pReflectedHeader;
        };

    public:

        ClangParserContext( FileSystem::Path const& solutionDirectoryPath, ReflectionDatabase* pDatabase )
            : m_pTU( nullptr )
            , m_solutionDirectoryPath( solutionDirectoryPath )
            , m_pDatabase( pDatabase )
            , m_pParentReflectedType( nullptr )
            , m_inEngineNamespace( false )
        {
            EE_ASSERT( pDatabase != nullptr );
        }

        void LogError( char const* pFormat, ... ) const;
        char const* GetErrorMessage() const { return m_errorMessage.c_str(); }
        inline bool HasErrorOccured() const { return !m_errorMessage.empty(); }

        ReflectedHeader const* GetReflectedHeader( StringID headerID ) const;

        void Reset( CXTranslationUnit* pTU );
        void PushNamespace( String const& name );
        void PopNamespace();

        bool SetModuleClassName( FileSystem::Path const& headerFilePath, String const& moduleClassName );
        inline StringID GenerateTypeID( String const& fullyQualifiedTypeName ) const { return StringID( fullyQualifiedTypeName ); }

        String const& GetCurrentNamespace() const { return m_currentNamespace; }
        bool IsInEngineNamespace() const { return m_inEngineNamespace; }
        bool IsEngineNamespace( String const& namespaceString ) const;

        void AddFoundReflectionMacro( ParsedMacro const& foundMacro );

        // Try to find a reflection macro for a property
        // If we found a macro we will remove it from the list of macros to reduce the cost of future searches
        // Returns true if a macro was found
        bool GetReflectionMacroForType( StringID headerID, CXCursor const& cr, ParsedMacro& macro );

        // Try to find a reflection macro for a property
        // If we found a macro we will remove it from the list of macros to reduce the cost of future searches
        // Returns true if a macro was found
        bool FindReflectionMacroForProperty( StringID headerID, uint32_t lineNumber, ParsedMacro& reflectionMacro );

        // Check if we have any orphaned reflection macros
        // If we have any then we will populate the error message with all the details
        bool CheckForUnhandledReflectionMacros() const;

    public:

        CXTranslationUnit*                                      m_pTU;

        // Should we do a full pass or just update the flags?
        bool                                                    m_detectDevOnlyTypesAndProperties = false;

        FileSystem::Path                                        m_solutionDirectoryPath;
        ReflectionDatabase*                                     m_pDatabase;
        TVector<HeaderToVisit>                                  m_headersToVisit;

        // The current parent/enclosing reflected type
        void*                                                   m_pParentReflectedType;
        StringID                                                m_currentHeaderID;
        FileSystem::Path                                        m_currentHeaderFilePath;

    private:

        THashMap<StringID, TVector<ParsedMacro>>                m_typeReflectionMacros;
        THashMap<StringID, TVector<ParsedMacro>>                m_propertyReflectionMacros;

        mutable String                                          m_errorMessage;
        TVector<String>                                         m_namespaceStack;
        TVector<String>                                         m_structureStack;
        String                                                  m_currentNamespace;
        bool                                                    m_inEngineNamespace;
    };
}