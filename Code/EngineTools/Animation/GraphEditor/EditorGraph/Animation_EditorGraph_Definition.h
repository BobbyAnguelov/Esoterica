#pragma once
#include "Animation_EditorGraph_FlowGraph.h"
#include "Animation_EditorGraph_StateMachineGraph.h"
#include "Animation_EditorGraph_Variations.h"
#include "Engine/Animation/Graph/Animation_RuntimeGraph_Definition.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphCompilationContext;

    //-------------------------------------------------------------------------

    class EditorGraphDefinition
    {

    public:

        EditorGraphDefinition();
        ~EditorGraphDefinition();

        inline bool IsValid() const { return m_pRootGraph != nullptr; }
        inline FlowGraph* GetRootGraph() { return m_pRootGraph; }
        inline FlowGraph const* GetRootGraph() const { return m_pRootGraph; }

        // Dirty State
        //-------------------------------------------------------------------------

        inline bool IsDirty() const { return m_isDirty; }
        inline void MarkDirty() { m_isDirty = true; }
        inline void ClearDirty() { m_isDirty = false; }

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
        bool                                                            m_isDirty = false;
    };
}