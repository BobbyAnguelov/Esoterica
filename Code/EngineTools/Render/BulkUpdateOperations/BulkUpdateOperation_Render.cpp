#include "BulkUpdateOperation_Render.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderTexture.h"
#include "EngineTools/Core/ToolsContext.h"
#include "Engine/Render/RenderMesh.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    TInlineVector<ResourceTypeID, 4> FixUpMaterialAssignments::GetApplicableTypeIDs() const
    {
        TInlineVector<ResourceTypeID, 4> types = { SkeletalMesh::GetStaticResourceTypeID(), StaticMesh::GetStaticResourceTypeID() };
        return types;
    }

    bool FixUpMaterialAssignments::PerformOperation( ToolsContext const* pToolsContext, Resource::IResource const* pResource, Resource::ResourceDescriptor &resourceDesc )
    {
        Mesh const* pMesh = static_cast<Mesh const*>( pResource );
        MeshResourceDescriptor* pMeshDesc = Cast<MeshResourceDescriptor>( &resourceDesc );
        return pMeshDesc->FixUpMaterialMappings( pMesh );
    }

    //-------------------------------------------------------------------------

    TInlineVector<EE::ResourceTypeID, 4> FixUpTextureReferences::GetApplicableTypeIDs() const
    {
        TInlineVector<ResourceTypeID, 4> types = { TextureResource::GetStaticResourceTypeID() };
        return types;
    }

    bool FixUpTextureReferences::PerformOperation( ToolsContext const* pToolsContext, Resource::IResource const* pResource, Resource::ResourceDescriptor &resourceDesc )
    {
        TextureResourceDescriptor* pTextureDesc = Cast<TextureResourceDescriptor>( &resourceDesc );

        bool modified = false;
        for ( auto& sourcePath : pTextureDesc->m_sourcePaths )
        {
            FileSystem::Path path = sourcePath.GetFileSystemPath( pToolsContext->GetSourceDataDirectory() );
            if ( !path.Exists() )
            {
                path.ReplaceExtension( "tga" );
                if ( path.Exists() )
                {
                    sourcePath = DataPath( path, pToolsContext->GetSourceDataDirectory() );
                    modified = true;
                }
            }
        }

        return modified;
    }
}