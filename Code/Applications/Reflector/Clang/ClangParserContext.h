#pragma once

#include "ClangUtils.h"
#include "Applications/Reflector/ReflectorSettingsAndUtils.h"
#include "Applications/Reflector/Database/ReflectionProjectTypes.h"
#include "System/Types/StringID.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem::Reflection
{
    class ReflectionDatabase;

    //-------------------------------------------------------------------------

    struct TypeRegistrationMacro
    {

    public:

        TypeRegistrationMacro()
            : m_type( ReflectionMacro::Unknown )
            , m_position( 0xFFFFFFFF )
        {}

        TypeRegistrationMacro( ReflectionMacro type, CXCursor cursor, CXSourceRange sourceRange );

        inline bool IsValid() const { return m_type != ReflectionMacro::Unknown; }
        inline bool IsModuleMacro() const { return m_type == ReflectionMacro::RegisterModule; }
        inline bool IsEnumMacro() const { return m_type == ReflectionMacro::RegisterEnum; }
        inline bool IsEntitySystemMacro() const { return m_type == ReflectionMacro::RegisterEntitySystem; }
        
        inline bool IsEntityComponentMacro() const 
        {
            return m_type == ReflectionMacro::RegisterEntityComponent || m_type == ReflectionMacro::RegisterSingletonEntityComponent; 
        }

        // Should be registered as a type
        inline bool IsRegisteredTypeMacro() const 
        { 
            return m_type == ReflectionMacro::RegisterType || IsEntitySystemMacro() || IsEntityComponentMacro() || m_type == ReflectionMacro::RegisterTypeResource; 
        }
        
        // Should be registered as a resource
        inline bool IsRegisteredResourceMacro() const
        { 
            return m_type == ReflectionMacro::RegisterResource || m_type == ReflectionMacro::RegisterVirtualResource || m_type == ReflectionMacro::RegisterTypeResource; 
        }

    public:

        uint32_t              m_position;
        ReflectionMacro     m_type;
        String              m_macroContents;
    };

    //-------------------------------------------------------------------------

    struct RegisteredPropertyMacro
    {
        RegisteredPropertyMacro( HeaderID ID, uint32_t line, bool isExposed = false )
            : m_headerID( ID )
            , m_lineNumber( line )
            , m_isExposed( isExposed )
        {}

        bool operator==( RegisteredPropertyMacro const& rhs )
        {
            return m_headerID == rhs.m_headerID && m_lineNumber == rhs.m_lineNumber;
        }

        bool operator!=( RegisteredPropertyMacro const& rhs )
        {
            return m_headerID != rhs.m_headerID || m_lineNumber != rhs.m_lineNumber;
        }

    public:

        HeaderID            m_headerID;
        uint32_t              m_lineNumber;
        bool                m_isExposed = false;
    };

    //-------------------------------------------------------------------------

    class ClangParserContext
    {

    public:

        ClangParserContext( SolutionInfo* pSolution, ReflectionDatabase* pDatabase )
            : m_pTU( nullptr )
            , m_pDatabase( pDatabase )
            , m_pCurrentEntry( nullptr )
            , m_inEngineNamespace( false )
            , m_pSolution( pSolution )
        {
            EE_ASSERT( pSolution != nullptr && pDatabase != nullptr );
        }

        void LogError( char const* pFormat, ... ) const;
        char const* GetErrorMessage() const { return m_errorMessage.c_str(); }
        inline bool ErrorOccured() const { return !m_errorMessage.empty(); }

        bool ShouldVisitHeader( HeaderID headerID ) const;

        void Reset( CXTranslationUnit* pTU );
        void PushNamespace( String const& name );
        void PopNamespace();

        bool SetModuleClassName( FileSystem::Path const& headerFilePath, String const& moduleClassName );
        inline StringID GenerateTypeID( String const& fullyQualifiedTypeName ) const { return StringID( fullyQualifiedTypeName ); }

        String const& GetCurrentNamespace() const { return m_currentNamespace; }
        bool IsInEngineNamespace() const { return m_inEngineNamespace; }
        bool IsEngineNamespace( String const& namespaceString ) const;

        // Type Registration
        void AddFoundTypeRegistrationMacro( TypeRegistrationMacro const& foundMacro ) { m_typeRegistrationMacros.push_back( foundMacro ); }
        void AddFoundRegisteredPropertyMacro( RegisteredPropertyMacro const& foundMacro ) { m_registeredPropertyMacros.push_back( foundMacro ); }
        bool ShouldRegisterType( CXCursor const& cr, TypeRegistrationMacro* pMacro = nullptr );

    public:

        CXTranslationUnit*                  m_pTU;

        // Should we do a full pass or just update the flags?
        bool                                m_detectDevOnlyTypesAndProperties = false;

        // Per solution
        SolutionInfo*                       m_pSolution;
        ReflectionDatabase*                 m_pDatabase;
        TVector<HeaderID>                   m_headersToVisit;

        // Per Translation unit
        void*                               m_pCurrentEntry;
        TVector<RegisteredPropertyMacro>    m_registeredPropertyMacros;
        TVector<TypeRegistrationMacro>      m_typeRegistrationMacros;

    private:

        mutable String                      m_errorMessage;
        TVector<String>                     m_namespaceStack;
        TVector<String>                     m_structureStack;
        String                              m_currentNamespace;
        bool                                m_inEngineNamespace;
    };
}