#pragma once
#include "Animation_RuntimeGraph_Definition.h"
#include "Animation_RuntimeGraph_Node.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphContext;

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    enum class GraphDebugMode
    {
        Off,
        On,
    };
    #endif

    //-------------------------------------------------------------------------

    class GraphInstance
    {
    public:

        GraphInstance( GraphVariation const* pGraphVariation );
        ~GraphInstance();

        void Initialize( GraphContext& context );
        void Shutdown( GraphContext& context );

        // Graph State
        //-------------------------------------------------------------------------

        // Is this a valid instance that has been correctly initialized
        bool IsInitialized() const { return m_pRootNode != nullptr && m_pRootNode->IsValid(); }

        // Fully reset all nodes in the graph
        void Reset( GraphContext& context );

        // Run the graph logic
        GraphPoseNodeResult UpdateGraph( GraphContext& context );

        // General Node Info
        //-------------------------------------------------------------------------

        inline bool IsValidNodeIndex( int16_t nodeIdx ) const 
        {
            return nodeIdx < m_pGraphVariation->m_pGraphDefinition->m_nodeSettings.size();
        }

        // Control Parameters
        //-------------------------------------------------------------------------

        inline int32_t GetNumControlParameters() const { return m_pGraphVariation->m_pGraphDefinition->m_numControlParameters; }

        inline int16_t GetControlParameterIndex( StringID parameterID ) const
        {
            for ( int16_t i = 0; i < m_pGraphVariation->m_pGraphDefinition->m_numControlParameters; i++ )
            {
                if ( m_pGraphVariation->m_pGraphDefinition->m_controlParameterIDs[i] == parameterID )
                {
                    return i;
                }
            }

            return InvalidIndex;
        }

        inline StringID GetControlParameterID( int16_t parameterNodeIdx )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            return m_pGraphVariation->m_pGraphDefinition->m_controlParameterIDs[parameterNodeIdx];
        }

        inline GraphValueType GetControlParameterType( int16_t parameterNodeIdx )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            return static_cast<ValueNode*>( m_nodes[parameterNodeIdx] )->GetValueType();
        }

        template<typename T>
        inline void SetControlParameterValue( GraphContext& context, int16_t parameterNodeIdx, T const& value )
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            static_cast<ValueNode*>( m_nodes[parameterNodeIdx] )->SetValue<T>( context, value );
        }

        template<typename T>
        inline T GetControlParameterValue( GraphContext& context, int16_t parameterNodeIdx ) const
        {
            EE_ASSERT( IsControlParameter( parameterNodeIdx ) );
            return static_cast<ValueNode*>( m_nodes[parameterNodeIdx] )->GetValue<T>( context );
        }

        // Debug Information
        //-------------------------------------------------------------------------
        
        #if EE_DEVELOPMENT_TOOLS
        inline GraphDebugMode GetDebugMode() const { return m_debugMode; }
        inline void SetDebugMode( GraphDebugMode mode ) { m_debugMode = mode; }
        inline void SetNodeDebugFilterList( TVector<int16_t> const& filterList ) { m_debugFilterNodes = filterList; }
        void DrawDebug( GraphContext& context, Drawing::DrawContext& drawContext );

        inline bool IsNodeActive( GraphContext& context, int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            auto pNode = m_nodes[nodeIdx];
            return pNode->IsNodeActive( context );
        }

        inline PoseNodeDebugInfo GetPoseNodeDebugInfo( GraphContext& context, int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            EE_ASSERT( m_nodes[nodeIdx]->GetValueType() == GraphValueType::Pose );
            auto pNode = static_cast<PoseNode const*>( m_nodes[nodeIdx] );
            return pNode->GetDebugInfo();
        }

        template<typename T>
        inline T GetRuntimeNodeValue( GraphContext& context, int16_t nodeIdx ) const
        {
            EE_ASSERT( IsValidNodeIndex( nodeIdx ) );
            auto pValueNode = static_cast<ValueNode*>( const_cast<GraphNode*>( m_nodes[nodeIdx] ) );
            return pValueNode->GetValue<T>( context );
        }
        #endif

    private:

        EE_FORCE_INLINE bool IsControlParameter( int16_t nodeIdx ) const 
        {
            return nodeIdx < m_pGraphVariation->m_pGraphDefinition->m_numControlParameters;
        }

        GraphInstance( GraphInstance const& ) = delete;
        GraphInstance( GraphInstance&& ) = delete;
        GraphInstance& operator=( GraphInstance const& ) = delete;
        GraphInstance& operator=( GraphInstance&& ) = delete;

    private:

        GraphVariation const* const             m_pGraphVariation = nullptr;
        TVector<GraphNode*>                     m_nodes;
        uint8_t*                                m_pAllocatedInstanceMemory = nullptr;
        PoseNode*                               m_pRootNode = nullptr;

        #if EE_DEVELOPMENT_TOOLS
        TVector<int16_t>                        m_activeNodes;
        GraphDebugMode                          m_debugMode = GraphDebugMode::Off;
        TVector<int16_t>                        m_debugFilterNodes; // The list of nodes that are allowed to debug draw (if this is empty all nodes will draw)
        #endif
    };
}
