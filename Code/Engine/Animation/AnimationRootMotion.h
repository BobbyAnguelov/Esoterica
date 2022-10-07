#pragma once
#include "Engine/_Module/API.h"
#include "AnimationFrameTime.h"
#include "System/Math/Transform.h"
#include "System/Types/Arrays.h"
#include "System/Types/Color.h"

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINE_API RootMotionData
    {
        EE_SERIALIZE( m_transforms, m_averageLinearVelocity, m_averageAngularVelocity, m_totalDelta );

    public:

        inline bool IsValid() const { return !m_transforms.empty(); }

        void Clear();

        // Get the number of root motion transforms
        inline int32_t GetNumFrames() const { return (int32_t) m_transforms.size(); }

        // Get the root transform at the given frame time
        Transform GetTransform( FrameTime const& frameTime ) const;

        // Get the root transform at the given percentage
        inline Transform GetTransform( Percentage percentageThrough ) const
        {
            EE_ASSERT( percentageThrough >= 0 && percentageThrough <= 1.0f );
            return GetTransform( GetFrameTime( percentageThrough ) );
        }

        // Get the delta for the root motion for the given time range. Handle's looping but assumes only a single loop occurred!
        inline Transform GetDelta( Percentage fromTime, Percentage toTime ) const;

        // Get the delta for the root motion for the given time range. DOES NOT SUPPORT LOOPING!
        inline Transform GetDeltaNoLooping( Percentage fromTime, Percentage toTime ) const;

        // Get the average linear velocity of the root for this animation
        inline float GetAverageLinearVelocity() const { return m_averageLinearVelocity; }

        // Get the average angular velocity (in the X/Y plane) of the root for this animation
        inline Radians GetAverageAngularVelocity() const { return m_averageAngularVelocity; }

        // Get the total root motion delta for this animation
        inline Transform const& GetTotalDelta() const { return m_totalDelta; }

        // Get the distance traveled by this animation
        inline Vector const& GetDisplacementDelta() const { return m_totalDelta.GetTranslation(); }

        // Get the rotation delta for this animation
        inline Quaternion const& GetRotationDelta() const { return m_totalDelta.GetRotation(); }

        // Will return the heading (direction of movement for that given frame towards the next frame). If there is no movement (or no next frame) it will return the facing!
        Vector GetIncomingHeadingDirection2DAtFrame( int32_t frameIdx ) const;

        // Will return the heading (direction of movement for that given frame towards the next frame). If there is no movement (or no next frame) it will return the facing!
        Quaternion GetIncomingHeadingOrientation2DAtFrame( int32_t frameIdx ) const;

        // Will return the heading (direction of movement for that given frame towards the next frame). If there is no movement (or no next frame) it will return the facing!
        Vector GetOutgoingHeadingDirection2DAtFrame( int32_t frameIdx ) const;

        // Will return the heading (direction of movement for that given frame towards the next frame). If there is no movement (or no next frame) it will return the facing!
        Quaternion GetOutgoingHeadingOrientation2DAtFrame( int32_t frameIdx ) const;

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( Drawing::DrawContext& ctx, Transform const& worldTransform, Color color0 = Colors::Yellow, Color color1 = Colors::Cyan ) const;
        #endif

    private:

        inline FrameTime GetFrameTime( Percentage percentageThrough ) const { return FrameTime( percentageThrough, GetNumFrames() ); }

    public:

        TVector<Transform>                      m_transforms;
        float                                   m_averageLinearVelocity = 0.0f; // In m/s
        Radians                                 m_averageAngularVelocity = 0.0f; // In rad/s, only on the X/Y plane
        Transform                               m_totalDelta;
    };

    //-------------------------------------------------------------------------

    inline Transform RootMotionData::GetTransform( FrameTime const& frameTime ) const
    {
        EE_ASSERT( IsValid() );
        EE_ASSERT( frameTime.GetFrameIndex() < m_transforms.size() );

        Transform displacementTransform;

        if ( m_transforms.empty() )
        {
            displacementTransform = Transform::Identity;
        }
        else
        {
            if ( frameTime.IsExactlyAtKeyFrame() )
            {
                displacementTransform = m_transforms[frameTime.GetFrameIndex()];
            }
            else // Read interpolated transform
            {
                Transform const& frameStartTransform = m_transforms[frameTime.GetFrameIndex()];
                Transform const& frameEndTransform = m_transforms[frameTime.GetFrameIndex() + 1];
                displacementTransform = Transform::Slerp( frameStartTransform, frameEndTransform, frameTime.GetPercentageThrough() );
            }
        }

        return displacementTransform;
    }

    EE_FORCE_INLINE Transform RootMotionData::GetDeltaNoLooping( Percentage fromTime, Percentage toTime ) const
    {
        Transform const startTransform = GetTransform( fromTime );
        Transform const endTransform = GetTransform( toTime );
        return Transform::DeltaNoScale( startTransform, endTransform );
    }

    EE_FORCE_INLINE Transform RootMotionData::GetDelta( Percentage fromTime, Percentage toTime ) const
    {
        Transform delta( NoInit );
        if ( fromTime <= toTime )
        {
            delta = GetDeltaNoLooping( fromTime, toTime );
        }
        else
        {
            Transform const preLoopDelta = GetDeltaNoLooping( fromTime, 1.0f );
            Transform const postLoopDelta = GetDeltaNoLooping( 0.0f, toTime );
            delta = postLoopDelta * preLoopDelta;
        }

        return delta;
    }
}