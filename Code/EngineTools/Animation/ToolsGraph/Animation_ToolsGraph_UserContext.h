#pragma once

#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Layers.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Transition.h"
#include "EngineTools/NodeGraph/NodeGraph_UserContext.h"
#include "EngineTools/Core/CategoryTree.h"
#include "Base/Types/HashMap.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Blend2D.h"
#include "Engine/Animation/Graph/Nodes/Animation_RuntimeGraphNode_Blend1D.h"


//-------------------------------------------------------------------------

namespace EE { class FileRegistry; }
namespace EE::TypeSystem { class TypeInfo; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class VariationHierarchy;

    namespace GraphNodes
    {
        class ParameterBaseToolsNode;
    }

    //-------------------------------------------------------------------------

    struct ToolsGraphUserContext : public NodeGraph::UserContext
    {
        // Node Helpers
        //-------------------------------------------------------------------------

        inline TInlineVector<GraphNodes::ParameterBaseToolsNode*, 20> const& GetParameters() const { EE_ASSERT( m_pParameters != nullptr ); return *m_pParameters; }
        inline Category<TypeSystem::TypeInfo const*> const& GetCategorizedNodeTypes() const { return *m_pCategorizedNodeTypes; }

        // Debug Data
        //-------------------------------------------------------------------------

        inline bool HasDebugData() const
        {
            return m_pGraphInstance != nullptr && m_pGraphInstance->WasInitialized();
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

        // Get the runtime instance of a node for debug purposes
        GraphNode const* GetNodeDebugInstance( int16_t runtimeNodeIdx ) const
        {
            return m_pGraphInstance->GetNodeDebugInstance( runtimeNodeIdx );
        }

        // Get the value of a given runtime value node
        template<typename T>
        inline T GetNodeValue( int16_t runtimeNodeIdx ) const
        {
            return m_pGraphInstance->GetRuntimeNodeDebugValue<T>( runtimeNodeIdx );
        }

    public:

        FileRegistry const*                                                 m_pFileRegistry = nullptr;
        StringID                                                            m_selectedVariationID;
        VariationHierarchy const*                                           m_pVariationHierarchy = nullptr;
        GraphInstance*                                                      m_pGraphInstance = nullptr;
        THashMap<UUID, int16_t>                                             m_nodeIDtoIndexMap;
        THashMap<int16_t, UUID>                                             m_nodeIndexToIDMap;
        TInlineVector<GraphNodes::ParameterBaseToolsNode*, 20> const*       m_pParameters = nullptr;
        Category<TypeSystem::TypeInfo const*> const*                        m_pCategorizedNodeTypes = nullptr;
        TypeSystem::TypeRegistry const*                                     m_pTypeRegistry = nullptr;
        bool                                                                m_showRuntimeIndices = false;
    };
}