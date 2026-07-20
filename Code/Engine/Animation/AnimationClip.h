#pragma once

#include "Engine/_Module/API.h"
#include "AnimationSyncTrack.h"
#include "AnimationEvent.h"
#include "AnimationRootMotion.h"
#include "AnimationSkeleton.h"
#include "AnimationPose.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Math/NumericRange.h"
#include "Base/Time/Time.h"
#include "Base/Encoding/Quantization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Event;

    //-------------------------------------------------------------------------

    struct TrackDefinition
    {
        EE_SERIALIZE( m_translationRangeX, m_translationRangeY, m_translationRangeZ, m_scaleRange, m_trackReadOffset, m_constantRotation, m_isRotationStatic, m_isTranslationStatic, m_isScaleStatic );

        friend class AnimationClipCompiler;

    public:

        TrackDefinition() = default;

        // Is the rotation for this track static i.e. a fixed value for the duration of the animation
        inline bool IsRotationTrackStatic() const { return m_isRotationStatic; }

        EE_FORCE_INLINE Quaternion GetStaticRotationValue() const { return m_constantRotation; }

        // Is the translation for this track static i.e. a fixed value for the duration of the animation
        inline bool IsTranslationTrackStatic() const { return m_isTranslationStatic; }

        EE_FORCE_INLINE Float3 GetStaticTranslationValue() const { return Float3( m_translationRangeX.m_rangeStart, m_translationRangeY.m_rangeStart, m_translationRangeZ.m_rangeStart ); }

        // Is the scale for this track static i.e. a fixed value for the duration of the animation
        inline bool IsScaleTrackStatic() const { return m_isScaleStatic; }

        EE_FORCE_INLINE float GetStaticScaleValue() const { return m_scaleRange.m_rangeStart; }

    public:

        Quantization::FloatRange                m_translationRangeX;
        Quantization::FloatRange                m_translationRangeY;
        Quantization::FloatRange                m_translationRangeZ;
        Quantization::FloatRange                m_scaleRange;
        int32_t                                 m_trackReadOffset = 0; // Offset into compressed pose data for this track (in terms of uint16s)

    private:

        Quaternion                              m_constantRotation;
        bool                                    m_isRotationStatic = false;
        bool                                    m_isTranslationStatic = false;
        bool                                    m_isScaleStatic = false;
    };

    //-------------------------------------------------------------------------

    struct FloatCurveDefinition
    {
        EE_SERIALIZE( m_ID, m_range, m_isStatic );

        EE_FORCE_INLINE float GetStaticValue() const { return m_range.m_rangeStart; }

    public:

        StringID                                m_ID;
        Quantization::FloatRange                m_range;
        bool                                    m_isStatic = false;
    };

    //-------------------------------------------------------------------------

    struct ModelSpaceSamplingChainLink
    {
        EE_SERIALIZE( m_boneIdx, m_parentBoneIdx, m_parentChainLinkIdx );

        int32_t m_boneIdx = InvalidIndex;
        int32_t m_parentBoneIdx = InvalidIndex;
        int32_t m_parentChainLinkIdx = InvalidIndex;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AnimationClip : public Resource::IResource
    {
        EE_RESOURCE( "anim", "Animation Clip", Colors::Orchid, 66, false );
        EE_SERIALIZE( m_skeleton, m_numFrames, m_duration, m_compressedPoseData, m_compressedPoseOffsets, m_trackDefs, m_rootMotion, m_isAdditive, m_modelSpaceSamplingChain, m_modelSpaceBoneSamplingIndices, m_compressedFloatCurveData, m_compressedFloatCurveOffsets, m_floatCurveDefs );

        friend class AnimationClipCompiler;
        friend class AnimationClipLoader;

    private:

        EE_FORCE_INLINE static Quaternion DecodeRotation( uint16_t const* pData )
        {
            Quantization::EncodedQuaternion const encodedQuat( pData[0], pData[1], pData[2] );
            return encodedQuat.ToQuaternion();
        }

        EE_FORCE_INLINE static Vector DecodeTranslation( uint16_t const* pData, TrackDefinition const& settings )
        {
            float const m_x = Quantization::DecodeFloat( pData[0], settings.m_translationRangeX.m_rangeStart, settings.m_translationRangeX.m_rangeLength );
            float const m_y = Quantization::DecodeFloat( pData[1], settings.m_translationRangeY.m_rangeStart, settings.m_translationRangeY.m_rangeLength );
            float const m_z = Quantization::DecodeFloat( pData[2], settings.m_translationRangeZ.m_rangeStart, settings.m_translationRangeZ.m_rangeLength );
            return Vector( m_x, m_y, m_z );
        }

        EE_FORCE_INLINE static float DecodeScale( uint16_t const* pData, TrackDefinition const& settings )
        {
            return Quantization::DecodeFloat( pData[0], settings.m_scaleRange.m_rangeStart, settings.m_scaleRange.m_rangeLength );
        }

    public:

        AnimationClip() = default;

        virtual bool IsValid() const final { return m_skeleton != nullptr && m_skeleton.IsLoaded() && m_numFrames > 0; }
        inline Skeleton const* GetSkeleton() const { return m_skeleton.GetPtr(); }
        inline int32_t GetNumBones() const { EE_ASSERT( m_skeleton != nullptr ); return m_skeleton->GetNumBones(); }

        // Animation Info
        //-------------------------------------------------------------------------

        inline bool IsSingleFrameAnimation() const { return m_numFrames == 1; }
        inline bool IsAdditive() const { return m_isAdditive; }
        inline float GetFPS() const { return IsSingleFrameAnimation() ? 0 : float( m_numFrames - 1 ) / m_duration; }
        inline int32_t GetNumFrames() const { return m_numFrames; }
        inline Seconds GetDuration() const { return m_duration; }
        inline Seconds GetTime( int32_t frame ) const { return Seconds( GetPercentageThrough( frame ).ToFloat() * m_duration ); }
        inline Seconds GetTime( Percentage percentageThrough ) const { return Seconds( percentageThrough.ToFloat() * m_duration ); }
        inline Percentage GetPercentageThrough( int32_t frame ) const { return IsSingleFrameAnimation() ? Percentage( 1.0f ) : Percentage( ( (float) frame ) / ( m_numFrames - 1 ) ); }
        Percentage GetPercentageThrough( FrameTime const &frameTime ) const;
        inline FrameTime GetFrameTime( Percentage const percentageThrough ) const { return FrameTime( percentageThrough, GetNumFrames() ); }
        inline FrameTime GetFrameTime( Seconds const timeThroughAnimation ) const { return GetFrameTime( IsSingleFrameAnimation() ? Percentage( 0.0f ) : Percentage( timeThroughAnimation / m_duration ) ); }
        inline SyncTrack const& GetSyncTrack() const{ return m_syncTrack; }

        // Pose
        //-------------------------------------------------------------------------

        void GetPose( FrameTime const& frameTime, Pose* pOutPose, Skeleton::LOD lod = Skeleton::LOD::High, bool sampleFloatChannels = true ) const;
        inline void GetPose( Percentage percentageThrough, Pose* pOutPose, Skeleton::LOD lod = Skeleton::LOD::High, bool sampleFloatChannels = true ) const { GetPose( GetFrameTime( percentageThrough ), pOutPose, lod, sampleFloatChannels ); }

        // Get a single parent space transform
        Transform GetParentSpaceTransform( FrameTime const& frameTime, int32_t boneIdx ) const;

        // Get a single model space transform, this will return the entire chain from root to the specified bone
        TInlineVector<BoneChainElement, 20> GetModelSpaceTransform( FrameTime const& frameTime, int32_t boneIdx ) const;

        // Secondary Animations
        //-------------------------------------------------------------------------

        // Does this animation have any secondary animations attached
        bool HasSecondaryAnimations() const { return !m_secondaryAnimations.empty(); }

        // Get the number of secondary animations attached
        int32_t GetNumSecondaryAnimations() const { return (int32_t) m_secondaryAnimations.size(); }

        // Get the secondary animations
        TInlineVector<AnimationClip const*, 1> const& GetSecondaryAnimations() const { return m_secondaryAnimations; }

        // Try get an secondary animation for a specified skeleton
        AnimationClip const* GetSecondaryAnimation( Skeleton const* pSkeleton ) const
        {
            for ( auto pSecondaryAnimation : m_secondaryAnimations )
            {
                if ( pSecondaryAnimation->GetSkeleton() == pSkeleton )
                {
                    return pSecondaryAnimation; 
                }
            }
            return nullptr;
        }

        // Get the set of secondary skeletons this animation animates
        TInlineVector<Skeleton const*,1> GetSecondarySkeletons() const;

        // Events
        //-------------------------------------------------------------------------

        // Get all the events for this animation
        inline TVector<Event*> const& GetEvents() const { return m_events; }

        // Get all the events for the specified range [fromTime, toTime). This function will append the results to the output array.
        // Note that the trailing edge is not exclusive except for the case where the 'toTime' == 1
        // WARNING: DOES NOT SUPPORT LOOPING!
        void GetEventsForRange( Percentage fromTime, Percentage toTime, TInlineVector<Event const*, 10>& outEvents ) const;

        // Root motion
        //-------------------------------------------------------------------------

        // Get the raw root motion data
        EE_FORCE_INLINE RootMotionData const& GetRootMotion() const { return m_rootMotion; }

        // Get the root transform at the given frame time
        EE_FORCE_INLINE Transform GetRootTransform( FrameTime const& frameTime ) const { return m_rootMotion.GetTransform( frameTime ); }

        // Get the root transform at the given percentage
        EE_FORCE_INLINE Transform GetRootTransform( Percentage percentageThrough ) const { return m_rootMotion.GetTransform( percentageThrough ); }

        // Get the delta for the root motion for the given time range. Handle's looping but assumes only a single loop occurred!
        EE_FORCE_INLINE Transform GetRootMotionDelta( Percentage fromTime, Percentage toTime ) const { return m_rootMotion.GetDelta( fromTime, toTime ); }

        // Get the delta for the root motion for the given time range. DOES NOT SUPPORT LOOPING!
        EE_FORCE_INLINE Transform GetRootMotionDeltaNoLooping( Percentage fromTime, Percentage toTime ) const { return m_rootMotion.GetDeltaNoLooping( fromTime, toTime ); }

        // Get the average linear velocity of the root for this animation
        EE_FORCE_INLINE float GetAverageLinearVelocity() const { return m_rootMotion.m_averageLinearVelocity; }

        // Get the average angular velocity (in the X/Y plane) of the root for this animation
        EE_FORCE_INLINE Radians GetAverageAngularVelocity() const { return m_rootMotion.m_averageAngularVelocity; }

        // Get the total root motion delta for this animation
        EE_FORCE_INLINE Transform const& GetTotalRootMotionDelta() const { return m_rootMotion.m_totalDelta; }

        // Get the distance traveled by this animation
        EE_FORCE_INLINE Vector const& GetDisplacementDelta() const { return m_rootMotion.m_totalDelta.GetTranslation(); }

        // Get the rotation delta for this animation
        EE_FORCE_INLINE Quaternion const& GetRotationDelta() const { return m_rootMotion.m_totalDelta.GetRotation(); }

    private:

        void GetParentSpaceTransform( FrameTime const& frameTime, int32_t boneIdx, Transform& outTransform ) const;

    private:

        TResourcePtr<Skeleton>                      m_skeleton;
        int32_t                                     m_numFrames = 0;
        Seconds                                     m_duration = 0.0f;

        TVector<TrackDefinition>                    m_trackDefs;
        TVector<uint16_t>                           m_compressedPoseData;
        TVector<uint32_t>                           m_compressedPoseOffsets;

        TVector<FloatCurveDefinition>               m_floatCurveDefs;
        TVector<uint16_t>                           m_compressedFloatCurveData;
        TVector<uint32_t>                           m_compressedFloatCurveOffsets;

        TVector<Event*>                             m_events;
        TInlineVector<AnimationClip const*,1>       m_secondaryAnimations;
        TInlineVector<FloatChannelData, 2>          m_floatChannelSetData;
        SyncTrack                                   m_syncTrack;
        bool                                        m_isAdditive = false;
        RootMotionData                              m_rootMotion;

        // The sub-skeleton that we need to calculate model space bone transforms
        TVector<ModelSpaceSamplingChainLink>        m_modelSpaceSamplingChain;
        TVector<int32_t>                            m_modelSpaceBoneSamplingIndices; // The bones that need to be blended in model space (these are indices into the 'm_modelSpaceSamplingBoneChain' array)
    };
}