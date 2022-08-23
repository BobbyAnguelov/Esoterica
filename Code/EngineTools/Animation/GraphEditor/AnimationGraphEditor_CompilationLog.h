#pragma once
#include "System/Types/UUID.h"
#include "System/Types/Arrays.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;
namespace EE { class UpdateContext; }

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphEditorContext;
    struct ToolsNodeContext;

    //-------------------------------------------------------------------------

    class GraphCompilationLogViewer
    {
    public:

        GraphCompilationLogViewer( GraphEditorContext& editorContext );

        // Draw the control parameter editor, returns true if there is a request the calling code needs to fulfill i.e. navigation
        void UpdateAndDraw( UpdateContext const& context, ToolsNodeContext* pGraphNodeContext, ImGuiWindowClass* pWindowClass, char const* pWindowName );

        // Update the compilation results to show
        void UpdateCompilationResults( TVector<NodeCompilationLogEntry> const& compilationLog ) { m_compilationLog = compilationLog; }

    private:

        GraphEditorContext&                             m_editorContext;
        TVector<NodeCompilationLogEntry>                m_compilationLog;
    };
}