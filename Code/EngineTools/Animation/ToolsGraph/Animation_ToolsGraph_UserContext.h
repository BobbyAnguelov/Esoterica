#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Layers.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Transition.h"
#include "EngineTools/Core/VisualGraph/VisualGraph_UserContext.h"
#include "EngineTools/Core/CategoryTree.h"
#include "Base/Types/HashMap.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Blend2D.h"


//-------------------------------------------------------------------------

namespace EE::Resource { class ResourceDatabase; }
namespace EE::TypeSystem { class TypeInfo; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class VariationHierarchy;

    namespace GraphNodes
    {
        class ControlParameterToolsNode;
        class VirtualParameterToolsNode;
    }

    //-------------------------------------------------------------------------

    struct ToolsGraphUserContext : public VisualGraph::UserContext
    {
        // Node Helpers
        //-------------------------------------------------------------------------

        inline TInlineVector<GraphNodes::ControlParameterToolsNode*, 20> const& GetControlParameters() const { EE_ASSERT( m_pControlParameters != nullptr ); return *m_pControlParameters; }
        inline TInlineVector<GraphNodes::VirtualParameterToolsNode*, 20> const& GetVirtualParameters() const { EE_ASSERT( m_pVirtualParameters != nullptr ); return *m_pVirtualParameters; }
        inline Category<TypeSystem::TypeInfo const*> const& GetCategorizedNodeTypes() const { return *m_pCategorizedNodeTypes; }

        // Debug Data
        //-------------------------------------------------------------------------

        inline bool HasDebugData() const
        {
            return m_pGraphInstance != nullptr && m_pGraphInstance->IsInitialized();
        }

        inline int16_t GetRuntimeGraphNodeIndex( UUID const& nodeID ) const
        {
            auto const foundIter = m_nodeIDtoIndexMap.find( nodeID );
            if ( foundIter != m_nodeIDtoIndexMap.end() )
            {
                return foundIter->second;
            }

            return InvalidIndex;
        }

        inline UUID GetGraphNodeUUID( int16_t const& runtimeNodeIdx ) const
        {
            auto const foundIter = m_nodeIndexToIDMap.find( runtimeNodeIdx );
            if ( foundIter != m_nodeIndexToIDMap.end() )
            {
                return foundIter->second;
            }

            return UUID();
        }

        TVector<int16_t> const& GetActiveNodes() const;

        bool IsNodeActive( int16_t nodeIdx ) const;

        // Get a pose node's debug information
        PoseNodeDebugInfo GetPoseNodeDebugInfo( int16_t runtimeNodeIdx ) const;

        // Get the value of a given runtime value node
        template<typename T>
        inline T GetRuntimeNodeDebugValue( int16_t runtimeNodeIdx ) const
        {
            return m_pGraphInstance->GetRuntimeNodeDebugValue<T>( runtimeNodeIdx );
        }

        // Get layer weight from a given runtime layer blend node
        inline float GetLayerWeight( int16_t runtimeNodeIdx, int32_t layerIdx ) const
        {
            auto pLayerNode = static_cast<GraphNodes::LayerBlendNode const*>( m_pGraphInstance->GetNodeDebugInstance( runtimeNodeIdx ) );
            if( pLayerNode->IsInitialized() )
            {
                return pLayerNode->GetLayerWeight( layerIdx );
            }

            return 0.0f;
        }

        // Get layer weight from a given runtime layer blend node
        inline Float2 GetBlend2DParameter( int16_t runtimeNodeIdx ) const
        {
            auto pLayerNode = static_cast<GraphNodes::Blend2DNode const*>( m_pGraphInstance->GetNodeDebugInstance( runtimeNodeIdx ) );
            if ( pLayerNode->IsInitialized() )
            {
                return pLayerNode->GetParameter();
            }

            return Float2::Zero;
        }

        // Get transition progress
        inline float GetTransitionProgress( int16_t runtimeNodeIdx ) const
        {
            auto pTransitionNode = static_cast<GraphNodes::TransitionNode const*>( m_pGraphInstance->GetNodeDebugInstance( runtimeNodeIdx ) );
            if ( pTransitionNode->IsInitialized() )
            {
                return pTransitionNode->GetProgressPercentage();
            }

            return 0.0f;
        }

    public:

        Resource::ResourceDatabase const*                                   m_pResourceDatabase = nullptr;
        StringID                                                            m_selectedVariationID;
        VariationHierarchy const*                                           m_pVariationHierarchy = nullptr;
        GraphInstance*                                                      m_pGraphInstance = nullptr;
        THashMap<UUID, int16_t>                                             m_nodeIDtoIndexMap;
        THashMap<int16_t, UUID>                                             m_nodeIndexToIDMap;
        TInlineVector<GraphNodes::ControlParameterToolsNode*, 20> const*    m_pControlParameters = nullptr;
        TInlineVector<GraphNodes::VirtualParameterToolsNode*, 20> const*    m_pVirtualParameters = nullptr;
        Category<TypeSystem::TypeInfo const*> const*                        m_pCategorizedNodeTypes = nullptr;
        TypeSystem::TypeRegistry const*                                     m_pTypeRegistry = nullptr;
        bool                                                                m_showRuntimeIndices = false;
    };
}