#pragma once

#include "Engine/Animation/TaskSystem/Animation_Task.h"
#include "Engine/Animation/AnimationTarget.h"

//-------------------------------------------------------------------------

namespace EE::Animation::Tasks
{
    class ChainSolverTask : public Task
    {
        EE_REFLECT_TYPE( ChainSolverTask );

    public:

        ChainSolverTask( int8_t sourceTaskIdx, int32_t effectorBoneIdx, int32_t chainLength, bool isTargetInWorldSpace, Target const& effectorTarget );
        virtual void Execute( TaskContext const& context ) override;

        virtual bool AllowsSerialization() const override { return true; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Two Bone IK"; }
        virtual Color GetDebugColor() const override { return Colors::OrangeRed; }
        virtual InlineString GetDebugTextInfo( bool isDetailedModeEnabled ) const override;
        virtual void DrawDebug( Drawing::DrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const override;
        #endif

    private:

        ChainSolverTask() : Task() {}

    private:

        int32_t         m_effectorBoneIdx = InvalidIndex;
        Transform       m_targetTransform; // Model space
        int32_t         m_chainLength;

        //-------------------------------------------------------------------------

        Target          m_effectorTarget;
        bool            m_isTargetInWorldSpace = false;
        bool            m_isRunningFromDeserializedData = false;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        StringID        m_debugEffectorBoneID;
        Transform       m_chainStartTransformMS;
        Transform       m_debugRequestedTargetTransformMS;
        float           m_debugTotalChainLength;
        #endif
    };
}