#pragma once

#include "Engine/_Module/API.h"
#include "Base/Resource/ResourcePtr.h"
#include "Engine/Render/RenderTexture.h"
#include "Engine/Render/Shaders/EngineShader.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API Material final : public Resource::IResource
    {
        EE_RESOURCE( "material", "Render Material", Colors::Aquamarine, 6, false );

        friend class MaterialCompiler;
        friend class MaterialLoader;

        EE_SERIALIZE( m_shaderID, m_parameterStorage );

    public:

        virtual bool IsValid() const override { return true; }

        EE_FORCE_INLINE int32_t GetShaderIndex() const { return m_shaderIndex; }
        EE_FORCE_INLINE MaterialShaderParametersInstance& GetShaderParametersInstance() { return m_shaderParametersInstance; }
        EE_FORCE_INLINE uint32_t GetShaderParametersOffsetIn32ByteBlocks() const { return m_shaderParametersInstance.GetBufferOffsetIn32ByteBlocks(); }

    private:

        template<typename T>
        struct StoredMaterialParameter
        {
            EE_SERIALIZE( m_parameterID, m_parameterValue );

            StringID                        m_parameterID;
            T                               m_parameterValue;
        };

        struct MaterialParameterStorage
        {
            EE_SERIALIZE( m_textures,
                          m_scalars,
                          m_vectors2,
                          m_vectors4,
                          m_matrices );

            TVector<StoredMaterialParameter<TResourcePtr<TextureResource>>> m_textures;
            TVector<StoredMaterialParameter<uint32_t>>                      m_scalars;
            TVector<StoredMaterialParameter<Int2>>                          m_vectors2;
            TVector<StoredMaterialParameter<Int4>>                          m_vectors4;
            TVector<StoredMaterialParameter<Matrix>>                        m_matrices;
        };

    private:

        StringID                            m_shaderID;
        MaterialParameterStorage            m_parameterStorage;

        // Not serialized - need to resolve at runtime
        int32_t                             m_shaderIndex = -1;
        MaterialShaderParametersInstance    m_shaderParametersInstance;

    private:

        AsyncMaterialParametersUpdate*      m_pMaterialParametersUpdate = nullptr;
    };
}
