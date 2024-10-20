#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationTarget.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class TwoBoneIKTask : public Task
    {
        EE_REFLECT_TYPE( TwoBoneIKTask );

    public:

        TwoBoneIKTask( int8_t sourceTaskIdx, int32_t effectorBoneIdx, float allowedStretchPercentage, bool isTargetInWorldSpace, Target const& effectorTarget );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Two Bone IK"; }
        virtual Color GetDebugColor() const override { return Colors::OrangeRed; }
        virtual InlineString GetDebugTextInfo() const override;
        virtual void DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const override;
        #endif

    private:

        TwoBoneIKTask() : Task() {}

    private:

        int32_t         m_effectorBoneIdx = InvalidIndex;
        float           m_allowedStretchPercentage = 0.0f;
        Target          m_effectorTarget;

        Transform       m_effectorTargetTransformCS;
        bool            m_isTargetInWorldSpace = false;
        bool            m_isRunningFromDeserializedData = false;

        #if EE_DEVELOPMENT_TOOLS
        Transform       m_debugRequestedTargetTransformCS;
        Transform       m_debugChainStartTransformCS;
        StringID        m_debugEffectorBoneID;
        float           m_debugTotalChainLength;
        #endif
    };
}