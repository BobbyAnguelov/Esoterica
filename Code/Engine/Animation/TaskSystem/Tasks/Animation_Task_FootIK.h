#include "Engine/Animation/TaskSystem/Animation_PoseTask.h"
#include "Engine/Animation/AnimationTarget.h"
#include "Engine/Animation/IK/IK.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class FootIKTask : public PoseTask
    {
        EE_REFLECT_TYPE( FootIKTask );

        constexpr static float const s_quantizationRangeForTranslation = 2.0;

    public:

        FootIKTask( int8_t sourceTaskIdx, int32_t leftFootBoneIdx, int32_t rightFootBoneIdx, bool isTargetInWorldSpace, Target const& leftTarget, Target const& rightTarget, IKBlendMode blendMode = IKBlendMode::Effector, float blendWeight = 1.0f );
        virtual void Execute( TaskContext const& context ) override;

        virtual int32_t GetNumDependencies() const override { return 1; }
        virtual void Serialize( TaskSerializer& serializer ) const override;
        virtual void Deserialize( TaskSerializer& serializer ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual char const* GetDebugName() const override { return "Foot IK"; }
        virtual Color GetDebugColor() const override { return Colors::OrangeRed; }
        virtual InlineString GetDebugTextInfo( bool isDetailedModeEnabled ) const override;
        virtual void DrawDebug( DebugDrawContext& drawingContext, Transform const& worldTransform, Skeleton::LOD lod, PoseBuffer const* pRecordedPoseBuffer, bool isDetailedViewEnabled ) const override;
        #endif

    private:

        FootIKTask() : PoseTask() {}

    private:

        int32_t         m_leftFootBoneIdx = InvalidIndex;
        int32_t         m_rightFootBoneIdx = InvalidIndex;
        int32_t         m_leftTargetBoneIdx = InvalidIndex;
        int32_t         m_rightTargetBoneIdx = InvalidIndex;
        Transform       m_leftTargetTransform; // Model space
        Transform       m_rightTargetTransform; // Model space

        //-------------------------------------------------------------------------

        Target          m_leftTarget;
        Target          m_rightTarget;
        IKBlendMode     m_blendMode = IKBlendMode::Effector;
        float           m_blendWeight = 1.0f;
        bool            m_isTargetInWorldSpace = false;
        bool            m_isRunningFromDeserializedData = false;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        StringID        m_debugLeftFootBoneID;
        StringID        m_debugRightFootBoneID;
        #endif
    };
}