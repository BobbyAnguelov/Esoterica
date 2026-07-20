#pragma once
#include "EngineTools/Core/EditorTool.h"
#include "Base/Resource/ResourcePtr.h"
#include "Base/Imgui/ImguiTextBuffer.h"

//-------------------------------------------------------------------------

namespace EE::Resource
{
    class BulkUpdateOperation;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API ResourceBulkUpdateEditorTool : public EditorTool
    {
        enum class Stage
        {
            None,
            ProcessNextResource,
            WaitForResourceLoad,
            PerformOperationNoLoad,
            PerformOperation,
            LoadFailed,
        };

    public:

        EE_SINGLETON_EDITOR_TOOL( ResourceBulkUpdateEditorTool );

    public:

        ResourceBulkUpdateEditorTool( ToolsContext const* pToolsContext );
        ~ResourceBulkUpdateEditorTool();

        virtual bool IsSingleWindowTool() const override { return true; }
        virtual bool HasTitlebarIcon() const override { return true; }
        virtual char const* GetTitlebarIcon() const override { return EE_ICON_HOME_GROUP; }
        virtual bool SupportsMainMenu() const { return false; }
        virtual void Initialize( UpdateContext const& context ) override;

    private:

        void DrawWindow( UpdateContext const& context, bool isFocused );

        void TryStartOperation();
        void UpdateOperation( UpdateContext const& context );
        void StopOperation();

        virtual void OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr ) override;

    private:

        TVector<TypeSystem::TypeInfo const*>    m_operationTypes;
        TypeSystem::TypeInfo const*             m_pSelectedOperationType = nullptr;

        BulkUpdateOperation*                    m_pOperation = nullptr;
        TVector<ResourceID>                     m_resourcesToProcess;
        int32_t                                 m_resourceToProcessIdx = -1;
        Stage                                   m_resourceStage = Stage::ProcessNextResource;
        ResourcePtr                             m_loadedResource;
        ImGuiX::TextBuffer                      m_logBuffer;
        Log                                     m_log;
    };
}