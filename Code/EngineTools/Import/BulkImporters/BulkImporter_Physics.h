#pragma once
#include "BulkImporter.h"
#include "Engine/Physics/PhysicsCollisionSettings.h"
#include "Engine/Physics/PhysicsCollisionMesh.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class PhysicsMeshBulkImporter final : public BulkImporter
    {
        EE_REFLECT_TYPE( PhysicsMeshBulkImporter );

        using BulkImporter::BulkImporter;

    private:

        virtual char const* GetTitle() const override { return "Physics Mesh"; }
        virtual TInlineVector<FileSystem::Extension, 8> GetSupportedFileExtensions() const override final { return { "fbx", "gltf", "glb" }; }
        virtual void ImportFile( FileImportContext const& ctx ) override;
        virtual ResourceTypeID GetImportedType() const override { return Physics::CollisionMesh::GetStaticResourceTypeID(); }

    protected:

        // The default collision settings
        EE_REFLECT();
        Physics::CollisionSettings   m_collisionSettings;
    };
}