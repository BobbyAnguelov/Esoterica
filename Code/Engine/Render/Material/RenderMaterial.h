#pragma once

#include "Engine/_Module/API.h"
#include "Base/Render/RenderShader.h"
#include "Base/Render/RenderStates.h"
#include "Base/Render/RenderPipelineState.h"
#include "Base/Render/RenderTexture.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API Material : public Resource::IResource
    {
        EE_RESOURCE( 'mtrl', "Render Material", 1, false );
        friend class MaterialCompiler;
        friend class MaterialLoader;

        EE_SERIALIZE( m_pAlbedoTexture, m_pMetalnessTexture, m_pRoughnessTexture, m_pNormalMapTexture, m_pAOTexture, m_albedo, m_metalness, m_roughness, m_normalScaler );

    public:

        virtual bool IsValid() const override { return true; }

        inline Texture const* GetAlbedoTexture() const { EE_ASSERT( IsValid() ); return m_pAlbedoTexture.GetPtr(); }
        inline Texture const* GetMetalnessTexture() const { EE_ASSERT( IsValid() ); return m_pMetalnessTexture.GetPtr(); }
        inline Texture const* GetRoughnessTexture() const { EE_ASSERT( IsValid() ); return m_pRoughnessTexture.GetPtr(); }
        inline Texture const* GetNormalMapTexture() const { EE_ASSERT( IsValid() ); return m_pNormalMapTexture.GetPtr(); }
        inline Texture const* GetAOTexture() const { EE_ASSERT( IsValid() ); return m_pAOTexture.GetPtr(); }

        EE_FORCE_INLINE bool HasAlbedoTexture() const { return m_pAlbedoTexture.IsSet(); }
        EE_FORCE_INLINE bool HasMetalnessTexture() const { return m_pMetalnessTexture.IsSet(); }
        EE_FORCE_INLINE bool HasRoughnessTexture() const { return m_pRoughnessTexture.IsSet(); }
        EE_FORCE_INLINE bool HasNormalMapTexture() const { return m_pNormalMapTexture.IsSet(); }
        EE_FORCE_INLINE bool HasAOTexture() const { return m_pAOTexture.IsSet(); }

        EE_FORCE_INLINE Color GetAlbedoValue() const { return m_albedo; }
        EE_FORCE_INLINE float GetMetalnessValue() const { return m_metalness; }
        EE_FORCE_INLINE float GetRoughnessValue() const { return m_roughness; }
        EE_FORCE_INLINE float GetNormalScalerValue() const { return m_normalScaler; }

    private:

        TResourcePtr<Texture>   m_pAlbedoTexture;
        TResourcePtr<Texture>   m_pMetalnessTexture;
        TResourcePtr<Texture>   m_pRoughnessTexture;
        TResourcePtr<Texture>   m_pNormalMapTexture;
        TResourcePtr<Texture>   m_pAOTexture;
        Color                   m_albedo = Colors::Black;
        float                   m_metalness = 0.0f;
        float                   m_roughness = 0.0f;
        float                   m_normalScaler = 1.0f;
    };
}