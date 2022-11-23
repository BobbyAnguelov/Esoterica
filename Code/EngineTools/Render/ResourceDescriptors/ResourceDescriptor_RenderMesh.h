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

        virtual bool IsValid() const override { return m_meshPath.IsValid(); }

        virtual void GetCompileDependencies( TVector<ResourceID>& outDependencies ) override
        {
            if ( m_meshPath.IsValid() )
            {
                outDependencies.emplace_back( m_meshPath );
            }
        }

    public:

        // The path to the mesh source file
        EE_EXPOSE ResourcePath                         m_meshPath;

        // Optional value that specifies the specific sub-mesh to compile, if this is not set, all sub-meshes contained in the source will be combined into a single mesh object
        EE_EXPOSE String                               m_meshName;

        // Default materials - TODO: extract from source files
        EE_EXPOSE TVector<TResourcePtr<Material>>      m_materials;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API StaticMeshResourceDescriptor : public MeshResourceDescriptor
    {
        EE_REGISTER_TYPE( StaticMeshResourceDescriptor );

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return StaticMesh::GetStaticResourceTypeID(); }

    public:

        // This allows you to perform non-uniform scaling/mirroring at import time since the engine does not support non-uniform scaling
        EE_EXPOSE Float3                               m_scale = Float3( 1.0f, 1.0f, 1.0f );
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API SkeletalMeshResourceDescriptor : public MeshResourceDescriptor
    {
        EE_REGISTER_TYPE( SkeletalMeshResourceDescriptor );

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return SkeletalMesh::GetStaticResourceTypeID(); }
    };
}