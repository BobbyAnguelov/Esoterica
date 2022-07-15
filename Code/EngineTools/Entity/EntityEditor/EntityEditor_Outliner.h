#pragma once
#include "EngineTools/_Module/API.h"

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

    class EE_ENGINETOOLS_API EntityEditorOutliner
    {
    public:

        EntityEditorOutliner( EntityEditorContext& ctx );

        void Draw( UpdateContext const& context );

    private:

        EntityEditorContext&                m_context;
    };
}