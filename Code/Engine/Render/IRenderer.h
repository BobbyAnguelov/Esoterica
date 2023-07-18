#pragma once

#include "Engine/_Module/API.h"
#include "Base/Esoterica.h"
#include "Base/Encoding/Hash.h"
#include "Base/Time/Time.h"
#include "Base/Types/UUID.h"

//-------------------------------------------------------------------------

namespace EE { class EntityWorld; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Viewport;
    class RenderTarget;

    //-------------------------------------------------------------------------

    // This is a basic set of priority values to help order any registered renderers
    enum RendererPriorityLevel
    {
        Game = 0,
        GameUI = 100,
        Debug = 200,
        DevelopmentTools = 300
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API IRenderer
    {

    public:

        virtual ~IRenderer() = default;
        virtual uint32_t GetRendererID() const = 0;
        virtual int32_t GetPriority() const = 0;

        virtual void RenderWorld( Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget, EntityWorld* pWorld ) {};
        virtual void RenderViewport( Seconds const deltaTime, Viewport const& viewport, RenderTarget const& renderTarget ) {};
    };
}

//-------------------------------------------------------------------------

#define EE_RENDERER_ID( typeName, priority ) \
constexpr static uint32_t const RendererID = Hash::FNV1a::GetHash32( #typeName ); \
virtual uint32_t GetRendererID() const override final { return typeName::RendererID; } \
virtual int32_t GetPriority() const override{ return priority; }