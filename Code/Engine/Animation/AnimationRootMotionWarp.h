#pragma once

#include "AnimationRootMotion.h"
#include "Events/AnimationEvent_Warp.h"
#include "Base/Types/Severity.h"
#include "Base/Types/String.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class AnimationClip;
    class RootMotionWarpContext;

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    class IRootMotionWarpDebugData
    {
    public:

        virtual ~IRootMotionWarpDebugData() {}
        virtual void DrawDebug( RootMotionWarpContext const& context, DebugDrawContext& drawCtx ) const = 0;
    };
    #endif

    // Root Motion Warp Context
    //-------------------------------------------------------------------------
    // Context used for root motion warping
    // Before it can be used it needs to be initialized with root motion, sections, target transform, start transform and start time
    //
    // Example usage:
    //
    //  RootMotionWarpContext warpContext;
    //  warpContext.InitializeFromClip( pClip );
    //  warpContext.SetStartTime( startTime );
    //  warpContext.SetStartTransform( startTransform );
    //  warpContext.SetTargetTransform( targetTransform );
    //
    //  bool const success = RootMotionWarper::Warp( warpContext, parameters )
    //

    class EE_ENGINE_API RootMotionWarpContext
    {
        friend class RootMotionWarper;
        friend class TargetWarpNode;

    public:

        struct Capability
        {
            bool                                m_canWarpXY = false;
            bool                                m_canWarpZ = false;
            bool                                m_canWarpRotation = false;
        };

        struct Section
        {
            int32_t                             m_startFrame = 0;
            int32_t                             m_endFrame = 0;
            TargetWarpRule                      m_warpRule = TargetWarpRule::WarpXYZ;
            TargetWarpAlgorithm                 m_translationAlgorithm = TargetWarpAlgorithm::Bezier;
        };

    public:

        RootMotionWarpContext() = default;
        ~RootMotionWarpContext();

        // Equivalent to the default construction plus a call to InitializeFromClip
        RootMotionWarpContext( AnimationClip const& clip );

        // Initializes the context from the clip's root motion and warp events
        void InitializeFromClip( AnimationClip const& clip );

        // Initializes the context from the root motion and sections provided. Sections are assumed to be ordered and non-overlapping
        void InitializeFromRootMotion( RootMotionData const& rootMotion, Section const* pSections, int32_t sectionCount );

        // Was this context initialized either from a clip or from a root motion
        bool IsInitialized() const;

        // Fully resets the context. Needs to be initialized again.
        void Clear();

        TVector<Section> const& GetSections() const { return m_sections; }

        // Start time is the time from which the warp is performed. Sections will be clipped by it
        void SetStartTime( FrameTime const& startTime ) { m_startTime = startTime; }
        FrameTime const& GetStartTime() const { return m_startTime; }

        // Transform we want the warp to align the root motion with ( world space )
        void SetTargetTransform( Transform const& targetTransform ) { m_targetTransform = targetTransform; }
        Transform const& GetTargetTransform() const { return m_targetTransform; }

        // Transform that tells where the character is right now ( world space )
        void SetStartTransform( Transform const& startTransform ) { m_startTransform = startTransform; }
        Transform const& GetStartTransform() const { return m_startTransform; }

        // When debugging is enabled the warper will store extra data that can be used for debugging later and also enables the logging of warnings and errors
        void SetDebuggingEnabled( bool isEnabled ) { m_isDebuggingEnabled = isEnabled; }
        bool IsDebuggingEnabled() const { return m_isDebuggingEnabled; }

        // If true the root will be aligned with the target transform at end of the last warp section
        void SetAlignTargetAtLastSection( bool shouldAlign ) { m_alignTargetAtLastSection = shouldAlign; }
        bool IsAlignTargetAtLastSection() const { return m_alignTargetAtLastSection; }

        // Use this to know what can be warped given the start time. Much cheaper than attempting to warp and learning it could not be done.
        Capability GetWarpCapabilities( FrameTime const& startTime ) const;

        // Warped root motion will be available if the last warp succeeded
        bool HasWarpedRootMotion() const { return m_warpedRootMotion.IsValid(); }
        RootMotionData const& GetWarpedRootMotion() const { return m_warpedRootMotion; }

        // Just shifts the warped root motion by the provided delta transform
        void OffsetWarpedRootMotion( Transform const& delta );

        #if EE_DEVELOPMENT_TOOLS
        // Draws debug information of the last successful warp if debugging was enabled on the context during it
        void DrawDebug( DebugDrawContext& drawCtx ) const;

        // Provides warning/errors encountered during the last warp. Will only be populated if debugging is enabled on the context
        Log const& GetLog() const { return m_log; }
        #endif

    private:

        void PrepareForWarp();

        #if EE_DEVELOPMENT_TOOLS
        void SetDebugData( IRootMotionWarpDebugData* pDebugData );
        void LogError( char const* pFormat, ... );
        void LogWarning( char const* pFormat, ... );
        #endif

    private:

        FrameTime                               m_startTime;
        Transform                               m_startTransform = Transform::Identity;
        Transform                               m_targetTransform = Transform::Identity;
        RootMotionData                          m_rootMotion;
        RootMotionData                          m_warpedRootMotion;
        TVector<Section>                        m_sections;
        TVector<Transform>                      m_deltaTransforms;
        TVector<Transform>                      m_inverseDeltaTransforms;
        bool                                    m_isDebuggingEnabled = false;
        bool                                    m_alignTargetAtLastSection = false;

        #if EE_DEVELOPMENT_TOOLS
        Log                                     m_log;
        IRootMotionWarpDebugData*               m_pDebugData = nullptr;
        #endif
    };

    // Root Motion Warper
    //-------------------------------------------------------------------------

    class EE_ENGINE_API RootMotionWarper
    {
        friend class RootMotionWarpDebugData;
        friend class RootMotionWarpContext;

    public:

        static bool Warp( RootMotionWarpContext& context, float maxTangentLength = 1.25f, float lerpFallbackDistanceThreshold = 0.1f );

    private:

        struct Section
        {
        public:

            // Is this a section that should be ignored by the warp
            inline bool IsFixedSection() const { return m_warpRule == TargetWarpRule::FixedSection; }

            // Ensure that the section is at least 3 frames in length (i.e. a single warp frame)
            inline bool HasValidFrameRange() const { return m_endFrame > m_startFrame + 1; }

            // Get number of warpable frames (including last frame)
            inline int32_t GetNumWarpableFrames() const { return m_endFrame - m_startFrame; }

        public:

            int32_t                             m_startFrame = 0;
            int32_t                             m_endFrame = 0;
            Transform                           m_deltaTransform;
            TVector<float>                      m_totalProgress; // The total progress along the section for each frame
            float                               m_distanceCovered = 0.0f;
            TargetWarpRule                      m_warpRule = TargetWarpRule::WarpXYZ;
            TargetWarpAlgorithm                 m_translationAlgorithm = TargetWarpAlgorithm::Bezier;
            bool                                m_hasTranslation = false;

            #if EE_DEVELOPMENT_TOOLS
            mutable Vector                      m_debugPoints[4]; // In the warp start space (i.e. relative to the start transform)
            #endif
        };

        struct Sections
        {
            TInlineVector<Section, 5>           m_sections;
            int8_t                              m_translationXYSectionIdx = InvalidIndex;
            int8_t                              m_rotationSectionIdx = InvalidIndex;
            bool                                m_isTranslationAllowedZ = false;
            int8_t                              m_numSectionZ = 0;
            int32_t                             m_totalNumWarpableZFrames = 0;
        };

        enum class InitializeWarpSectionsMode
        {
            Full,
            ValidityCheckOnly,
        };

    private:

        static bool InitializeWarpSections( RootMotionWarpContext const& context, InitializeWarpSectionsMode mode, Sections* pOutSections, FrameTime const* pStartTimeOverride = nullptr );

        static void SolveFixedSection( RootMotionWarpContext& context, Section const& section, Transform const& startTransform );
        static void SolveTranslationZSection( RootMotionWarpContext& context, Section const& section, Transform const& startTransform, float correctionZ );
        static void SolveTranslationSection( RootMotionWarpContext& context, float maxTangentLength, float lerpFallbackDistanceThreshold, Section const& section, Transform const& startTransform, Transform const& endTransform );
        static void SolveRotationSection( RootMotionWarpContext& context, Section const& section, Transform const& startTransform, Quaternion const& correction );
    };
}