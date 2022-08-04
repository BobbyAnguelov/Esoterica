#pragma once

#include "EngineTools/_Module/API.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Instance.h"
#include "System/Imgui/ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph { struct DrawContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphInstance;

    // Graph Types
    //-------------------------------------------------------------------------

    enum class GraphType
    {
        EE_REGISTER_ENUM

        BlendTree,
        ValueTree,
        TransitionTree,
    };

    // Debug
    //-------------------------------------------------------------------------

    struct EditorGraphNodeContext
    {
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

        bool IsNodeActive( int16_t nodeIdx ) const;
        
        #if EE_DEVELOPMENT_TOOLS
        PoseNodeDebugInfo GetPoseNodeDebugInfo( int16_t runtimeNodeIdx ) const;
        #endif

        template<typename T>
        inline T GetRuntimeNodeDebugValue( int16_t runtimeNodeIdx ) const
        {
            return m_pGraphInstance->GetRuntimeNodeDebugValue<T>( runtimeNodeIdx );
        }

    public:

        StringID                            m_currentVariationID;
        GraphInstance*                      m_pGraphInstance = nullptr;
        THashMap<UUID, int16_t>             m_nodeIDtoIndexMap;
    };

    //-------------------------------------------------------------------------

    #if EE_DEVELOPMENT_TOOLS
    void DrawPoseNodeDebugInfo( VisualGraph::DrawContext const& ctx, float width, PoseNodeDebugInfo const& debugInfo );
    void DrawEmptyPoseNodeDebugInfo( VisualGraph::DrawContext const& ctx, float width );
    #endif
}