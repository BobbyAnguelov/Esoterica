#pragma once
#include "Animation_ToolsGraphNode.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class VariationDataToolsNode : public FlowToolsNode
    {
        EE_REFLECT_TYPE( VariationDataToolsNode );

    public:

        struct Data : public IReflectedType
        {
            EE_REFLECT_TYPE( Data );

            // Return any resources that we reference
            virtual void GetReferencedResources( TInlineVector<ResourceID, 2>& outReferencedResources ) const {}
        };

        //-------------------------------------------------------------------------

        struct OverrideValue : public IReflectedType
        {
            EE_REFLECT_TYPE( OverrideValue );

            EE_REFLECT( ReadOnly );
            StringID                        m_variationID;

            EE_REFLECT( ReadOnly, DisableTypePicker );
            TTypeInstance<Data>             m_variationData;
        };

    public:

        VariationDataToolsNode();

        virtual bool IsRenameable() const override { return true; }
        virtual bool RequiresUniqueName() const override final { return true; }
        virtual void DrawExtraControls( NodeGraph::DrawContext const& ctx, NodeGraph::UserContext* pUserContext ) override;

        // Data access
        //-------------------------------------------------------------------------

        // Get the default data
        Data* GetDefaultVariationData() { return m_defaultVariationData.Get(); }

        // Get the default data
        Data const* GetDefaultVariationData() const {  m_defaultVariationData.Get(); }

        // Get the default data casted to a specific type
        template<typename T>
        T* GetDefaultVariationDataAs() { return reinterpret_cast<T*>( m_defaultVariationData.Get() ); }

        // Get the default data casted to a specific type
        template<typename T>
        T const* GetDefaultVariationDataAs() const { return reinterpret_cast<T*>( m_defaultVariationData.Get() ); }

        // This will return the final resolved variation data for this slot
        Data* GetResolvedVariationData( VariationHierarchy const& variationHierarchy, StringID variationID );

        // This will return the final resolved variation data for this slot
        inline Data const* GetResolvedVariationData( VariationHierarchy const& variationHierarchy, StringID variationID ) const { return const_cast<VariationDataToolsNode*>( this )->GetResolvedVariationData( variationHierarchy, variationID ); }

        // This will return the final resolved variation data for this slot, casted to a specific type
        template<typename T>
        T* GetResolvedVariationDataAs( VariationHierarchy const& variationHierarchy, StringID variationID ) { return reinterpret_cast<T*>( GetResolvedVariationData( variationHierarchy, variationID ) ); }

        // This will return the final resolved variation data for this slot, casted to a specific type
        template<typename T>
        inline T const* GetResolvedVariationDataAs( VariationHierarchy const& variationHierarchy, StringID variationID ) const { return reinterpret_cast<T*>( const_cast<VariationDataToolsNode*>( this )->GetResolvedVariationData( variationHierarchy, variationID ) ); }

        // Variation override management
        //-------------------------------------------------------------------------

        bool HasVariationOverride( StringID variationID ) const;
        void CreateVariationOverride( StringID variationID );
        void RenameVariationOverride( StringID oldVariationID, StringID newVariationID );
        void RemoveVariationOverride( StringID variationID );

    protected:

        virtual TypeSystem::TypeInfo const* GetVariationDataTypeInfo() const = 0;

    protected:

        EE_REFLECT( Hidden );
        TTypeInstance<Data>                 m_defaultVariationData;

        EE_REFLECT( Hidden );
        TVector<OverrideValue>              m_overrides;
    };
}