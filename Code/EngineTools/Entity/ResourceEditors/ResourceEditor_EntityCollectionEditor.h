#pragma once

#include "ResourceEditor_EntityEditor.h"
#include "Engine/Entity/EntityDescriptors.h"

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EE_ENGINETOOLS_API EntityCollectionEditor final : public EntityEditor
    {
        EE_EDITOR_TOOL( EntityCollectionEditor );

    public:

        EntityCollectionEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& collectionResourceID );

        virtual void Initialize( UpdateContext const& context ) override;
        virtual void Shutdown( UpdateContext const& context ) override;

    private:

        virtual bool SaveData() override;
        virtual void Update( UpdateContext const& context, bool isVisible, bool isFocused ) override;
        virtual void DrawMenu( UpdateContext const& context ) override;

    private:

        TResourcePtr<SerializedEntityCollection>        m_collection;
        bool                                            m_collectionInstantiated = false;
        bool                                            m_drawGrid = true;
    };
}