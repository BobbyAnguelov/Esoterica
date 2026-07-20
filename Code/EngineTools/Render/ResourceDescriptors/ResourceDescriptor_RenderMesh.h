#pragma once

#include "EngineTools/_Module/API.h"
#include "EngineTools/Resource/ResourceDescriptor.h"
#include "Engine/Render/RenderMaterial.h"
#include "Engine/Render/RenderMesh.h"
#include "Base/Resource/ResourcePtr.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    //-------------------------------------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API MeshLODSettings final : public IReflectedType
    {
        EE_REFLECT_TYPE( MeshLODSettings );

        // Distance from camera to the center of the mesh where this LOD will be used
        EE_REFLECT();
        float                           m_lodDistance = 0.0f;

        // Resource path suffix that will be used to find the LOD file path
        EE_REFLECT();
        String                          m_filenameSuffix = {};

        // Should this LOD be automatically generated?
        EE_REFLECT();
        bool                            m_autoGenerateLOD = false;

        // Target triangle count for simplification when automatic LOD generation is used
        EE_REFLECT();
        uint32_t                        m_targetTriangleCount = UINT32_MAX;

        // Target triangle count percentage for simplification when automatic LOD generation is used
        EE_REFLECT();
        float                           m_targetTrianglePercentage = 100.0F;

        // Target error for simplification when automatic LOD generation is used
        EE_REFLECT();
        float                           m_targetAttributeMaxError = 1e-2F;

        // UV attribute weight for simplification when automatic LOD generation is used
        EE_REFLECT();
        float                           m_targetAttributeWeightUV = 30.0F;

        // UV attribute weight for simplification when automatic LOD generation is used
        EE_REFLECT();
        float                           m_targetAttributeWeightNormals = 1.0F;
    };

    //-------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API MeshGroup final : public IDataFile
    {
        EE_DATA_FILE( MeshGroup, "meshgrp", "Mesh Group", 1 );

    public:

        EE_REFLECT();
        TVector<MeshLODSettings>        m_lodSettings;
    };

    //-------------------------------------------------------------------------------------------------------

    struct MeshMaterialMapping final : public IReflectedType
    {
        EE_REFLECT_TYPE( MeshMaterialMapping );

        inline static StringID GetMappingID( Mesh::Submesh const& submesh )
        {
            InlineString str( InlineString::CtorSprintf(), "%s/%s/%u", submesh.m_ID.c_str(), submesh.m_materialNameID.c_str(), submesh.m_lodMask );
            return StringID( str.c_str() );
        }

    public:

        EE_REFLECT( ReadOnly );
        StringID                        m_mappingID;

        EE_REFLECT();
        TResourcePtr<Material>          m_material;
    };

    //-------------------------------------------------------------------------------------------------------

    struct MeshSocketDefinition final : public IReflectedType
    {
        EE_REFLECT_TYPE( MeshSocketDefinition );

        EE_REFLECT();
        StringID                        m_ID;

        EE_REFLECT();
        StringID                        m_boneID;

        EE_REFLECT();
        Transform                       m_offsetTransform;
    };

    //-------------------------------------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API MeshResourceDescriptor : public Resource::ResourceDescriptor
    {
        EE_REFLECT_TYPE( MeshResourceDescriptor );

    public:

        virtual bool IsValid() const override { return m_meshGroup.IsValid() && m_meshPath.IsValid(); }

        virtual void Clear() override;

        virtual int32_t GetFileVersion() const override { return 2; }

        virtual void GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const override;

        virtual void GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const override;

        // Try to fix any malformed material mappings - returns true if any changes were made
        bool FixUpMaterialMappings( Mesh const* pMesh );

    public:

        // The path to the mesh source file
        EE_REFLECT();
        DataPath                        m_meshPath;

        // Optional: specifies the specific sub-meshes to compile, if this is not set, all sub-meshes contained in the source will be combined into a single mesh object
        EE_REFLECT();
        TVector<String>                 m_meshesToInclude;

        EE_REFLECT( ReadOnly );
        TVector<MeshMaterialMapping>    m_materialMappings;

        EE_REFLECT( ReadOnly );
        TVector<MeshSocketDefinition>   m_sockets;

        // LOD Group
        EE_REFLECT();
        TDataFilePath<MeshGroup>        m_meshGroup;
    };

    //-------------------------------------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API StaticMeshResourceDescriptor final : public MeshResourceDescriptor
    {
        EE_REFLECT_TYPE( StaticMeshResourceDescriptor );

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual FileSystem::Extension GetExtension() const override final { return StaticMesh::GetStaticResourceTypeID().ToString(); }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return StaticMesh::GetStaticResourceTypeID(); }
        virtual char const* GetFriendlyName() const override final { return StaticMesh::s_friendlyName; }
    };

    //-------------------------------------------------------------------------------------------------------

    struct EE_ENGINETOOLS_API SkeletalMeshResourceDescriptor final : public MeshResourceDescriptor
    {
        EE_REFLECT_TYPE( SkeletalMeshResourceDescriptor );

        virtual bool IsUserCreateableDescriptor() const override { return true; }
        virtual FileSystem::Extension GetExtension() const override final { return SkeletalMesh::GetStaticResourceTypeID().ToString(); }
        virtual ResourceTypeID GetCompiledResourceTypeID() const override { return SkeletalMesh::GetStaticResourceTypeID(); }
        virtual char const* GetFriendlyName() const override final { return SkeletalMesh::s_friendlyName; }
    };
}
