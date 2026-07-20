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

        inline void SetRenderSystem( RenderSystem* pRenderSystem )
        {
            EE_ASSERT( !m_pRenderSystem );
            m_pRenderSystem = pRenderSystem;
        }

    private:

        virtual Resource::LoadResult Load( ResourceID const& resourceID, FileSystem::Path const& resourcePath, Resource::ResourceRecord* pResourceRecord, Serialization::BinaryInputArchive* pArchive ) const override;
        virtual Resource::UnloadResult Unload( ResourceID const& resourceID, Resource::ResourceRecord* pResourceRecord ) const override;

    private:

        RenderSystem* m_pRenderSystem = nullptr;
    };
}
