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
        for ( auto& rgs : m_referencedGraphStates )
        {
            EE::Delete( rgs.m_pRecordedState );
        }

        m_referencedGraphStates.clear();
        m_initializedNodeIndices.clear();
        m_inputArchive.Reset();
        m_outputArchive.Reset();
    }

    void RecordedGraphState::PrepareForReading()
    {
        m_inputArchive.ReadFromData( m_outputArchive.GetBinaryData(), m_outputArchive.GetBinaryDataSize() );

        for ( auto& rg : m_referencedGraphStates )
        {
            rg.m_pRecordedState->m_inputArchive.ReadFromData( rg.m_pRecordedState->m_outputArchive.GetBinaryData(), rg.m_pRecordedState->m_outputArchive.GetBinaryDataSize() );
        }
    }

    static void GetGraphIDs( RecordedGraphState const& state, TVector<ResourceID>& outGraphIDs )
    {
        outGraphIDs.emplace_back( state.m_graphID );

        for ( auto const& rg : state.m_referencedGraphStates )
        {
            GetGraphIDs( *rg.m_pRecordedState, outGraphIDs );
        }
    }

    void RecordedGraphState::GetAllRecordedGraphResourceIDs( TVector<ResourceID>& outGraphIDs ) const
    {
        GetGraphIDs( *this, outGraphIDs );
    }

    //-------------------------------------------------------------------------

    RecordedGraphState* RecordedGraphState::CreateReferencedGraphStateRecording( int16_t referencedGraphNodeIdx )
    {
        EE_ASSERT( referencedGraphNodeIdx >= 0 );
        EE_ASSERT( VectorContains( m_initializedNodeIndices, referencedGraphNodeIdx ) ); // If this fires it means we recording an uninitialized referenced graph!
        EE_ASSERT( !VectorContains( m_referencedGraphStates, referencedGraphNodeIdx, [] ( ReferencedGraphState const& rgs, int16_t childGraphNodeIdx ) { return rgs.m_referencedGraphNodeIdx == childGraphNodeIdx; } ) ); // Ensure we dont record the same graph twice

        auto& rgs = m_referencedGraphStates.emplace_back();
        rgs.m_referencedGraphNodeIdx = referencedGraphNodeIdx;
        rgs.m_pRecordedState = EE::New<RecordedGraphState>();

        return rgs.m_pRecordedState;
    }

    RecordedGraphState* RecordedGraphState::GetReferencedGraphRecording( int16_t referencedGraphNodeIdx ) const
    {
        EE_ASSERT( referencedGraphNodeIdx >= 0 );
        EE_ASSERT( VectorContains( m_initializedNodeIndices, referencedGraphNodeIdx ) ); // If this fires it means we restoring an uninitialized referenced graph!

        auto foundIter = VectorFind( m_referencedGraphStates, referencedGraphNodeIdx, [] ( ReferencedGraphState const& rgs, int16_t childGraphNodeIdx ) { return rgs.m_referencedGraphNodeIdx == childGraphNodeIdx; } );
        EE_ASSERT( foundIter != m_referencedGraphStates.end() ); // Ensure we have the desired data

        return foundIter->m_pRecordedState;
    }

    bool GraphRecorder::HasRecordedDataForGraph( ResourceID const& graphResourceID ) const
    {
        TVector<ResourceID> graphIDs;
        m_initialState.GetAllRecordedGraphResourceIDs( graphIDs );
        return VectorContains( graphIDs, graphResourceID );
    }
}
#endif