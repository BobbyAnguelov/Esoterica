#pragma once
#include "Base/TypeSystem/ReflectedType.h"

//-------------------------------------------------------------------------

namespace EE 
{
    class Vector;
    class ResourceID;
    class Gizmo;
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
        virtual void CreateEntitiesFromResource( EditorContext& context, EntityMap* pMap, ResourceID const& resourceID, Vector const& worldPosition, Vector const& surfaceNormal ) const;
    };

    // Creates a decal entity from a dropped DecalMaterial resource.
    // Decal will be aligned to the surface normal under the mouse cursor (or oriented to the camera if drop location is arbitrary).
    //-------------------------------------------------------------------------

    struct DecalMaterialViewportResourceDropHandler final : public ViewportResourceDropHandler
    {
        EE_REFLECT_TYPE( DecalMaterialViewportResourceDropHandler );

        virtual bool CanCreateEntitiesFromResourceType( ResourceTypeID resourceTypeID ) const override;
        virtual void CreateEntitiesFromResource( EditorContext& context, EntityMap* pMap, ResourceID const& resourceID, Vector const& worldPosition, Vector const& surfaceNormal ) const override;
    };

    //-------------------------------------------------------------------------

    struct GizmoViewportControls
    {
        static float GetRequiredSize( Gizmo &gizmo );
        static void Draw( Gizmo &gizmo );
    };
}