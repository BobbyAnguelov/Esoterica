#pragma once

#include "Base/ThirdParty/imgui/imgui.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Render::RHI
{
    struct Texture;
    struct Sampler;
}

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    inline ImTextureID ImTextureID_Pack( uint16_t sampler, uint16_t texture )
    {
        union
        {
            struct
            {
                uint16_t m_sampler;
                uint16_t m_texture;
            };
            uint32_t m_packed;
        } pack;

        pack.m_sampler = sampler;
        pack.m_texture = texture;

        return pack.m_packed;
    }

    inline void ImTextureID_Unpack( ImTextureID id, uint16_t& outSampler, uint16_t& outTexture )
    {
        union
        {
            struct
            {
                uint16_t m_sampler;
                uint16_t m_pTexture;
            };
            uint32_t m_packed;
        } pack;

        pack.m_packed = id;

        outSampler = pack.m_sampler;
        outTexture = pack.m_pTexture;
    }

    EE_BASE_API ImTextureID GetImTextureID( Render::RHI::Sampler* pSampler, Render::RHI::Texture* pTexture );
}
#endif