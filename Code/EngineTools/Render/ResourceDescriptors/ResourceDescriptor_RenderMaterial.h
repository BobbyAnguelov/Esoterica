#pragma once

#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "System/Render/RenderTexture.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct MaterialResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( MaterialResourceDescriptor );

        virtual bool IsValid() const override { return m_albedoTexture.IsSet(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return Material::GetStaticResourceTypeID(); }
        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override {}

    public:

        EE_REFLECT() TResourcePtr<Texture>       m_albedoTexture;
        EE_REFLECT() TResourcePtr<Texture>       m_metalnessTexture;
        EE_REFLECT() TResourcePtr<Texture>       m_roughnessTexture;
        EE_REFLECT() TResourcePtr<Texture>       m_normalMapTexture;
        EE_REFLECT() TResourcePtr<Texture>       m_aoTexture;
        EE_REFLECT() Color                       m_albedo = Colors::Black;
        EE_REFLECT() float                       m_metalness = 0.0f;
        EE_REFLECT() float                       m_roughness = 0.0f;
        EE_REFLECT() float                       m_normalScaler = 1.0f;
    };
}