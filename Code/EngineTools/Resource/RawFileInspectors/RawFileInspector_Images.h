#pragma once
#include "EngineTools/Resource/RawFileInspector.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class RawFileInspectorImages : public RawFileInspector
    {
    public:

        RawFileInspectorImages( ToolsContext const* pToolsContext, FileSystem::Path const& filePath );

    private:

        virtual char const* GetInspectorTitle() const override { return  "Image Inspector"; }
        virtual void DrawFileInfo() override;
        virtual void DrawFileContents() override;
        virtual void DrawResourceDescriptorCreator() override;
    };
}