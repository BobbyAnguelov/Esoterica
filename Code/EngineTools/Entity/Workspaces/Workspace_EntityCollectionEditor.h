#pragma once

#include "Workspace_EntityEditor.h"
#include "Engine/Entity/EntityDescriptors.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API EntityCollectionEditor final : public EntityEditorWorkspace
    {
    public:

        EntityCollectionEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& collectionResourceID );

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    private:

        virtual bool Save() override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual void DrawMenu( UpdateContext const& context ) override;

    private:

        TResourcePtr<SerializedEntityCollection>        m_collection;
        bool                                            m_collectionInstantiated = false;
        bool                                            m_drawGrid = true;
    };
}