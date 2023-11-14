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
    class EE_ENGINETOOLS_API ImportedAnimation : public ImportedData
    {

    public:

        struct TrackData
        {
            TVector<Transform>                 m_localTransforms; // Ground truth transforms
            TVector<Transform>                 m_globalTransforms; // Generated from the local transforms
        };

    public:

        ImportedAnimation( ImportedSkeleton const& skeleton ) : m_skeleton( skeleton ) {}
        virtual ~ImportedAnimation() = default;

        virtual bool IsValid() const override final { return m_numFrames > 0; }

        inline bool IsAdditive() const { return m_isAdditive; }
        inline Seconds GetDuration() const { return m_duration; }
        inline int32_t GetNumFrames() const { return m_numFrames; }
        inline float GetSamplingFrameRate() const { return m_samplingFrameRate; }

        inline uint32_t GetNumBones() const { return m_skeleton.GetNumBones(); }
        inline TVector<TrackData> const& GetTrackData() const { return m_tracks; }
        inline TVector<TrackData>& GetTrackData() { return m_tracks; }
        inline ImportedSkeleton const& GetSkeleton() const { return m_skeleton; }

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
        void MakeAdditiveRelativeToAnimation( ImportedAnimation const& baseAnimation );

    protected:

        ImportedSkeleton const              m_skeleton;
        float                               m_samplingFrameRate = 0;
        Seconds                             m_duration = 0.0f;
        int32_t                             m_numFrames = 0;
        TVector<TrackData>                  m_tracks;
        TVector<Transform>                  m_rootTransforms;
        bool                                m_isAdditive = false;
    };
}