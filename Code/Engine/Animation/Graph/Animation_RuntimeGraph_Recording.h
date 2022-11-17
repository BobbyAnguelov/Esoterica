#pragma once
#include "Engine/_Module/API.h"
#include "Engine/Animation/AnimationTarget.h"
#include "System/Time/Time.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/Types/Containers_ForwardDecl.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    class GraphNode;

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API GraphStateRecorder
    {
        struct ChildGraphState
        {
            int16_t                                         m_nodeIdx = InvalidIndex;
            GraphStateRecorder*                             m_pInitialState = nullptr;
        };

    public:

        void Reset();

        GraphStateRecorder* CreateChildGraphStateRecording( int16_t childGraphNodeIdx );

        // Serialization helpers
        template<typename T>
        inline void operator<<( T& value )
        {
            m_archive << value;
        }

        // Serialization helpers
        template<typename T>
        inline void operator<<( T const& value )
        {
            m_archive << value;
        }

    public:

        TVector<int16_t>                                    m_initializedNodeIndices;
        Serialization::BinaryOutputArchive                  m_archive;
        TVector<ChildGraphState>                            m_childGraphInitialStates;
    };

    //-------------------------------------------------------------------------

    struct GraphUpdateRecorder
    {
        union ParameterData
        {
            ParameterData() {}

            bool        m_bool;
            StringID    m_ID;
            int32_t     m_int;
            float       m_float;
            Vector      m_vector;
            Target      m_target;
        };

        struct FrameData
        {
            Seconds                                         m_deltaTime;
            Transform                                       m_characterWorldTransform;
            TVector<ParameterData>                          m_parameterData;
        };

    public:

        inline void Reset() { m_frameData.clear(); }

    public:

        TVector<FrameData>                                  m_frameData;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API GraphStateRecording
    {
        struct ChildGraphState
        {
            int16_t                                         m_nodeIdx = InvalidIndex;
            GraphStateRecording*                            m_pRecordedState = nullptr;
        };

    public:

        ~GraphStateRecording();

        void Reset();

        // Serialization helpers
        template<typename T>
        inline void operator<<( T& value ) const
        {
            m_archive << value;
        }

        // Node accessor
        template<typename T>
        inline T* GetNode( int16_t nodeIdx ) const
        {
            EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_pNodes->size() );
            return (T*) ( *m_pNodes )[nodeIdx];
        }

        GraphStateRecording* GetChildGraphRecording( int16_t childGraphNodeIdx ) const;

    public:

        TVector<int16_t>                                    m_initializedNodeIndices;
        TVector<GraphNode*>*                                m_pNodes = nullptr;
        mutable Serialization::BinaryInputArchive           m_archive;
        TVector<ChildGraphState>                            m_childGraphInitialStates;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINE_API GraphRecorder final
    {
    public:

        ~GraphRecorder();

        // Clear all recorded data and prepare for a new recording
        void Reset();

        // Create the necessary state needed to restore a graph to a recorded initial state
        void GetRecordedGraphState( GraphStateRecording& outRecording );

        // Get the number of recorded frames
        EE_FORCE_INLINE int32_t GetNumRecordedFrames() const { return (int32_t) m_updateRecorder.m_frameData.size(); }

        // Is this index within the recorded range?
        EE_FORCE_INLINE bool IsValidRecordedFrameIndex( int32_t frameIdx ) const { return frameIdx >= 0 && frameIdx < m_updateRecorder.m_frameData.size(); }

        // Get the for a given recorded frame
        EE_FORCE_INLINE GraphUpdateRecorder::FrameData GetFrameData( int32_t frameIdx ) const
        {
            EE_ASSERT( frameIdx >= 0 && frameIdx < m_updateRecorder.m_frameData.size() );
            return m_updateRecorder.m_frameData[frameIdx];
        }

    public:

        // Initial state
        GraphStateRecorder                                  m_stateRecorder;

        // Per frame data
        GraphUpdateRecorder                                 m_updateRecorder;
    };
}
#endif