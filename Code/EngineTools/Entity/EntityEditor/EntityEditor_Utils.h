#pragma once
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE 
{
    class Vector;
    class ResourceID;
    namespace ImGuiX { class Gizmo; }
}

//-------------------------------------------------------------------------

namespace EE::EntityModel
{
    class EntityMap;
    class EditorContext;

    //-------------------------------------------------------------------------

    struct ViewportResourceDropHandler : public IReflectedType
    {
        EE_REFLECT_TYPE( ViewportResourceDropHandler );

        virtual bool CanCreateEntitiesFromResourceType( ResourceTypeID resourceTypeID ) const;
        virtual void CreateEntitiesFromResource( EditorContext& context, EntityMap* pMap, ResourceID const& resourceID, Vector const& worldPosition ) const;
    };

    //-------------------------------------------------------------------------

    struct GizmoViewportControls
    {
        static float GetRequiredSize( ImGuiX::Gizmo &gizmo );
        static void Draw( ImGuiX::Gizmo &gizmo );
    };
}