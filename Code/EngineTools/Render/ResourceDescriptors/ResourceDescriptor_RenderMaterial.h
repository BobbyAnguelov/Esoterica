#pragma once

#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "Base/Render/RenderTexture.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    struct MaterialResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( MaterialResourceDescriptor );

        virtual bool IsValid() const override { return m_albedoTexture.IsSet(); }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return Material::GetStaticResourceTypeID(); }
        virtual void GetCompileDependencies( TVector<ResourcePath>& outDependencies ) override {}

        virtual void Clear() override
        {
            m_albedoTexture.Clear();
            m_metalnessTexture.Clear();
            m_roughnessTexture.Clear();
            m_normalMapTexture.Clear();
            m_aoTexture.Clear();
            m_albedo = Colors::Black;
            m_metalness = 0.0f;
            m_roughness = 0.0f;
            m_normalScaler = 1.0f;
        }

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