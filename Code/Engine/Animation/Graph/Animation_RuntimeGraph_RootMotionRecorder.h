#pragma once

#include "System/Math/Transform.h"
#include "System/Time/Time.h"
#include "System/Types/Arrays.h"
#include "System/Types/Pool.h"

//-------------------------------------------------------------------------

namespace EE::Drawing { class DrawContext; }

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    enum class RootMotionRecorderDebugMode
    {
        Off = 0,
        DrawRoot,
        DrawRecordedRootMotion,
        DrawRecordedRootMotionAdvanced,
    };

    //-------------------------------------------------------------------------

    class RootMotionRecorder
    {
        constexpr static int32_t const s_recordingBufferSize = 300;

    public:

        enum class ActionType : uint16_t
        {
            Unknown = 0,
            Sample,
            Modification,
            Blend,
        };

        struct RecordedAction
        {
            RecordedAction( int16_t nodeIdx, ActionType actionType, Transform const& rootMotionDelta )
                : m_rootMotionDelta( rootMotionDelta )
                , m_nodeIdx( nodeIdx )
                , m_actionType( actionType )
            {}

            Transform                   m_rootMotionDelta;
            int16_t              m_nodeIdx;
            ActionType                  m_actionType;
            TInlineVector<int16_t, 2>     m_dependencies;
        };

        struct RecordedPosition
        {
            Transform                   m_expectedTransform;
            Transform                   m_actualTransform;
        };

    public:

        RootMotionRecorder();

        inline TVector<RecordedAction> const& GetRecordedActions() const { return m_recordedActions; }
        void StartCharacterUpdate( Transform const& characterWorldTransform );
        void EndCharacterUpdate( Transform const& characterWorldTransform );

        void SetDebugMode( RootMotionRecorderDebugMode mode );
        RootMotionRecorderDebugMode GetDebugMode() const { return m_debugMode; }
        void DrawDebug( Drawing::DrawContext& drawingContext );

        //-------------------------------------------------------------------------

        inline bool HasRecordedActions() const { return !m_recordedActions.empty(); }

        EE_FORCE_INLINE int16_t GetLastActionIndex() const { return (int16_t) m_recordedActions.size() - 1; }

        EE_FORCE_INLINE int16_t RecordSampling( int16_t nodeIdx, Transform const& rootMotionDelta )
        {
            EE_ASSERT( nodeIdx != InvalidIndex );
            int16_t const idx = (int16_t) m_recordedActions.size();
            m_recordedActions.emplace_back( nodeIdx, ActionType::Sample, rootMotionDelta);
            return idx;
        }

        EE_FORCE_INLINE int16_t RecordModification( int16_t nodeIdx, Transform const& rootMotionDelta )
        {
            EE_ASSERT( nodeIdx != InvalidIndex );
            int16_t const previousIdx = GetLastActionIndex();
            int16_t const idx = (int16_t) m_recordedActions.size();
            auto& action = m_recordedActions.emplace_back( nodeIdx, ActionType::Modification, rootMotionDelta );
            if ( previousIdx >= 0 )
            {
                action.m_dependencies.emplace_back( previousIdx );
            }
            return idx;
        }

        // Blend operations automatically pop a blend context
        EE_FORCE_INLINE int16_t RecordBlend( int16_t nodeIdx, int16_t originalRootMotionActionIdx0, int16_t originalRootMotionActionIdx1, Transform const& rootMotionDelta )
        {
            EE_ASSERT( nodeIdx != InvalidIndex );
            int16_t const idx = (int16_t) m_recordedActions.size();
            auto& action = m_recordedActions.emplace_back( nodeIdx, ActionType::Blend, rootMotionDelta );
            action.m_dependencies.emplace_back( originalRootMotionActionIdx0 );

            // It's possible for the use to only have a single source for a blend since not all nodes register root motion actions
            if ( originalRootMotionActionIdx0 == originalRootMotionActionIdx1 )
            {
                action.m_dependencies.emplace_back( InvalidIndex );
            }
            else
            {
                EE_ASSERT( originalRootMotionActionIdx1 > originalRootMotionActionIdx0 );
                action.m_dependencies.emplace_back( originalRootMotionActionIdx1 );
            }

            return idx;
        }

    private:

        TVector<RecordedAction>         m_recordedActions;
        Transform                       m_startWorldTransform;
        Transform                       m_endWorldTransform;
        TVector<RecordedPosition>       m_recordedRootTransforms; // Circular buffer
        int32_t                         m_freeBufferIdx = 0;
        RootMotionRecorderDebugMode     m_debugMode = RootMotionRecorderDebugMode::Off;
    };
}
#endif