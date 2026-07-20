#pragma once

#include "Engine/Animation/TaskSystem/Animation_PoseTask.h"
//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AimIKTask : public PoseTask
    {
        EE_REFLECT_TYPE( AimIKTask );

    public:

        AimIKTask( int8_t sourceTaskIdx, Vector const& worldSpaceTarget );
        virtual void Execute( TaskContext const& context ) override;

        virtual int32_t GetNumDependencies() const override { return 1; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

    private:

        AimIKTask() : PoseTask() {}

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "AIM IK"; }
        virtual Color GetDebugColor() const override { return Colors::Cyan; }
        virtual void DrawDebug( DebugDrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const override;
        #endif

    private:

        Vector  m_worldSpaceTarget;
        Vector  m_modelSpaceTarget;
        bool    m_generateEffectorData = true; // Whether we should calculate effector data or assume it was deserialized
    };
}