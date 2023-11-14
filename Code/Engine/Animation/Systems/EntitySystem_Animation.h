#pragma once

#include "Engine/_Module/API.h"
#include "Engine/Entity/EntitySystem.h"

//-------------------------------------------------------------------------

namespace EE
{
    class Transform;
    class SpatialEntityComponent;
}

namespace EE::Render
{
    class SkeletalMeshComponent;
}

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphComponent;
    class AnimationClipPlayerComponent;
    class Pose;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AnimationSystem : public EntitySystem
    {
        EE_ENTITY_SYSTEM( AnimationSystem, RequiresUpdate( UpdateStage::PrePhysics, UpdatePriority::Low ), RequiresUpdate( UpdateStage::PostPhysics, UpdatePriority::Low ) );

    public:

        virtual ~AnimationSystem();

    private:

        virtual void RegisterComponent( EntityComponent* pComponent ) override;
        virtual void UnregisterComponent( EntityComponent* pComponent ) override;
        virtual void Update( EntityWorldUpdateContext const& ctx ) override;

        void UpdateAnimPlayers( EntityWorldUpdateContext const& ctx, Transform const& characterWorldTransform );
        void UpdateAnimGraphs( EntityWorldUpdateContext const& ctx, Transform const& characterWorldTransform );

        void TransferAnimationPoseToMesh( Pose const * pPose );

    private:

        TVector<AnimationClipPlayerComponent*>          m_animPlayers;
        TVector<GraphComponent*>                        m_animGraphs;
        TVector<Render::SkeletalMeshComponent*>         m_meshComponents;
        SpatialEntityComponent*                         m_pRootComponent = nullptr;
    };
}