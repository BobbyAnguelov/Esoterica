#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "System/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::RawAssets { class RawMesh; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Mesh;

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API MeshResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REGISTER_TYPE( MeshResourceDescriptor );

        // The path to the mesh source file
        EE_EXPOSE ResourcePath                         m_meshPath;

        // Default materials - TODO: extract from FBX
        EE_EXPOSE TVector<TResourcePtr<Material>>      m_materials;

        // Optional value that specifies the specific sub-mesh to compile, if this is not set, all sub-meshes contained in the source will be combined into a single mesh object
        EE_EXPOSE String                               m_meshName;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API StaticMeshResourceDescriptor : public MeshResourceDescriptor
    {
        EE_REGISTER_TYPE( StaticMeshResourceDescriptor );

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return StaticMesh::GetStaticResourceTypeID(); }
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API SkeletalMeshResourceDescriptor : public MeshResourceDescriptor
    {
        EE_REGISTER_TYPE( SkeletalMeshResourceDescriptor );

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return SkeletalMesh::GetStaticResourceTypeID(); }
    };
}