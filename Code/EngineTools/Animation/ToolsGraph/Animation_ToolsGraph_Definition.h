#pragma once
#include "Graphs/Animation_ToolsGraph_FlowGraph.h"
#include "Animation_ToolsGraph_Variations.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"
#include "System/Log.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphCompilationContext;

    //-------------------------------------------------------------------------

    class ToolsGraphDefinition
    {

    public:

        ToolsGraphDefinition();
        ~ToolsGraphDefinition();

        inline bool IsValid() const { return m_pRootGraph != nullptr; }
        inline FlowGraph* GetRootGraph() { return m_pRootGraph; }
        inline FlowGraph const* GetRootGraph() const { return m_pRootGraph; }

        // Parameters
        //-------------------------------------------------------------------------

        // Refreshes and fixes up all parameter references in this graph
        void RefreshParameterReferences();

        // Reflects all parameters from another graph into this one
        // This will try to create a new control parameter for each control and virtual parameter present in the other graph
        void ReflectParameters( ToolsGraphDefinition const& otherGraphDefinition, bool reflectVirtualParameters, TVector<Log::LogEntry>* pOptionalOutputLog = nullptr );

        // Variations
        //-------------------------------------------------------------------------

        inline VariationHierarchy const& GetVariationHierarchy() const { return m_variationHierarchy; }
        inline VariationHierarchy& GetVariationHierarchy() { return m_variationHierarchy; }

        inline bool IsValidVariation( StringID variationID ) const { return m_variationHierarchy.IsValidVariation( variationID ); }
        Variation const* GetVariation( StringID variationID ) const { return m_variationHierarchy.GetVariation( variationID ); }
        Variation* GetVariation( StringID variationID ) { return m_variationHierarchy.GetVariation( variationID ); }

        // Serialization
        //-------------------------------------------------------------------------

        // Load an existing graph
        bool LoadFromJson( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphDescriptorObjectValue );

        // Saves this graph
        void SaveToJson( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const;

    private:

        // Frees all memory and resets the internal state
        void ResetInternalState();

        // Resets the graph definition to a valid but empty graph
        void ResetToDefaultState();

    private:

        FlowGraph*                                                      m_pRootGraph = nullptr;
        VariationHierarchy                                              m_variationHierarchy;
    };
}