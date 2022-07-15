#pragma once
#include "EngineTools/Resource/RawFileInspector.h"
#include "EngineTools/RawAssets/gltf/gltfSceneContext.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class RawFileInspectorGLTF : public RawFileInspector
    {
    public:

        RawFileInspectorGLTF( ToolsContext const* pToolsContext, FileSystem::Path const& filePath );

    private:

        virtual char const* GetInspectorTitle() const override { return  "GLTF Inspector"; }
        virtual void DrawFileInfo() override;
        virtual void DrawFileContents() override;
        virtual void DrawResourceDescriptorCreator() override;

    private:

        gltf::gltfSceneContext        m_sceneContext;
    };
}