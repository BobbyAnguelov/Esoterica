#pragma once
#include "EngineTools/Resource/EditorTools/EditorTool_RawFileInspector.h"
#include "EngineTools/RawAssets/Formats/GLTF.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_ENGINETOOLS_API GLTFFileInspectorEditorTool : public RawFileInspectorEditorTool
    {

    public:

        GLTFFileInspectorEditorTool( ToolsContext const* pToolsContext, FileSystem::Path const& rawFilePath );
        virtual void UpdateAndDrawInspectorWindow( UpdateContext const& context, bool isFocused ) override;

    private:

        void ReadFileContents();

    private:

        gltf::SceneContext        m_sceneContext;
        bool                          m_refreshFileInfo = false;
    };
}