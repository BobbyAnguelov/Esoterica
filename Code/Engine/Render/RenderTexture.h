#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Render/RenderAsyncResourceUpdate.h"
#include "Base/Render/RHI.h"
#include "Base/Resource/IResource.h"
#include "Base/Serialization/BinarySerialization.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class EE_ENGINE_API TextureResource final : public Resource::IResource
    {
        EE_RESOURCE( "texture", "Texture Resource", Colors::DarkCyan, 19, true );

        EE_SERIALIZE( m_width,
                      m_height,
                      m_depth,
                      m_arrayLayers,
                      m_mipLevels,
                      m_rhiDataFormat );

        friend class TextureLoader;
        friend class TextureCompiler;

    public:

        inline RHI::Texture* GetTexture() const { return m_pTexture; }
        inline uint32_t GetWidth() const { return m_width; }
        inline uint32_t GetHeight() const { return m_height; }
        inline uint32_t GetDepth() const { return m_depth; }
        inline uint32_t GetArrayLayers() const { return m_arrayLayers; }
        inline uint32_t GetMipLevels() const { return m_mipLevels; }
        inline uint32_t GetFormat() const { return m_rhiDataFormat; }

        virtual bool IsValid() const override { return m_pTexture != nullptr; }

    private:

        RHI::Texture*                   m_pTexture = nullptr;

        uint32_t                        m_width = 0;
        uint32_t                        m_height = 0;
        uint32_t                        m_depth = 0;
        uint32_t                        m_arrayLayers = 0;
        uint32_t                        m_mipLevels = 0;
        uint32_t                        m_rhiDataFormat = 0;

    private:

        AsyncTextureUpdate*             m_pAsyncTextureUpdate = nullptr;
    };
}
