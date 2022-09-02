#pragma once
#include "EngineTools/Core/Widgets/TreeListView.h"
#include "Engine/Entity/EntityIDs.h"
#include "System/Types/String.h"

//-------------------------------------------------------------------------

struct ImGuiWindowClass;

namespace EE
{
    class ToolsContext;
    class UpdateContext;
    class EntityWorld;
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityMap;
    class EntityEditorContext;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API EntityWorldOutliner final : public TreeListView
    {
    public:

        EntityWorldOutliner( ToolsContext const* pToolsContext, EntityEditorContext& ctx );

        void Initialize( UpdateContext const& context, uint32_t widgetUniqueID );
        void Shutdown( UpdateContext const& context );

        inline char const* GetWindowName() const { return m_windowName.c_str(); }

        void SetWorldToOutline( EntityWorld* pWorld, EntityMapID const& mapID = EntityMapID() );

        void UpdateAndDraw( UpdateContext const& context, ImGuiWindowClass* pWindowClass );

    private:

        virtual void RebuildTreeInternal() override;
        virtual void HandleDragAndDropOnItem( TreeListViewItem* pDragAndDropTargetItem ) override;

    private:

        EntityEditorContext&                m_context;
        EntityWorld*                        m_pWorld = nullptr;
        EntityMap*                          m_pMap = nullptr;
        String                              m_windowName;
    };
}