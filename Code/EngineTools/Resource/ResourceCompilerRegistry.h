#pragma once
#include "Base/Types/Arrays.h"
#include "ResourceCompiler.h"

//-------------------------------------------------------------------------

namespace EE::TypeSystem { class TypeRegistry; }

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_ENGINETOOLS_API CompilerRegistry
    {
    public:

        CompilerRegistry( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& rawResourceDirectoryPath );
        ~CompilerRegistry();

        //-------------------------------------------------------------------------

        inline TVector<Compiler const*> const& GetRegisteredCompilers() const { return m_compilers; }

        inline bool HasCompilerForResourceType( ResourceTypeID const& typeID ) const { return m_compilerTypeMap.find( typeID ) != m_compilerTypeMap.end(); }

        inline Compiler const* GetCompilerForResourceType( ResourceTypeID const& typeID ) const
        {
            auto compilerTypeIter = m_compilerTypeMap.find( typeID );
            if ( compilerTypeIter != m_compilerTypeMap.end() )
            {
                return compilerTypeIter->second;
            }

            return nullptr;
        }

        inline int32_t GetVersionForType( ResourceTypeID const& typeID ) const
        {
            auto pCompiler = GetCompilerForResourceType( typeID );
            EE_ASSERT( pCompiler != nullptr );
            return pCompiler->GetVersion( typeID );
        }

    private:

        void RegisterCompiler( Compiler const* pCompiler );
        void UnregisterCompiler( Compiler const* pCompiler );

    private:

        TVector<Compiler const*>                            m_compilers;
        THashMap<ResourceTypeID, Compiler const*>           m_compilerTypeMap;
    };
}