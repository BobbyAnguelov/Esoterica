#pragma once
#include "EngineTools/_Module/API.h"
#include "EngineTools/Core/PropertyGrid/PropertyGrid.h"

//-------------------------------------------------------------------------

namespace EE
{
    class UpdateContext;
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityEditorContext;

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API EntityEditorPropertyGrid
    {
    public:

        EntityEditorPropertyGrid( EntityEditorContext& ctx );
        ~EntityEditorPropertyGrid();

        void Draw( UpdateContext const& context );

    private:

        void PreEdit( PropertyEditInfo const& eventInfo );
        void PostEdit( PropertyEditInfo const& eventInfo );

    private:

        EntityEditorContext&            m_context;
        EventBindingID                  m_preEditBindingID;
        EventBindingID                  m_postEditBindingID;
        PropertyGrid                    m_propertyGrid;
    };
}