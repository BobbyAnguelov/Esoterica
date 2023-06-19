#pragma once

#include "RawSkeleton.h"
#include "System/Types/Arrays.h"
#include "System/Math/Matrix.h"
#include "System/Types/String.h"
#include "System/Types/StringID.h"
#include "System/Time/Time.h"
#include "System/Math/NumericRange.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets
{
    class EE_ENGINETOOLS_API RawAnimation : public RawAsset
    {

    public:

        struct TrackData
        {
            TVector<Transform>                 m_localTransforms; // Ground truth transforms
            TVector<Transform>                 m_globalTransforms; // Generated from the local transforms
            FloatRange                         m_translationValueRangeX;
            FloatRange                         m_translationValueRangeY;
            FloatRange                         m_translationValueRangeZ;
            FloatRange                         m_scaleValueRange;
        };

    public:

        RawAnimation( RawSkeleton const& skeleton ) : m_skeleton( skeleton ) {}
        virtual ~RawAnimation() = default;

        virtual bool IsValid() const override final { return m_numFrames > 0; }

        inline bool IsAdditive() const { return m_isAdditive; }
        inline Seconds GetDuration() const { return m_duration; }
        inline int32_t GetNumFrames() const { return m_numFrames; }
        inline float GetSamplingFrameRate() const { return m_samplingFrameRate; }

        inline uint32_t GetNumBones() const { return m_skeleton.GetNumBones(); }
        inline TVector<TrackData> const& GetTrackData() const { return m_tracks; }
        inline TVector<TrackData>& GetTrackData() { return m_tracks; }
        inline RawSkeleton const& GetSkeleton() const { return m_skeleton; }

        inline TVector<Transform> const& GetRootMotion() const { return m_rootTransforms; }
        inline TVector<Transform>& GetRootMotion() { return m_rootTransforms; }

        // Extract root motion and calculate transform component ranges
        void Finalize();

        // Regenerate local transforms from the global ones - This is only needed in very special circumstances
        void RegenerateLocalTransforms();

        // Additive Generation
        //-------------------------------------------------------------------------

        // Generate Additive Data
        void MakeAdditiveRelativeToSkeleton();

        // Generate Additive Data
        void MakeAdditiveRelativeToFrame( int32_t baseFrameIdx );

        // Generate Additive Data
        void MakeAdditiveRelativeToAnimation( RawAnimation const& baseAnimation );

    private:

        void CalculateComponentRanges();

    protected:

        RawSkeleton const                   m_skeleton;
        float                               m_samplingFrameRate = 0;
        Seconds                             m_duration = 0.0f;
        int32_t                             m_numFrames = 0;
        TVector<TrackData>                  m_tracks;
        TVector<Transform>                  m_rootTransforms;
        bool                                m_isAdditive = false;
    };
}