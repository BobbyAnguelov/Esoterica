#pragma once
#include "Animation_RuntimeGraph_Definition.h"
#include "Animation_RuntimeGraph_Node.h"
#include "Engine/Animation/AnimationSkeleton.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    using ResourceLUT = THashMap<uint64_t, Resource::ResourcePtr>;

    //-------------------------------------------------------------------------

    class EE_ENGINE_API GraphDefinition final : public Resource::IResource
    {
        EE_RESOURCE( "ag", "Animation Graph Definition", Colors::Plum, 80, false );
        EE_SERIALIZE( m_variationID, m_skeleton, m_persistentNodeIndices, m_instanceNodeStartOffsets, m_instanceRequiredMemory, m_instanceRequiredAlignment, m_rootNodeIdx, m_controlParameterIDs, m_virtualParameterIDs, m_virtualParameterNodeIndices, m_internalGraphSlots, m_externalGraphSlots, m_resources );

        friend class AnimationGraphCompiler;
        friend class GraphDefinitionCompiler;
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

        struct InternalGraphSlot
        {
            EE_SERIALIZE( m_nodeIdx, m_dataSlotIdx );

            InternalGraphSlot() = default;
            InternalGraphSlot( int16_t idx, int16_t dataSlotIdx ) : m_nodeIdx( idx ), m_dataSlotIdx( dataSlotIdx ) { EE_ASSERT( idx != InvalidIndex ); }

            int16_t                                 m_nodeIdx = InvalidIndex;
            int16_t                                 m_dataSlotIdx = InvalidIndex;
        };

        struct ExternalPoseSlot
        {
            EE_SERIALIZE( m_nodeIdx, m_slotID );

            ExternalPoseSlot() = default;
            ExternalPoseSlot( int16_t idx, StringID ID ) : m_nodeIdx( idx ), m_slotID( ID ) { EE_ASSERT( idx != InvalidIndex && ID.IsValid() ); }

            int16_t                                 m_nodeIdx = InvalidIndex;
            StringID                                m_slotID;
        };

    public:

        virtual bool IsValid() const override
        {
            return m_variationID.IsValid() && m_skeleton.IsLoaded() && m_rootNodeIdx != InvalidIndex;
        }

        inline StringID GetVariationID() const { return m_variationID; }

        inline Skeleton const* GetPrimarySkeleton() const
        {
            EE_ASSERT( IsValid() );
            return m_skeleton.GetPtr();
        }

        inline ResourceID const& GetPrimarySkeletonResourceID() const { return m_skeleton.GetResourceID(); }

        TVector<InternalGraphSlot> const &GetInternalGraphSlots() const { return m_internalGraphSlots; }
        TVector<ExternalGraphSlot> const &GetExternalGraphSlots() const { return m_externalGraphSlots; }
        TVector<ExternalPoseSlot> const &GetExternalPoseSlots() const { return m_externalPoseSlots; }

        // Get the flattened resource lookup table
        ResourceLUT const& GetResourceLookupTable() const { return m_resourceLUT; }

        int32_t GetNumSlots() const { return (int32_t) m_resources.size(); }

        template<typename T>
        inline T const* GetResourceForSlot( int16_t slotIdx ) const
        {
            if ( slotIdx == InvalidIndex )
            {
                return nullptr;
            }

            EE_ASSERT( slotIdx >= 0 && slotIdx < m_resources.size() );

            if ( m_resources[slotIdx].IsSet() )
            {
                return m_resources[slotIdx].GetPtr<T>();
            }

            return nullptr;
        }

        inline ResourceID GetResourceIDForSlot( int16_t slotIdx ) const
        {
            if ( slotIdx != InvalidIndex )
            {
                EE_ASSERT( slotIdx >= 0 && slotIdx < m_resources.size() );
                if ( m_resources[slotIdx].IsSet() )
                {
                    return m_resources[slotIdx].GetResourceID();
                }
            }

            return ResourceID();
        }

        //-------------------------------------------------------------------------

        #if EE_DEVELOPMENT_TOOLS
        String const& GetNodePath( int16_t nodeIdx ) const { return m_nodePaths[nodeIdx]; }
        #endif

    protected:

        StringID                                    m_variationID;
        TResourcePtr<Skeleton>                      m_skeleton = nullptr;

        // Graph Definition
        //-------------------------------------------------------------------------

        TVector<int16_t>                            m_persistentNodeIndices;
        TVector<uint32_t>                           m_instanceNodeStartOffsets;
        uint32_t                                    m_instanceRequiredMemory = 0;
        uint32_t                                    m_instanceRequiredAlignment = 0;
        int16_t                                     m_rootNodeIdx = InvalidIndex;
        TVector<StringID>                           m_controlParameterIDs;
        TVector<StringID>                           m_virtualParameterIDs;
        TVector<int16_t>                            m_virtualParameterNodeIndices;
        TVector<InternalGraphSlot>                  m_internalGraphSlots;
        TVector<ExternalGraphSlot>                  m_externalGraphSlots;
        TVector<ExternalPoseSlot>                   m_externalPoseSlots;

        #if EE_DEVELOPMENT_TOOLS
        TVector<String>                             m_nodePaths;
        #endif

        THashMap<StringID, int16_t>                 m_parameterLookupMap; // Filled by the animation graph loader
        TVector<GraphNode::Definition*>             m_nodeDefinitions; // Filled by the animation graph loader

        // Dataset
        //-------------------------------------------------------------------------

        TVector<Resource::ResourcePtr>              m_resources;

        // Used to lookup all resource in this dataset as well any child datasets.
        // This is basically a flattened list of all resources references by this dataset.
        ResourceLUT                                 m_resourceLUT; // Filled by the animation graph loader
    };
}