#pragma once

#include "Engine/_Module/API.h"
#include "AnimationSyncTrack.h"
#include "AnimationEvent.h"
#include "AnimationRootMotion.h"
#include "AnimationSkeleton.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Math/NumericRange.h"
#include "Base/Time/Time.h"
#include "Base/Encoding/Quantization.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class Pose;
    class Event;

    //-------------------------------------------------------------------------

    struct QuantizationRange
    {
        EE_SERIALIZE( m_rangeStart, m_rangeLength );

        QuantizationRange() = default;

        QuantizationRange( float start, float length )
            : m_rangeStart( start )
            , m_rangeLength( length )
        {}

        inline bool IsValid() const { return m_rangeLength > 0; }

    public:

        float                                     m_rangeStart = 0;
        float                                     m_rangeLength = -1;
    };

    //-------------------------------------------------------------------------

    struct TrackCompressionSettings
    {
        EE_SERIALIZE( m_translationRangeX, m_translationRangeY, m_translationRangeZ, m_scaleRange, m_constantRotation, m_isRotationStatic, m_isTranslationStatic, m_isScaleStatic );

        friend class AnimationClipCompiler;

    public:

        TrackCompressionSettings() = default;

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

        QuantizationRange                       m_translationRangeX;
        QuantizationRange                       m_translationRangeY;
        QuantizationRange                       m_translationRangeZ;
        QuantizationRange                       m_scaleRange;

    private:

        Quaternion                              m_constantRotation;
        bool                                    m_isRotationStatic = false;
        bool                                    m_isTranslationStatic = false;
        bool                                    m_isScaleStatic = false;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API AnimationClip : public Resource::IResource
    {
        EE_RESOURCE( 'anim', "Animation Clip" );
        EE_SERIALIZE( m_skeleton, m_numFrames, m_duration, m_compressedPoseData2, m_compressedPoseOffsets, m_trackCompressionSettings, m_rootMotion, m_isAdditive );

        friend class AnimationClipCompiler;
        friend class AnimationClipLoader;

    private:

        EE_FORCE_INLINE static Quaternion DecodeRotation( uint16_t const* pData )
        {
            Quantization::EncodedQuaternion const encodedQuat( pData[0], pData[1], pData[2] );
            return encodedQuat.ToQuaternion();
        }

        EE_FORCE_INLINE static Vector DecodeTranslation( uint16_t const* pData, TrackCompressionSettings const& settings )
        {
            float const m_x = Quantization::DecodeFloat( pData[0], settings.m_translationRangeX.m_rangeStart, settings.m_translationRangeX.m_rangeLength );
            float const m_y = Quantization::DecodeFloat( pData[1], settings.m_translationRangeY.m_rangeStart, settings.m_translationRangeY.m_rangeLength );
            float const m_z = Quantization::DecodeFloat( pData[2], settings.m_translationRangeZ.m_rangeStart, settings.m_translationRangeZ.m_rangeLength );
            return Vector( m_x, m_y, m_z );
        }

        EE_FORCE_INLINE static float DecodeScale( uint16_t const* pData, TrackCompressionSettings const& settings )
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
        inline uint32_t GetNumFrames() const { return m_numFrames; }
        inline Seconds GetDuration() const { return m_duration; }
        inline Seconds GetTime( uint32_t frame ) const { return Seconds( GetPercentageThrough( frame ).ToFloat() * m_duration ); }
        inline Seconds GetTime( Percentage percentageThrough ) const { return Seconds( percentageThrough.ToFloat() * m_duration ); }
        inline Percentage GetPercentageThrough( uint32_t frame ) const { return Percentage( ( (float) frame ) / m_numFrames ); }
        inline FrameTime GetFrameTime( Percentage const percentageThrough ) const { return FrameTime( percentageThrough, GetNumFrames() ); }
        inline FrameTime GetFrameTime( Seconds const timeThroughAnimation ) const { return GetFrameTime( IsSingleFrameAnimation() ? Percentage( 0.0f ) : Percentage( timeThroughAnimation / m_duration ) ); }
        inline SyncTrack const& GetSyncTrack() const{ return m_syncTrack; }

        // Pose
        //-------------------------------------------------------------------------

        void GetPose( FrameTime const& frameTime, Pose* pOutPose ) const;
        inline void GetPose( Percentage percentageThrough, Pose* pOutPose ) const { GetPose( GetFrameTime( percentageThrough ), pOutPose ); }

        // Events
        //-------------------------------------------------------------------------

        // Get all the events for this animation
        inline TVector<Event*> const& GetEvents() const { return m_events; }

        // Get all the events for the specified range. This function will append the results to the output array. Handle's looping but assumes only a single loop occurred!
        inline void GetEventsForRange( Seconds fromTime, Seconds toTime, TInlineVector<Event const*, 10>& outEvents ) const;

        // Get all the events for the specified range. This function will append the results to the output array. DOES NOT SUPPORT LOOPING!
        inline void GetEventsForRangeNoLooping( Seconds fromTime, Seconds toTime, TInlineVector<Event const*, 10>& outEvents ) const;

        // Helper function that converts percentage times to actual anim times
        EE_FORCE_INLINE void GetEventsForRange( Percentage fromTime, Percentage toTime, TInlineVector<Event const*, 10>& outEvents ) const
        {
            EE_ASSERT( fromTime >= 0.0f && fromTime <= 1.0f );
            EE_ASSERT( toTime >= 0.0f && toTime <= 1.0f );
            GetEventsForRange( m_duration * fromTime, m_duration * toTime, outEvents );
        }

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

        TResourcePtr<Skeleton>                  m_skeleton;
        uint32_t                                m_numFrames = 0;
        Seconds                                 m_duration = 0.0f;
        TVector<uint16_t>                       m_compressedPoseData2;
        TVector<TrackCompressionSettings>       m_trackCompressionSettings;
        TVector<uint32_t>                       m_compressedPoseOffsets;
        TVector<Event*>                         m_events;
        SyncTrack                               m_syncTrack;
        RootMotionData                          m_rootMotion;
        bool                                    m_isAdditive = false;
    };
}

//-------------------------------------------------------------------------
// The below functions are in the header so they will be inlined
//-------------------------------------------------------------------------
// Do not move to the CPP file!!!

namespace EE::Animation
{
    inline void AnimationClip::GetEventsForRangeNoLooping( Seconds fromTime, Seconds toTime, TInlineVector<Event const*, 10>& outEvents ) const
    {
        EE_ASSERT( toTime >= fromTime );

        for ( auto const& pEvent : m_events )
        {
            // Events are stored sorted by time so as soon as we reach an event after the end of the time range, we're done
            if ( pEvent->GetStartTime() > toTime )
            {
                break;
            }

            if ( FloatRange( fromTime, toTime ).Overlaps( pEvent->GetTimeRange() ) )
            {
                outEvents.emplace_back( pEvent );
            }
        }
    }

    EE_FORCE_INLINE void AnimationClip::GetEventsForRange( Seconds fromTime, Seconds toTime, TInlineVector<Event const*, 10>& outEvents ) const
    {
        if ( fromTime <= toTime )
        {
            GetEventsForRangeNoLooping( fromTime, toTime, outEvents );
        }
        else
        {
            GetEventsForRangeNoLooping( fromTime, m_duration, outEvents );
            GetEventsForRangeNoLooping( 0, toTime, outEvents );
        }
    }
}