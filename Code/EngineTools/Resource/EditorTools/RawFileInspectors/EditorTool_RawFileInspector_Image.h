#pragma once
#include "EngineTools/Resource/EditorTools/EditorTool_RawFileInspector.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_ENGINETOOLS_API ImageFileInspectorEditorTool : public RawFileInspectorEditorTool
    {

    public:

        ImageFileInspectorEditorTool( ToolsContext const* pToolsContext, FileSystem::Path const& rawFilePath );
        virtual void UpdateAndDrawInspectorWindow( UpdateContext const& context, bool isFocused ) override;
    };
}