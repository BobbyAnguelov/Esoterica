#pragma once

#include "ClangUtils.h"
#include "Applications/Reflector/ReflectorSettingsAndUtils.h"
#include "Applications/Reflector/Database/ReflectionProjectTypes.h"
#include "System/Types/HashMap.h"
#include "System/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class ReflectionDatabase;

    //-------------------------------------------------------------------------

    struct ReflectionMacro
    {

    public:

        ReflectionMacro() = default;
        ReflectionMacro( HeaderInfo const* pHeaderInfo, CXCursor cursor, CXSourceRange sourceRange, ReflectionMacroType type );

        inline bool IsValid() const { return m_type != ReflectionMacroType::Unknown; }
        inline bool IsModuleMacro() const { return m_type == ReflectionMacroType::ReflectModule; }
        inline bool IsEnumMacro() const { return m_type == ReflectionMacroType::ReflectEnum; }
        inline bool IsEntitySystemMacro() const { return m_type == ReflectionMacroType::EntitySystem; }
        inline bool IsEntityWorldSystemMacro() const { return m_type == ReflectionMacroType::EntityWorldSystem; }
        inline bool IsEntityComponentMacro() const { return m_type == ReflectionMacroType::EntityComponent || m_type == ReflectionMacroType::SingletonEntityComponent; }

        // Should be registered as a type
        inline bool IsReflectedTypeMacro() const 
        { 
            return m_type == ReflectionMacroType::ReflectType || IsEntitySystemMacro() || IsEntityWorldSystemMacro() || IsEntityComponentMacro() || m_type == ReflectionMacroType::ReflectedResource;
        }
        
        // Should be registered as a resource
        inline bool IsRegisteredResourceMacro() const
        { 
            return m_type == ReflectionMacroType::Resource || m_type == ReflectionMacroType::ReflectedResource; 
        }

    public:

        HeaderID            m_headerID;
        uint32_t            m_lineNumber = 0;
        uint32_t            m_position = 0xFFFFFFFF;
        ReflectionMacroType m_type = ReflectionMacroType::Unknown;
        String              m_macroComment;
        String              m_macroContents;
    };

    //-------------------------------------------------------------------------

    class ClangParserContext
    {

    public:

        struct HeaderToVisit
        {
            HeaderToVisit( HeaderID ID, HeaderInfo const* pHeaderInfo ) : m_ID( ID ), m_pHeaderInfo( pHeaderInfo ) {}

            inline bool operator==( HeaderID const& ID ) const { return m_ID == ID; }

        public:

            HeaderID            m_ID;
            HeaderInfo const*   m_pHeaderInfo;
        };

    public:

        ClangParserContext( SolutionInfo* pSolution, ReflectionDatabase* pDatabase )
            : m_pTU( nullptr )
            , m_pDatabase( pDatabase )
            , m_pParentReflectedType( nullptr )
            , m_inEngineNamespace( false )
            , m_pSolution( pSolution )
        {
            EE_ASSERT( pSolution != nullptr && pDatabase != nullptr );
        }

        void LogError( char const* pFormat, ... ) const;
        char const* GetErrorMessage() const { return m_errorMessage.c_str(); }
        inline bool HasErrorOccured() const { return !m_errorMessage.empty(); }

        HeaderInfo const* GetHeaderInfo( HeaderID headerID ) const;

        void Reset( CXTranslationUnit* pTU );
        void PushNamespace( String const& name );
        void PopNamespace();

        bool SetModuleClassName( FileSystem::Path const& headerFilePath, String const& moduleClassName );
        inline StringID GenerateTypeID( String const& fullyQualifiedTypeName ) const { return StringID( fullyQualifiedTypeName ); }

        String const& GetCurrentNamespace() const { return m_currentNamespace; }
        bool IsInEngineNamespace() const { return m_inEngineNamespace; }
        bool IsEngineNamespace( String const& namespaceString ) const;

        void AddFoundReflectionMacro( ReflectionMacro const& foundMacro );

        // Try to find a reflection macro for a property
        // If we found a macro we will remove it from the list of macros to reduce the cost of future searches
        // Returns true if a macro was found
        bool GetReflectionMacroForType( HeaderID headerID, CXCursor const& cr, ReflectionMacro& macro );

        // Try to find a reflection macro for a property
        // If we found a macro we will remove it from the list of macros to reduce the cost of future searches
        // Returns true if a macro was found
        bool FindReflectionMacroForProperty( HeaderID headerID, uint32_t lineNumber, ReflectionMacro& reflectionMacro );

        // Check if we have any orphaned reflection macros
        // If we have any then we will populate the error message with all the details
        bool CheckForOrphanedReflectionMacros() const;

    public:

        CXTranslationUnit*                                      m_pTU;

        // Should we do a full pass or just update the flags?
        bool                                                    m_detectDevOnlyTypesAndProperties = false;

        SolutionInfo*                                           m_pSolution;
        ReflectionDatabase*                                     m_pDatabase;
        TVector<HeaderToVisit>                                  m_headersToVisit;

        // The current parent/enclosing reflected type
        void*                                                   m_pParentReflectedType;

    private:

        THashMap<HeaderID, TVector<ReflectionMacro>>            m_typeReflectionMacros;
        THashMap<HeaderID, TVector<ReflectionMacro>>            m_propertyReflectionMacros;

        mutable String                                          m_errorMessage;
        TVector<String>                                         m_namespaceStack;
        TVector<String>                                         m_structureStack;
        String                                                  m_currentNamespace;
        bool                                                    m_inEngineNamespace;
    };
}