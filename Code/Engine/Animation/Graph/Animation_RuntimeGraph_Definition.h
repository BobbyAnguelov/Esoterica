#pragma once
#include "Animation_RuntimeGraph_Node.h"
#include "Animation_RuntimeGraph_DataSet.h"
#include "Engine/Animation/AnimationClip.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class EE_ENGINE_API GraphDefinition final : public Resource::IResource
    {
        EE_REGISTER_RESOURCE( 'ag', "Animation Graph" );
        EE_SERIALIZE( m_persistentNodeIndices, m_instanceNodeStartOffsets, m_instanceRequiredMemory, m_instanceRequiredAlignment, m_rootNodeIdx, m_controlParameterIDs, m_virtualParameterIDs, m_virtualParameterNodeIndices, m_childGraphSlots, m_externalGraphSlots );

        friend class GraphDefinitionCompiler;
        friend class AnimationGraphCompiler;
        friend class GraphLoader;
        friend class GraphInstance;

    public:

        struct ExternalGraphSlot
        {
            EE_SERIALIZE( m_nodeIdx, m_slotID );

            ExternalGraphSlot() = default;
            ExternalGraphSlot( int16_t idx, StringID ID ) : m_nodeIdx( idx ), m_slotID( ID ) { EE_ASSERT( idx != InvalidIndex && ID.IsValid() ); }

            int16_t                                 m_nodeIdx = InvalidIndex;
            StringID                                m_slotID;
        };

        struct ChildGraphSlot
        {
            EE_SERIALIZE( m_nodeIdx, m_dataSlotIdx );

            ChildGraphSlot() = default;
            ChildGraphSlot( int16_t idx, int16_t dataSlotIdx ) : m_nodeIdx( idx ), m_dataSlotIdx( dataSlotIdx ) { EE_ASSERT( idx != InvalidIndex && dataSlotIdx != InvalidIndex ); }

            int16_t                                 m_nodeIdx = InvalidIndex;
            int16_t                                 m_dataSlotIdx;
        };

    public:

        virtual bool IsValid() const override { return m_rootNodeIdx != InvalidIndex; }

        #if EE_DEVELOPMENT_TOOLS
        String const& GetNodePath( int16_t nodeIdx ) const{ return m_nodePaths[nodeIdx]; }
        #endif

    private:

        TVector<int16_t>                            m_persistentNodeIndices;
        TVector<uint32_t>                           m_instanceNodeStartOffsets;
        uint32_t                                    m_instanceRequiredMemory = 0;
        uint32_t                                    m_instanceRequiredAlignment = 0;
        int16_t                                     m_rootNodeIdx = InvalidIndex;
        TVector<StringID>                           m_controlParameterIDs;
        TVector<StringID>                           m_virtualParameterIDs;
        TVector<int16_t>                            m_virtualParameterNodeIndices;
        TVector<ChildGraphSlot>                     m_childGraphSlots;
        TVector<ExternalGraphSlot>                  m_externalGraphSlots;
        THashMap<StringID, int16_t>                 m_parameterLookupMap;

        #if EE_DEVELOPMENT_TOOLS
        TVector<String>                             m_nodePaths;
        #endif

        // Node settings are created/destroyed by the animation graph loader
        TVector<GraphNode::Settings*>               m_nodeSettings;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GraphVariation final : public Resource::IResource
    {
        EE_REGISTER_RESOURCE( 'agv', "Animation Graph Variation" );
        EE_SERIALIZE( m_pGraphDefinition, m_pDataSet );

        friend class AnimationGraphCompiler;
        friend class GraphLoader;
        friend class GraphInstance;

    public:

        virtual bool IsValid() const override
        {
            return m_pGraphDefinition.IsLoaded() && m_pDataSet.IsLoaded();
        }

        inline Skeleton const* GetSkeleton() const 
        {
            EE_ASSERT( IsValid() );
            return m_pDataSet->GetSkeleton();
        }

        inline GraphDefinition const* GetDefinition() const 
        {
            EE_ASSERT( IsValid() );
            return m_pGraphDefinition.GetPtr();
        }

    protected:

        TResourcePtr<GraphDefinition>               m_pGraphDefinition = nullptr;
        TResourcePtr<GraphDataSet>                  m_pDataSet = nullptr;
    };
}