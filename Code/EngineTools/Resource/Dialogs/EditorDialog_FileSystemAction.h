#pragma once
#include "EngineTools/Core/DialogManager.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "EngineTools/Widgets/Pickers/DataPathPicker.h"

//-------------------------------------------------------------------------

namespace EE
{
    class ToolsContext;
}

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class EE_ENGINETOOLS_API FileSystemActionDialog : public ModalDialog
    {
    public:

        enum class Action
        {
            Invalid,
            Rename,
            Delete,
        };

        enum class Stage
        {
            Confirmation,
            Execution
        };

    public:

        FileSystemActionDialog( Action action, ToolsContext const* pToolsContext, DataPath const& path );

        // Special Rename Ctor
        FileSystemActionDialog( ToolsContext const* pToolsContext, DataPath const& oldPath, DataPath const& newPath );


        virtual char const* GetTitle() const override;
        virtual bool ShowCloseButton() const { return false; }
        virtual bool IsResizeable() const override { return false; }
        virtual ImVec2 GetSize() const override;
        virtual bool Draw() override;

    private:

        void StartDelete();
        bool DrawDeleteConfirmation();
        bool DrawDeleteExecution();

        void StartRename();
        bool DrawRenameConfirmation();
        bool DrawRenameExecution();

    private:

        Action                                      m_action = Action::Invalid;
        Stage                                       m_stage = Stage::Confirmation;
        ToolsContext const*                         m_pToolsContext = nullptr;

        DataPath                                    m_path;
        DataPath                                    m_newPath; // Only relevant for rename
        bool const                                  m_allowSelectionOfNewName = true;
        ImGuiX::TextBuffer                          m_newName;

        float                                       m_actionProgress = 0.0f;
    };
}