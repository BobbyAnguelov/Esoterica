#pragma once
#include "Engine/_Module/API.h"
#include "Engine/Animation/AnimationTarget.h"
#include "System/Time/Time.h"
#include "System/Serialization/BinarySerialization.h"
#include "System/Types/Containers_ForwardDecl.h"
#include "System/Resource/ResourceID.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    class GraphNode;

    //-------------------------------------------------------------------------
    // Graph State
    //-------------------------------------------------------------------------

    // Records the state of a given graph instance
    struct EE_ENGINE_API RecordedGraphState
    {
        struct ChildGraphState
        {
            int16_t                                         m_childGraphNodeIdx = InvalidIndex;
            RecordedGraphState*                             m_pRecordedState = nullptr;
        };

    public:

        ~RecordedGraphState();

        // Clears all recorded data and allows for a new recording to occur
        void Reset();

        // Prepares recording to be read back
        void PrepareForReading();

        // Get a unique list of the various graphs recorded
        void GetAllRecordedGraphResourceIDs( TVector<ResourceID>& outGraphIDs );

        // Child graphs
        //-------------------------------------------------------------------------

        RecordedGraphState* CreateChildGraphStateRecording( int16_t childGraphNodeIdx );

        RecordedGraphState* GetChildGraphRecording( int16_t childGraphNodeIdx ) const;

        // Serialization helpers
        //-------------------------------------------------------------------------

        template<typename T>
        inline void WriteValue( T& value )
        {
            m_outputArchive << value;
        }
        template<typename T>
        inline void WriteValue( T const& value )
        {
            m_outputArchive << value;
        }

        template<typename T>
        inline void ReadValue( T& value ) const
        {
            m_inputArchive << value;
        }

        // Node accessor
        //-------------------------------------------------------------------------

        template<typename T>
        inline T* GetNode( int16_t nodeIdx ) const
        {
            EE_ASSERT( nodeIdx >= 0 && nodeIdx < m_pNodes->size() );
            return (T*) ( *m_pNodes )[nodeIdx];
        }

    public:

        ResourceID                                          m_graphID;
        StringID                                            m_variationID;
        TVector<int16_t>                                    m_initializedNodeIndices;
        TVector<ChildGraphState>                            m_childGraphStates;
        TVector<GraphNode*>*                                m_pNodes = nullptr;

    private:

        Serialization::BinaryOutputArchive                  m_outputArchive;
        mutable Serialization::BinaryInputArchive           m_inputArchive;
    };

    //-------------------------------------------------------------------------
    // Graph Updates
    //-------------------------------------------------------------------------

    // All the data needed to replay a given recorded frame
    struct RecordedGraphFrameData
    {
        union ParameterData
        {
            ParameterData() {}

            bool                                            m_bool;
            StringID                                        m_ID;
            int32_t                                         m_int;
            float                                           m_float;
            Vector                                          m_vector;
            Target                                          m_target;
        };

    public:

        Seconds                                             m_deltaTime;
        Transform                                           m_characterWorldTransform;
        TVector<ParameterData>                              m_parameterData;
    };

    // Records information about each update for the recorded graph instance
    struct GraphUpdateRecorder
    {
    public:

        inline bool HasRecordedData() const { return !m_recordedData.empty(); }
        inline int32_t GetNumRecordedFrames() const { return int32_t( m_recordedData.size() ); }
        inline bool IsValidRecordedFrameIndex( int32_t frameIdx ) const { return frameIdx >= 0 && frameIdx < m_recordedData.size(); }
        inline void Reset() { m_recordedData.clear(); }

    public:

        ResourceID                                          m_graphID;
        StringID                                            m_variationID;
        TVector<RecordedGraphFrameData>                     m_recordedData;
    };
}
#endif