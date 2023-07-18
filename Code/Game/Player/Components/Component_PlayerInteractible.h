#pragma once

#include "Game/_Module/API.h"
#include "Engine/Entity/EntitySpatialComponent.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Player
{
    class EE_GAME_API PlayerInteractibleComponent : public SpatialEntityComponent
    {
        EE_ENTITY_COMPONENT( PlayerInteractibleComponent );

    public:

        Animation::GraphVariation const* GetGraph() const { return m_pGraph.GetPtr(); }

    private:

        EE_REFLECT() TResourcePtr<Animation::GraphVariation> m_pGraph;
    };
}