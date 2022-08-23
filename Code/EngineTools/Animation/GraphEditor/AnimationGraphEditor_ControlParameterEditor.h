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
    struct ToolsNodeContext;

    namespace GraphNodes
    {
        class ControlParameterToolsNode;
        class VirtualParameterToolsNode;
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
            ParameterPreviewState( GraphNodes::ControlParameterToolsNode* pParameter ) : m_pParameter( pParameter ) { EE_ASSERT( m_pParameter != nullptr ); }
            virtual ~ParameterPreviewState() = default;

            virtual void DrawPreviewEditor( ToolsNodeContext* pGraphNodeContext ) = 0;

        public:

            GraphNodes::ControlParameterToolsNode*     m_pParameter = nullptr;
        };

    public:

        GraphControlParameterEditor( GraphEditorContext& editorContext );
        ~GraphControlParameterEditor();

        // Draw the control parameter editor, returns true if there is a request the calling code needs to fulfill i.e. navigation
        bool UpdateAndDraw( UpdateContext const& context, ToolsNodeContext* pGraphNodeContext, ImGuiWindowClass* pWindowClass, char const* pWindowName );

        // Get the virtual parameter graph we want to show
        GraphNodes::VirtualParameterToolsNode* GetVirtualParameterToEdit() { return m_pVirtualParamaterToEdit; }

    private:

        void DrawParameterList();
        void DrawParameterPreviewControls( ToolsNodeContext* pGraphNodeContext );
        void DrawAddParameterCombo();

        void StartParameterRename( UUID const& parameterID );
        void StartParameterDelete( UUID const& parameterID );
        void DrawDialogs();

        void CreatePreviewStates();
        void DestroyPreviewStates();

    private:

        GraphEditorContext&                             m_editorContext;
        GraphNodes::VirtualParameterToolsNode*         m_pVirtualParamaterToEdit = nullptr;
        UUID                                            m_currentOperationParameterID;
        OperationType                                   m_activeOperation;
        char                                            m_parameterNameBuffer[255];
        char                                            m_parameterCategoryBuffer[255];
        TVector<ParameterPreviewState*>                 m_parameterPreviewStates;
        THashMap<UUID, int32_t>                         m_cachedNumUses;
        CountdownTimer<EngineClock>                     m_updateCacheTimer;
    };
}