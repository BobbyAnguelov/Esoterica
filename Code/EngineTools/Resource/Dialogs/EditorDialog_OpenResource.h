#pragma once
#include "EngineTools/Core/DialogManager.h"
#include "EngineTools/FileSystem/DataFileRegistry.h"
#include "Base/Imgui/ImguiFilter.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
}

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_ENGINETOOLS_API OpenResourceDialog : public ModalDialog
    {

    public:

        OpenResourceDialog( ToolsContext const* pToolsContext );

        virtual char const* GetTitle() const override { return "Open Resource"; }
        virtual bool IsResizeable() const override { return true; }
        virtual ImVec2 GetSize() const override { return ImVec2( 800, 600 ); }
        virtual bool Draw() override;

    private:

        void UpdateFilter();

    private:

        ToolsContext const*                             m_pToolsContext = nullptr;
        ImGuiX::FilterData                              m_filter;
        TVector<DataFileRegistry::FileInfo const*>      m_files;
        TVector<DataFileRegistry::FileInfo const*>      m_filteredFiles;
        String                                          m_buffer;
    };
}