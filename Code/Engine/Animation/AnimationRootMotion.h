#pragma once
#include "Engine/_Module/API.h"
#include "AnimationFrameTime.h"
#include "Base/Math/Transform.h"
#include "Base/Types/Arrays.h"
#include "Base/Types/Color.h"
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE { class DebugDrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    struct EE_ENGINE_API RootMotionData
    {
        EE_SERIALIZE( m_transforms, m_numFrames, m_averageLinearVelocity, m_averageAngularVelocity, m_totalDelta );

    public:

        enum class SamplingMode : uint8_t
        {
            EE_REFLECT_ENUM

            Delta = 0,  // Just returns the delta between the start and end time
            WorldSpace, // Will return a delta that attempts to move the character to the expected root motion position (assumes root motion transforms are in world space)
        };

    public:

        // Is this data valid, note that there is a distinction between valid data and having actual motion
        inline bool IsValid() const { return m_numFrames > 0; }

        // Does this root motion actual move the character
        inline bool IsStationary() const { return m_transforms.size() <= 1; }

        // Clear all root motion data
        void Clear();

        // Get the number of root motion transforms
        inline int32_t GetNumFrames() const { return m_numFrames; }

        // Get the root transform at the given frame time
        Transform GetTransform( FrameTime const& frameTime ) const;

        // Get the root transform at the given percentage
        inline Transform GetTransform( Percentage percentageThrough ) const
        {
            EE_ASSERT( percentageThrough >= 0 && percentageThrough <= 1.0f );
            return GetTransform( GetFrameTime( percentageThrough ) );
        }

        // Get the end root transform
        inline Transform const& GetEndTransform() const { return m_transforms.back(); }

        // Get the delta for the root motion for the given time range. Handle's looping but assumes only a single loop occurred!
        Transform GetDelta( Percentage fromTime, Percentage toTime ) const;

        // Get the delta for the root motion for the given time range. DOES NOT SUPPORT LOOPING!
        Transform GetDeltaNoLooping( Percentage fromTime, Percentage toTime ) const;

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

        // Will return the direction of movement for that given frame towards the next frame. If there is no movement (or no next frame) it will return the facing!
        Vector GetIncomingMovementDirection2DAtFrame( int32_t frameIdx ) const;

        // Will return the direction of movement for that given frame towards the next frame. If there is no movement (or no next frame) it will return the facing!
        Quaternion GetIncomingMovementOrientation2DAtFrame( int32_t frameIdx ) const;

        // Will return the direction of movement for that given frame towards the next frame. If there is no movement (or no next frame) it will return the facing!
        Vector GetOutgoingMovementDirection2DAtFrame( int32_t frameIdx ) const;

        // Will return the direction of movement for that given frame towards the next frame. If there is no movement (or no next frame) it will return the facing!
        Quaternion GetOutgoingMovementOrientation2DAtFrame( int32_t frameIdx ) const;

        //-------------------------------------------------------------------------

        // Sample the root motion based on the supplied sampling mode: either just the plain delta or the world space delta
        Transform SampleRootMotion( SamplingMode mode, Transform const& currentWorldTransform, Percentage startTime, Percentage endTime ) const;

        // Operators and ranged for support
        //-------------------------------------------------------------------------

        // Get the transform at the specified frame index
        inline Transform& operator[] ( int32_t frameIdx )
        {
            EE_ASSERT( !m_transforms.empty() );
            if ( m_transforms.size() == 1 )
            {
                return m_transforms[0];
            }
            else
            {
                EE_ASSERT( frameIdx >= 0 && frameIdx < m_transforms.size() );
                return m_transforms[frameIdx];
            }
        }

        inline Transform const& operator[] ( int32_t frameIdx ) const { return const_cast<RootMotionData*>( this )->operator[]( frameIdx ); }

        EE_FORCE_INLINE TVector<Transform>::iterator begin() { return m_transforms.begin(); }
        EE_FORCE_INLINE TVector<Transform>::iterator end() { return m_transforms.end(); }
        EE_FORCE_INLINE TVector<Transform>::const_iterator begin() const { return m_transforms.begin(); }
        EE_FORCE_INLINE TVector<Transform>::const_iterator end() const { return m_transforms.end(); }

        Transform& front() { return m_transforms.front(); }
        Transform const& front() const { return m_transforms.front(); }
        Transform& back() { return m_transforms.back(); }
        Transform const& back() const { return m_transforms.back(); }

        // Debug
        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        void DrawDebug( DebugDrawContext& ctx, Transform const& worldTransform, Color color0 = Colors::Yellow, Color color1 = Colors::Cyan ) const;
        #endif

    private:

        inline FrameTime GetFrameTime( Percentage percentageThrough ) const { return FrameTime( percentageThrough, GetNumFrames() ); }

    public:

        TVector<Transform>                      m_transforms;
        int32_t                                 m_numFrames = 0;
        float                                   m_averageLinearVelocity = 0.0f; // In m/s
        Radians                                 m_averageAngularVelocity = 0.0f; // In rad/s, only on the X/Y plane
        Transform                               m_totalDelta;
    };
}