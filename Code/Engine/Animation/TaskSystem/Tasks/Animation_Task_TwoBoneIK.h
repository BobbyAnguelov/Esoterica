#pragma once

#include "Engine/Animation/TaskSystem/Animation_PoseTask.h"
#include "Engine/Animation/AnimationTarget.h"
#include "Engine/Animation/IK/IK.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class TwoBoneIKTask : public PoseTask
    {
        EE_REFLECT_TYPE( TwoBoneIKTask );

        constexpr static float const s_quantizationRangeForTranslation = 4.5;

    public:

        TwoBoneIKTask( int8_t sourceTaskIdx, int32_t effectorBoneIdx, bool isTargetInWorldSpace, Target const& effectorTarget, IKBlendMode blendMode = IKBlendMode::Effector, float blendWeight = 1.0f, float referencePoseTwistWeight = 0.0f );
        virtual void Execute( TaskContext const& context ) override;

        virtual int32_t GetNumDependencies() const override { return 1; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Two Bone IK"; }
        virtual Color GetDebugColor() const override { return Colors::OrangeRed; }
        virtual InlineString GetDebugTextInfo( bool isDetailedModeEnabled ) const override;
        virtual void DrawDebug( DebugDrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const override;
        #endif

    private:

        TwoBoneIKTask() : PoseTask() {}

    private:

        int32_t         m_effectorBoneIdx = InvalidIndex;
        int32_t         m_targetBoneIdx = InvalidIndex;
        Transform       m_targetTransform; // Model space
        float           m_referencePoseTwistWeight = 0.0f;

        //-------------------------------------------------------------------------

        Target          m_target;
        IKBlendMode     m_blendMode = IKBlendMode::Effector;
        float           m_blendWeight = 1.0f;
        bool            m_isTargetInWorldSpace = false;
        bool            m_isRunningFromDeserializedData = false;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        StringID        m_debugEffectorBoneID;
        #endif
    };
}