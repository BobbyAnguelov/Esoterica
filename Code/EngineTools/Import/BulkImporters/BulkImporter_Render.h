#pragma once
#include "BulkImporter.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/Import/ImportedMesh.h"
#include "Base/Memory/UniquePtr.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    class RenderMeshBulkImporter : public BulkImporter
    {
        EE_REFLECT_TYPE( RenderMeshBulkImporter );

        using BulkImporter::BulkImporter;

    protected:

        virtual TInlineVector<FileSystem::Extension, 8> GetSupportedFileExtensions() const override final { return { "fbx", "gltf", "glb" }; }
        virtual bool HasRequiredSettingsForImport() const override;
        virtual InlineString GetInfoMessage() const override;

        void CreateMaterialDescriptor( FileImportContext const& ctx, Material const& material, FileSystem::Path const& materialFilePath ) const;
        void SetupMaterialsForDescriptor( FileImportContext const& ctx, TUniquePtr<Import::Mesh> const& importedMesh, Render::MeshResourceDescriptor& descriptor ) const;

    private:

        ResourceID FindOrCreateTexture( FileImportContext const& ctx, Material::Property const& materialProperty, char const* pTextureGroupName, ResourceID const& defaultTextureID ) const;
        ResourceID FindOrCreateTexturePBR( FileImportContext const& ctx, Material const& material, ResourceID const& defaultTextureID ) const;

    protected:

        EE_REFLECT();
        TDataFilePath<Render::MeshGroup>        m_meshGroup;

        EE_REFLECT();
        DataPath                                m_textureSearchOverridePath;
    };

    //-------------------------------------------------------------------------

    class StaticMeshBulkImporter final : public RenderMeshBulkImporter
    {
        EE_REFLECT_TYPE( StaticMeshBulkImporter );

    public:

        using RenderMeshBulkImporter::RenderMeshBulkImporter;

    private:

        virtual char const* GetTitle() const override { return "Static Mesh"; }
        virtual ResourceTypeID GetImportedType() const override { return Render::StaticMesh::GetStaticResourceTypeID(); }
        virtual void ImportFile( FileImportContext const& ctx ) override;
    };

    //-------------------------------------------------------------------------

    class SkeletalMeshBulkImporter final : public RenderMeshBulkImporter
    {
        EE_REFLECT_TYPE( SkeletalMeshBulkImporter );

    public:

        using RenderMeshBulkImporter::RenderMeshBulkImporter;

    private:

        virtual char const* GetTitle() const override { return "Skeletal Mesh"; }
        virtual ResourceTypeID GetImportedType() const override { return Render::SkeletalMesh::GetStaticResourceTypeID(); }
        virtual void ImportFile( FileImportContext const& ctx ) override;
    };
}