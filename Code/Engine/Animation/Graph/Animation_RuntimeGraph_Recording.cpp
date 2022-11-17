#include "Animation_RuntimeGraph_Recording.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    void GraphStateRecorder::Reset()
    {
        for ( auto& cgis : m_childGraphInitialStates )
        {
            EE::Delete( cgis.m_pInitialState );
        }

        m_childGraphInitialStates.clear();
        m_initializedNodeIndices.clear();
        m_archive.Reset();
    }

    GraphStateRecorder* GraphStateRecorder::CreateChildGraphStateRecording( int16_t childGraphNodeIdx )
    {
        EE_ASSERT( childGraphNodeIdx >= 0 );
        EE_ASSERT( VectorContains( m_initializedNodeIndices, childGraphNodeIdx ) ); // If this fires it means we recording an uninitialized child graph!
        EE_ASSERT( !VectorContains( m_childGraphInitialStates, childGraphNodeIdx, [] ( ChildGraphState const& cgis, int16_t childGraphNodeIdx ) { return cgis.m_nodeIdx == childGraphNodeIdx; } ) ); // Ensure we dont record the same graph twice

        auto& cgis = m_childGraphInitialStates.emplace_back();
        cgis.m_nodeIdx = childGraphNodeIdx;
        cgis.m_pInitialState = EE::New<GraphStateRecorder>();

        return cgis.m_pInitialState;
    }

    //-------------------------------------------------------------------------

    GraphStateRecording::~GraphStateRecording()
    {
        EE_ASSERT( m_pNodes == nullptr );
        Reset();
    }

    void GraphStateRecording::Reset()
    {
        for ( auto& cgis : m_childGraphInitialStates )
        {
            EE::Delete( cgis.m_pRecordedState );
        }

        m_childGraphInitialStates.clear();
        m_initializedNodeIndices.clear();
        m_archive.Reset();
    }

    //-------------------------------------------------------------------------

    GraphRecorder::~GraphRecorder()
    {
        Reset();
    }

    void GraphRecorder::Reset()
    {
        m_stateRecorder.Reset();
        m_updateRecorder.Reset();
    }

    void GraphRecorder::GetRecordedGraphState( GraphStateRecording& outRecording )
    {
        // Main graph
        outRecording.m_archive.ReadFromData( m_stateRecorder.m_archive.GetBinaryData(), m_stateRecorder.m_archive.GetBinaryDataSize() );
        outRecording.m_initializedNodeIndices = m_stateRecorder.m_initializedNodeIndices;

        // Child graphs
        for ( auto& cg : m_stateRecorder.m_childGraphInitialStates )
        {
            // Create record
            auto& cgRecording = outRecording.m_childGraphInitialStates.emplace_back();
            cgRecording.m_nodeIdx = cg.m_nodeIdx;

            // Create child recording data
            cgRecording.m_pRecordedState = EE::New<GraphStateRecording>();
            cgRecording.m_pRecordedState->m_archive.ReadFromData( cg.m_pInitialState->m_archive.GetBinaryData(), cg.m_pInitialState->m_archive.GetBinaryDataSize() );
            cgRecording.m_pRecordedState->m_initializedNodeIndices = cg.m_pInitialState->m_initializedNodeIndices;
        }
    }

    GraphStateRecording* GraphStateRecording::GetChildGraphRecording( int16_t childGraphNodeIdx ) const
    {
        EE_ASSERT( childGraphNodeIdx >= 0 );
        EE_ASSERT( VectorContains( m_initializedNodeIndices, childGraphNodeIdx ) ); // If this fires it means we restoring an uninitialized child graph!

        auto foundIter = VectorFind( m_childGraphInitialStates, childGraphNodeIdx, [] ( ChildGraphState const& cgis, int16_t childGraphNodeIdx ) { return cgis.m_nodeIdx == childGraphNodeIdx; } );
        EE_ASSERT( foundIter != m_childGraphInitialStates.end() ); // Ensure we have the desired data

        return foundIter->m_pRecordedState;
    }
}
#endif