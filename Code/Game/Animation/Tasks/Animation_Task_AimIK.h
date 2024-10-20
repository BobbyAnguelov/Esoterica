#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class AimIKTask : public Task
    {
        EE_REFLECT_TYPE( AimIKTask );

    public:

        AimIKTask( int8_t sourceTaskIdx, Vector const& worldSpaceTarget );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

    private:

        AimIKTask() : Task() {}

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "AIM IK"; }
        virtual Color GetDebugColor() const override { return Colors::Cyan; }
        virtual void DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const override;
        #endif

    private:

        Vector  m_worldSpaceTarget;
        Vector  m_modelSpaceTarget;
        bool    m_generateEffectorData = true; // Whether we should calculate effector data or assume it was deserialized
    };
}