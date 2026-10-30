#pragma once

#include "Base/Resource/ResourceLoader.h"
#include "Engine/Render/RenderTexture.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    class RenderSystem;

    //-------------------------------------------------------------------------

    class TextureLoader final : public Resource::ResourceLoader
    {
    public:

        TextureLoader();

        inline void SetRenderSystemPtr( RenderSystem* pRenderSystem ) { EE_ASSERT( pRenderSystem != nullptr ); m_pRenderSystem = pRenderSystem; }
        inline void ClearRenderSystemPtr() { m_pRenderSystem = nullptr; }

    private:

        virtual Resource::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const override;
        virtual Resource::UnloadResult Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

    private:

        RenderSystem* m_pRenderSystem = nullptr;
    };
}
