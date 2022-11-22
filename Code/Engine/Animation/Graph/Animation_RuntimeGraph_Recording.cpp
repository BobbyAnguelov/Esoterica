#include "Animation_RuntimeGraph_Recording.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::Animation
{
    RecordedGraphState::~RecordedGraphState()
    {
        EE_ASSERT( m_pNodes == nullptr );
        Reset();
    }

    void RecordedGraphState::Reset()
    {
        for ( auto& cgis : m_childGraphStates )
        {
            EE::Delete( cgis.m_pRecordedState );
        }

        m_childGraphStates.clear();
        m_initializedNodeIndices.clear();
        m_inputArchive.Reset();
        m_outputArchive.Reset();
    }

    void RecordedGraphState::PrepareForReading()
    {
        m_inputArchive.ReadFromData( m_outputArchive.GetBinaryData(), m_outputArchive.GetBinaryDataSize() );

        for ( auto& cg : m_childGraphStates )
        {
            cg.m_pRecordedState->m_inputArchive.ReadFromData( cg.m_pRecordedState->m_outputArchive.GetBinaryData(), cg.m_pRecordedState->m_outputArchive.GetBinaryDataSize() );
        }
    }

    static void GetGraphIDs( RecordedGraphState const& state, TVector<ResourceID>& outGraphIDs )
    {
        outGraphIDs.emplace_back( state.m_graphID );

        for ( auto const& cg : state.m_childGraphStates )
        {
            GetGraphIDs( *cg.m_pRecordedState, outGraphIDs );
        }
    }

    void RecordedGraphState::GetAllRecordedGraphResourceIDs( TVector<ResourceID>& outGraphIDs )
    {
        GetGraphIDs( *this, outGraphIDs );
    }

    //-------------------------------------------------------------------------

    RecordedGraphState* RecordedGraphState::CreateChildGraphStateRecording( int16_t childGraphNodeIdx )
    {
        EE_ASSERT( childGraphNodeIdx >= 0 );
        EE_ASSERT( VectorContains( m_initializedNodeIndices, childGraphNodeIdx ) ); // If this fires it means we recording an uninitialized child graph!
        EE_ASSERT( !VectorContains( m_childGraphStates, childGraphNodeIdx, [] ( ChildGraphState const& cgis, int16_t childGraphNodeIdx ) { return cgis.m_childGraphNodeIdx == childGraphNodeIdx; } ) ); // Ensure we dont record the same graph twice

        auto& cgis = m_childGraphStates.emplace_back();
        cgis.m_childGraphNodeIdx = childGraphNodeIdx;
        cgis.m_pRecordedState = EE::New<RecordedGraphState>();

        return cgis.m_pRecordedState;
    }

    RecordedGraphState* RecordedGraphState::GetChildGraphRecording( int16_t childGraphNodeIdx ) const
    {
        EE_ASSERT( childGraphNodeIdx >= 0 );
        EE_ASSERT( VectorContains( m_initializedNodeIndices, childGraphNodeIdx ) ); // If this fires it means we restoring an uninitialized child graph!

        auto foundIter = VectorFind( m_childGraphStates, childGraphNodeIdx, [] ( ChildGraphState const& cgis, int16_t childGraphNodeIdx ) { return cgis.m_childGraphNodeIdx == childGraphNodeIdx; } );
        EE_ASSERT( foundIter != m_childGraphStates.end() ); // Ensure we have the desired data

        return foundIter->m_pRecordedState;
    }
}
#endif