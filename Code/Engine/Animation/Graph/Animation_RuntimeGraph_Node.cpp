#include "Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    void InstantiationContext::LogWarning( char const* pFormat, ... ) const
    {
        auto& entry = m_pLog->emplace_back();
        entry.m_updateID = 0;
        entry.m_nodeIdx = m_currentNodeIdx;
        entry.m_severity = Severity::Warning;

        va_list args;
        va_start( args, pFormat );
        entry.m_message.append_sprintf_va_list( pFormat, args );
        va_end( args );
    }
    #endif

    //-------------------------------------------------------------------------

    void GraphNode::Definition::Load( Serialization::BinaryInputArchive& archive )
    {
        archive.Serialize( m_nodeIdx );
    }

    void GraphNode::Definition::Save( Serialization::BinaryOutputArchive& archive ) const
    {
        archive.Serialize( m_nodeIdx );
    }

    //-------------------------------------------------------------------------

    GraphNode::~GraphNode()
    {
        EE_ASSERT( m_initializationCount == 0 );
    }

    void GraphNode::Initialize( GraphContext& context )
    {
        if ( WasInitialized() )
        {
            ++m_initializationCount;
        }
        else
        {
            InitializeInternal( context );
        }
    }

    void GraphNode::Shutdown( GraphContext& context )
    {
        EE_ASSERT( WasInitialized() );
        if ( --m_initializationCount == 0 )
        {
            ShutdownInternal( context );
        }
    }

    void GraphNode::MarkNodeActive( GraphContext& context )
    {
        m_lastUpdateID = context.m_updateID;

        #if EE_DEVELOPMENT_TOOLS
        context.TrackActiveNode( GetNodeIndex() );
        #endif
    }

    void GraphNode::InitializeInternal( GraphContext& context )
    {
        EE_ASSERT( !WasInitialized() );
        ++m_initializationCount;
    }

    void GraphNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( !WasInitialized() );
        m_lastUpdateID = 0xFFFFFFFF;
    }

    #if EE_DEVELOPMENT_TOOLS
    void GraphNode::RecordGraphState( RecordedGraphState& outState )
    {
        outState.WriteValue( m_initializationCount );
    }

    void GraphNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        inState.ReadValue( m_initializationCount );
    }
    #endif

    //-------------------------------------------------------------------------

    void PoseNode::Initialize( GraphContext& context, SyncTrackTime const& initialTime )
    {
        if ( WasInitialized() )
        {
            ++m_initializationCount;
        }
        else
        {
            InitializeInternal( context, initialTime );
        }
    }

    void PoseNode::InitializeInternal( GraphContext& context, SyncTrackTime const& initialTime )
    {
        GraphNode::InitializeInternal( context );

        // Reset node state
        m_loopCount = 0;
        m_previousTime = 0.0f;
        m_currentTime = m_previousTime;

        // Set the duration to 0.0f even though this is an invalid value as it is expected that nodes will set this correctly at initialization time
        m_duration = 0.0f; 
    }

    #if EE_DEVELOPMENT_TOOLS
    PoseNodeDebugInfo PoseNode::GetDebugInfo() const
    {
        PoseNodeDebugInfo info;
        info.m_loopCount = m_loopCount;
        info.m_duration = m_duration;
        info.m_currentTime = m_currentTime;
        info.m_previousTime = m_previousTime;
        info.m_pSyncTrack = &GetSyncTrack();
        info.m_currentSyncTime = info.m_pSyncTrack->GetTime( m_currentTime );
        return info;
    }

    void PoseNode::RecordGraphState( RecordedGraphState& outState )
    {
        GraphNode::RecordGraphState( outState );
        outState.WriteValue( m_loopCount );
        outState.WriteValue( m_duration );
        outState.WriteValue( m_currentTime );
        outState.WriteValue( m_previousTime );
    }

    void PoseNode::RestoreGraphState( RecordedGraphState const& inState )
    {
        GraphNode::RestoreGraphState( inState );
        inState.ReadValue( m_loopCount );
        inState.ReadValue( m_duration );
        inState.ReadValue( m_currentTime );
        inState.ReadValue( m_previousTime );
    }
    #endif
}