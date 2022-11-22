#include "Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    #if EE_DEVELOPMENT_TOOLS
    Color GetColorForValueType( GraphValueType type )
    {
        const Color colors[10] =
        {
            Colors::GhostWhite,
            Colors::PaleGreen,
            Colors::Violet,
            Colors::LightSkyBlue,
            Colors::CornflowerBlue,
            Colors::DeepSkyBlue,
            Colors::Aqua,
            Colors::YellowGreen,
            Colors::GreenYellow,
            Colors::HotPink,
        };

        return colors[(uint8_t) type];
    }

    char const* GetNameForValueType( GraphValueType type )
    {
        switch ( type )
        {
            case GraphValueType::Bool:
            {
                return "Bool";
            }
            break;

            case GraphValueType::ID:
            {
                return "ID";
            }
            break;

            case GraphValueType::Int:
            {
                return "Int";
            }
            break;

            case GraphValueType::Float:
            {
                return "Float";
            }
            break;

            case GraphValueType::Vector:
            {
                return "Vector";
            }
            break;

            case GraphValueType::Target:
            {
                return "Target";
            }
            break;

            case GraphValueType::BoneMask:
            {
                return "Bone Mask";
            }
            break;

            case GraphValueType::Pose:
            {
                return "Pose";
            }
            break;

            case GraphValueType::Special:
            {
                return "Special";
            }
            break;

            default:
            {
                return "Unknown";
            }
            break;
        }
    }
    #endif

    //-------------------------------------------------------------------------

    void GraphNode::Settings::Load( Serialization::BinaryInputArchive& archive )
    {
        archive.Serialize( m_nodeIdx );
    }

    void GraphNode::Settings::Save( Serialization::BinaryOutputArchive& archive ) const
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
        if ( IsInitialized() )
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
        EE_ASSERT( IsInitialized() );
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
        EE_ASSERT( !IsInitialized() );
        ++m_initializationCount;
    }

    void GraphNode::ShutdownInternal( GraphContext& context )
    {
        EE_ASSERT( !IsInitialized() );
        m_lastUpdateID = 0xFFFFFFFF;
    }

    bool GraphNode::IsNodeActive( GraphContext& context ) const
    {
        return m_lastUpdateID == context.m_updateID;
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
        if ( IsInitialized() )
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
        m_duration = 0.0f;
        m_previousTime = 0.0f;
        m_currentTime = m_previousTime;
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