#pragma once
#include "Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "Animation_ToolsGraph_Variations.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphCompilationContext;

    //-------------------------------------------------------------------------

    class ToolsGraphDefinition final : public IReflectedType
    {
        EE_REFLECT_TYPE( ToolsGraphDefinition );

    public:

        ToolsGraphDefinition() = default;

        // Resets the graph definition to a valid but empty graph
        void CreateDefaultRootGraph();

        inline bool IsValid() const { return m_rootGraph.IsSet(); }
        inline FlowGraph* GetRootGraph() { return m_rootGraph.Get(); }
        inline FlowGraph const* GetRootGraph() const { return m_rootGraph.Get(); }

        // Parameters
        //-------------------------------------------------------------------------

        // Refreshes and fixes up all parameter references in this graph
        void RefreshParameterReferences();

        // Reflects all parameters from another graph into this one
        // This will try to create a new control parameter for each control and virtual parameter present in the other graph
        void ReflectParameters( ToolsGraphDefinition const& otherGraphDefinition, bool reflectVirtualParameters, TVector<String>* pOptionalOutputLog = nullptr );

        // Variations
        //-------------------------------------------------------------------------

        inline VariationHierarchy const& GetVariationHierarchy() const { return m_variationHierarchy; }
        inline VariationHierarchy& GetVariationHierarchy() { return m_variationHierarchy; }

        inline bool IsValidVariation( StringID variationID ) const { return m_variationHierarchy.IsValidVariation( variationID ); }
        Variation const* GetVariation( StringID variationID ) const { return m_variationHierarchy.GetVariation( variationID ); }
        Variation* GetVariation( StringID variationID ) { return m_variationHierarchy.GetVariation( variationID ); }

        TInlineVector<StringID, 10> GetVariationIDs() const;

    private:

        virtual void PostDeserialize() override;

    private:

        EE_REFLECT()
        TTypeInstance<FlowGraph>                                        m_rootGraph;

        EE_REFLECT()
        VariationHierarchy                                              m_variationHierarchy;
    };
}