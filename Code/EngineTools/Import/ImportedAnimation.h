#pragma once

#include "ImportedSkeleton.h"
#include "Base/Types/Arrays.h"
#include "Base/Math/Matrix.h"
#include "Base/Types/String.h"
#include "Base/Types/StringID.h"
#include "Base/Time/Time.h"
#include "Base/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    struct EE_ENGINETOOLS_API AnimationClip
    {

    public:

        struct TrackData
        {
            TVector<Transform>                  m_parentSpaceTransforms;
            TVector<Transform>                  m_modelSpaceTransforms;
        };

        struct FloatChannelData
        {
            inline bool HasValue() const { return !m_values.empty(); }
            inline bool IsStatic() const { return m_values.size() == 1; }

        public:

            StringID                            m_ID;
            TVector<float>                      m_values; // Note: single value if static else value per frame.
        };

    public:

        AnimationClip( Skeleton const& skeleton ) : m_skeleton( skeleton ) {}

        inline int32_t GetNumFrames() const { return m_tracks.empty() ? -1 : (int32_t) m_tracks[0].m_parentSpaceTransforms.size(); }

        inline Skeleton const& GetSkeleton() const { return m_skeleton; }

        // Get the number of bones for the skeleton
        inline uint32_t GetNumBones() const { return m_skeleton.GetNumBones(); }

        // Get the raw animation data
        inline TVector<TrackData> const& GetTrackData() const { return m_tracks; }

        // Get the raw animation data
        inline TVector<TrackData>& GetTrackData() { return m_tracks; }

        // Get the number of float channel sets
        inline uint32_t GetNumFloatChannels() const { return (int32_t) m_floatChannels.size(); }

        // Get the raw float channel sets data
        inline TVector<FloatChannelData> const& GetFloatChannelSetsData() const { return m_floatChannels; }

        // Get the raw float channel sets data
        inline TVector<FloatChannelData>& GetFloatChannelSetsData() { return m_floatChannels; }

        // Truncate this clip the specified number of frame, the new number MUST be less than the current number of frames
        void TruncateLength( int32_t numRequiredFrames );

        // Create the model space transforms from the parent space ones
        void GenerateModelSpaceTransforms();

        // Recreate the parent space transforms from the model space ones
        void RecreateParentSpaceTransformsFromModelSpaceTransform();

        // Generate Additive Data
        void MakeAdditiveRelativeToSkeleton();

        // Generate Additive Data
        void MakeAdditiveRelativeToFrame( int32_t baseFrameIdx );

        // Generate Additive Data
        void MakeAdditiveRelativeToAnimation( AnimationClip const& baseClip, int32_t baseFrameIdx = InvalidIndex );

        // Sanitize all transforms - basically make all near zero values exactly zero
        void SanitizeAllTransforms();

    public:

        Skeleton const              m_skeleton;
        TVector<TrackData>                  m_tracks;
        TVector<FloatChannelData>           m_floatChannels;
        bool                                m_hasData = true;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API Animation : public ImportedData
    {

    public:

        Animation( Skeleton const& primarySkeleton ) : m_primaryClip( primarySkeleton ) {}
        virtual ~Animation() = default;

        virtual bool IsValid() const override final { return m_numFrames > 0; }

        inline bool IsAdditive() const { return m_isAdditive; }
        inline Seconds GetDuration() const { return m_duration; }
        inline int32_t GetNumFrames() const { return m_numFrames; }
        inline float GetSamplingFrameRate() const { return m_samplingFrameRate; }

        inline TVector<Transform> const& GetRootMotion() const { return m_rootTransforms; }
        inline TVector<Transform>& GetRootMotion() { return m_rootTransforms; }

        inline AnimationClip const& GetPrimaryClip() const { return m_primaryClip; }
        inline AnimationClip& GetPrimaryClip() { return m_primaryClip; }

        inline int32_t GetNumSecondaryClips() const { return int32_t( m_secondaryClips.size() ); }
        inline AnimationClip const& GetSecondaryClip( int32_t clipIdx ) const { return m_secondaryClips[clipIdx]; }
        inline AnimationClip& GetSecondaryClip( int32_t clipIdx ) { return m_secondaryClips[clipIdx]; }

        // Extract root motion, generate model space transforms and sanitize all transforms
        void Finalize();

        // Regenerate local transforms from the global ones - This is only needed in very special circumstances
        void RecreateParentSpaceTransformsFromModelSpaceTransform();

        // Additive Generation
        //-------------------------------------------------------------------------

        // Generate Additive Data
        void MakeAdditiveRelativeToSkeleton();

        // Generate Additive Data
        void MakeAdditiveRelativeToFrame( int32_t baseFrameIdx );

        // Generate Additive Data
        void MakeAdditiveRelativeToAnimation( Animation const& baseAnimation, int32_t baseFrameIdx = InvalidIndex );

    protected:

        AnimationClip               m_primaryClip;
        TVector<AnimationClip>      m_secondaryClips;
        float                               m_samplingFrameRate = 0;
        Seconds                             m_duration = 0.0f;
        int32_t                             m_numFrames = 0;
        TVector<Transform>                  m_rootTransforms;
        bool                                m_isAdditive = false;
    };
}