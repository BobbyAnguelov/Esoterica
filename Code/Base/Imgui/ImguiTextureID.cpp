#include "ImguiTextureID.h"
#include "Base/Render/RHI.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    ImTextureID GetImTextureID( Render::RHI::Sampler* pSampler, Render::RHI::Texture* pTexture )
    {
        Render::RHI::SamplerStateHandle samplerHandle = Render::RHI::GetSamplerStateHandle( pSampler );
        Render::RHI::TextureHandle textureHandle = Render::RHI::GetTextureHandle( pTexture, Render::RHI::DescriptorTypeFlags::Texture, 0 );
        return ImTextureID_Pack( samplerHandle, textureHandle );
    }
}
#endif