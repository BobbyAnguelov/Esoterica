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

        constexpr static float const s_quantizationRanges[4] = { 4.5, 51, 127, 254 }; // 4.5m ( 0.078mm precision ), 51m ( 0.78mm precision), 127m ( 1.93mm precision), 254m ( 3.87mm precision)

        enum class Precision_t
        {
            High = 0,
            Medium,
            Low,
            VeryLow
        };

        inline static float GetQuantizationRange( Precision_t precision )
        {
            return s_quantizationRanges[(uint8_t) precision];
        }

        inline static Precision_t GetPrecisionForDistanceSq( float distanceSq )
        {
            constinit static float const s_quantizationRangesSq[4] = { Math::Sqr( s_quantizationRanges[0] ), Math::Sqr( s_quantizationRanges[1] ), Math::Sqr( s_quantizationRanges[2] ), Math::Sqr( s_quantizationRanges[3] ) };

            EE_ASSERT( distanceSq >= 0 );

            for ( int32_t i = 0; i < 4; i++ )
            {
                if ( distanceSq < s_quantizationRangesSq[i] )
                {
                    return (Precision_t) i;
                }
            }

            EE_UNREACHABLE_CODE();
            return Precision_t::VeryLow;
        }


    public:

        TwoBoneIKTask( int8_t sourceTaskIdx, int32_t effectorBoneIdx, bool isTargetInWorldSpace, Target const& effectorTarget, IKBlendMode blendMode = IKBlendMode::Effector, float blendWeight = 1.0f, float chainRotationWeight = 0.0f );
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

        //-------------------------------------------------------------------------

        Target          m_effectorTarget;
        IKBlendMode     m_blendMode = IKBlendMode::Effector;
        float           m_blendWeight = 1.0f;
        bool            m_isTargetInWorldSpace = false;
        bool            m_isRunningFromDeserializedData = false;
        float           m_chainRotationWeight = 0.0f;
        Precision_t     m_precision = Precision_t::High;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        StringID        m_debugEffectorBoneID;
        #endif
    };
}