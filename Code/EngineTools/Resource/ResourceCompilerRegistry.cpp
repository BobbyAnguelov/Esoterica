#include "ResourceCompilerRegistry.h"
#include "Base/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    CompilerRegistry::CompilerRegistry( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& rawResourceDirectoryPath )
    {
        TVector<TypeSystem::TypeInfo const*> compilerTypes = typeRegistry.GetAllDerivedTypes( Compiler::GetStaticTypeID(), false, false, true );

        for ( auto pCompilerType : compilerTypes )
        {
            auto pCreatedCompiler = Cast<Compiler>( pCompilerType->CreateType() );
            pCreatedCompiler->Initialize( typeRegistry, rawResourceDirectoryPath );
            m_compilers.emplace_back( pCreatedCompiler );
            RegisterCompiler( pCreatedCompiler );
        }
    }

    CompilerRegistry::~CompilerRegistry()
    {
        for ( auto& pCompiler : m_compilers )
        {
            UnregisterCompiler( pCompiler );
            EE::Delete( pCompiler );
        }

        EE_ASSERT( m_compilerTypeMap.empty() );
    }

    void CompilerRegistry::RegisterCompiler( Compiler const* pCompiler )
    {
        EE_ASSERT( pCompiler != nullptr );
        EE_ASSERT( VectorContains( m_compilers, pCompiler ) );

        //-------------------------------------------------------------------------

        auto const& resourceTypes = pCompiler->GetOutputTypes();
        for ( auto& type : resourceTypes )
        {
            // Two compilers registering for the same resource type is not allowed
            EE_ASSERT( m_compilerTypeMap.find( type ) == m_compilerTypeMap.end() );
            m_compilerTypeMap.insert( TPair<ResourceTypeID, Resource::Compiler const*>( type, pCompiler ) );
        }
    }

    void CompilerRegistry::UnregisterCompiler( Compiler const* pCompiler )
    {
        EE_ASSERT( pCompiler != nullptr );
        EE_ASSERT( VectorContains( m_compilers, pCompiler ) );

        //-------------------------------------------------------------------------

        auto const& resourceTypes = pCompiler->GetOutputTypes();
        for ( auto& type : resourceTypes )
        {
            m_compilerTypeMap.erase( type );
        }
    }
}