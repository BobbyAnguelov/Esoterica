#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Render/Material/RenderMaterial.h"
#include "Engine/Render/Mesh/StaticMesh.h"
#include "Engine/Render/Mesh/SkeletalMesh.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Import { class ImportedMesh; }

//-------------------------------------------------------------------------

namespace EE::Render
{
    class Mesh;

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API MeshResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( MeshResourceDescriptor );

        virtual bool IsValid() const override { return m_meshPath.IsValid(); }

        virtual void GetCompileDependencies( TVector<DataPath>& outDependencies ) override
        {
            if ( m_meshPath.IsValid() )
            {
                outDependencies.emplace_back( m_meshPath );
            }
        }

        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return Mesh::GetStaticResourceTypeID(); }        

        virtual void Clear() override
        {
            m_meshPath.Clear();
            m_meshesToInclude.clear();
            m_materials.clear();
            m_mergeSectionsByMaterial = true;
        }

    public:

        // The path to the mesh source file
        EE_REFLECT();
        DataPath                                m_meshPath;

        // Optional: specifies the specific sub-meshes to compile, if this is not set, all sub-meshes contained in the source will be combined into a single mesh object
        EE_REFLECT();
        TVector<String>                         m_meshesToInclude;

        // Default materials
        EE_REFLECT();
        TVector<TResourcePtr<Material>>         m_materials;

        // Should all mesh-sections with the same materials be merged together?
        EE_REFLECT();
        bool                                    m_mergeSectionsByMaterial = true;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API StaticMeshResourceDescriptor : public MeshResourceDescriptor
    {
        EE_REFLECT_TYPE( StaticMeshResourceDescriptor );

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual int32_t GetFileVersion() const override { return 0; }
        virtual FileSystem::Extension GetExtension() const override final { return StaticMesh::GetStaticResourceTypeID().ToString(); }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return StaticMesh::GetStaticResourceTypeID(); }
        virtual char const* GetFriendlyName() const override final { return StaticMesh::s_friendlyName; }

        virtual void Clear() override
        {
            MeshResourceDescriptor::Clear();
            m_scale = Float3( 1.0f, 1.0f, 1.0f );
        }

    public:

        // This allows you to perform non-uniform scaling/mirroring at import time since the engine does not support non-uniform scaling
        EE_REFLECT();
        Float3                                  m_scale = Float3( 1.0f, 1.0f, 1.0f );
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API SkeletalMeshResourceDescriptor : public MeshResourceDescriptor
    {
        EE_REFLECT_TYPE( SkeletalMeshResourceDescriptor );

        virtual int32_t GetFileVersion() const override { return 0; }
        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual FileSystem::Extension GetExtension() const override final { return SkeletalMesh::GetStaticResourceTypeID().ToString(); }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return SkeletalMesh::GetStaticResourceTypeID(); }
        virtual char const* GetFriendlyName() const override final { return SkeletalMesh::s_friendlyName; }
    };
}