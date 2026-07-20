#pragma once

#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Render/RenderMaterial.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/TypeSystem/TypeInstance.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    enum class MaterialTranslucency
    {
        EE_REFLECT_ENUM

        Opaque,
        Masked,
        AlphaBlend,
    };

    //-------------------------------------------------------------------------

    struct MaterialResourceDescriptor final : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( MaterialResourceDescriptor );

    public:

        virtual bool                  IsValid() const override { return m_shader.IsValid() && m_shaderParameters.IsSet(); }
        virtual int32_t               GetFileVersion() const override { return 0; }
        virtual bool                  IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID        GetCompiledResourceTypeID() const override { return Material::GetStaticResourceTypeID(); }
        virtual FileSystem::Extension GetExtension() const override final { return Material::GetStaticResourceTypeID().ToString(); }
        virtual char const*           GetFriendlyName() const override final { return Material::s_friendlyName; }

        virtual void GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const override
        {
            ForEachDependency( [&outDependencies]( Resource::ResourcePtr const* pResource ) { outDependencies.emplace_back( pResource->GetDataPath(), true ); } );
        }

        virtual void GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const override
        {
            ForEachDependency( [&outDependencies] ( Resource::ResourcePtr const* pResource ) { VectorEmplaceBackUnique( outDependencies, pResource->GetResourceID() ); } );
        }

        virtual void Clear() override
        {
            m_shaderParameters.CreateInstance( m_shaderParameters.GetInstanceTypeInfo() );
        }

        template<typename F>
        inline void ForEachDependency( F fn ) const
        {
            EE_ASSERT( m_shaderParameters.IsSet() );
            for ( TypeSystem::PropertyInfo const& property : m_shaderParameters.GetInstanceTypeInfo()->m_properties )
            {
                if ( property.IsResourcePtrProperty() )
                {
                    Resource::ResourcePtr const* pResource = property.GetPropertyAddress<Resource::ResourcePtr>( m_shaderParameters.Get() );
                    if ( pResource->IsSet() )
                    {
                        fn( pResource );
                    }
                }
            }
        }

    public:

        EE_REFLECT( CustomEditor = "SurfaceShaderPicker" );
        StringID m_shader;

        EE_REFLECT( DisableTypePicker );
        TypeInstance m_shaderParameters;
    };
}
