#pragma once
#include "System/Types/UUID.h"
#include "System/Types/Arrays.h"
#include "System/Types/HashMap.h"
#include "System/Time/Timers.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;
namespace EE { class UpdateContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphEditorContext;
    struct EditorGraphNodeContext;

    namespace GraphNodes
    {
        class ControlParameterEditorNode;
        class VirtualParameterEditorNode;
    }

    //-------------------------------------------------------------------------

    class GraphControlParameterEditor
    {
        enum class OperationType
        {
            None,
            Rename,
            Delete
        };

    public:

        struct ParameterPreviewState
        {
            ParameterPreviewState( GraphNodes::ControlParameterEditorNode* pParameter ) : m_pParameter( pParameter ) { EE_ASSERT( m_pParameter != nullptr ); }
            virtual ~ParameterPreviewState() = default;

            virtual void DrawPreviewEditor( EditorGraphNodeContext* pGraphNodeContext ) = 0;

        public:

            GraphNodes::ControlParameterEditorNode*     m_pParameter = nullptr;
        };

    public:

        GraphControlParameterEditor( GraphEditorContext& editorContext );
        ~GraphControlParameterEditor();

        // Draw the control parameter editor, returns true if there is a request the calling code needs to fulfill i.e. navigation
        bool UpdateAndDraw( UpdateContext const& context, EditorGraphNodeContext* pGraphNodeContext, ImGuiWindowClass* pWindowClass, char const* pWindowName );

        // Get the virtual parameter graph we want to show
        GraphNodes::VirtualParameterEditorNode* GetVirtualParameterToEdit() { return m_pVirtualParamaterToEdit; }

    private:

        void DrawParameterList();
        void DrawParameterPreviewControls( EditorGraphNodeContext* pGraphNodeContext );
        void DrawAddParameterCombo();

        void StartParameterRename( UUID const& parameterID );
        void StartParameterDelete( UUID const& parameterID );
        void DrawDialogs();

        void CreatePreviewStates();
        void DestroyPreviewStates();

    private:

        GraphEditorContext&                             m_editorContext;
        GraphNodes::VirtualParameterEditorNode*         m_pVirtualParamaterToEdit = nullptr;
        UUID                                            m_currentOperationParameterID;
        OperationType                                   m_activeOperation;
        char                                            m_parameterNameBuffer[255];
        char                                            m_parameterCategoryBuffer[255];
        TVector<ParameterPreviewState*>                 m_parameterPreviewStates;
        THashMap<UUID, int32_t>                         m_cachedNumUses;
        CountdownTimer<EngineClock>                     m_updateCacheTimer;
    };
}