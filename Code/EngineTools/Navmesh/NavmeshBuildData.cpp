#include "NavmeshBuildData.h"
#include "Engine/Entity/Entity.h"
#include "Engine/Entity/EntityDescriptors.h"
#include "Engine/Physics/Components/Component_PhysicsCollisionMesh.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsCollisionMesh.h"
#include "EngineTools/Import/Importer.h"

//-------------------------------------------------------------------------

namespace EE::Navmesh
{
    NavmeshBuildData::~NavmeshBuildData()
    {
        Clear();
    }

    void NavmeshBuildData::Clear()
    {
        ClearLog();

        m_collisionMeshInstances.clear();

        for ( auto& pair : m_importedCollisionMeshes )
        {
            EE::Delete( pair.second );
        }
        m_importedCollisionMeshes.clear();

        m_collisionMeshesLoaded = false;
    }

    bool NavmeshBuildData::HasData() const
    {
        if ( !m_collisionMeshInstances.empty() )
        {
            return true;
        }

        if ( !m_importedCollisionMeshes.empty() )
        {
            return true;
        }

        return false;
    }

    void NavmeshBuildData::ExtractBuildData( TypeSystem::TypeRegistry const& typeRegistry, EntityModel::EntityCollection const& entityCollection )
    {
        EE_ASSERT( entityCollection.IsValid() );

        Clear();

        // Search collection for relevant components
        //-------------------------------------------------------------------------

        TVector<EntityModel::EntityCollection::SearchResult> const foundNavmeshComponents = entityCollection.GetComponentsOfType( typeRegistry, NavmeshComponent::GetStaticTypeID() );
        TVector<EntityModel::EntityCollection::SearchResult> const foundCollisionComponents = entityCollection.GetComponentsOfType( typeRegistry, Physics::CollisionMeshComponent::GetStaticTypeID() );

        // Create entities
        //-------------------------------------------------------------------------

        TVector<Entity*> createdEntities = entityCollection.CreateEntities( typeRegistry );

        // Update all spatial transforms
        // We need to do this since we dont update world transforms when loading a collection
        for ( auto pEntity : createdEntities )
        {
            if ( pEntity->IsSpatialEntity() )
            {
                pEntity->SetWorldTransform( pEntity->GetWorldTransform() );
            }
        }

        // Extract component data
        //-------------------------------------------------------------------------

        for ( auto const& searchResult : foundNavmeshComponents )
        {
            Entity const* pEntity = createdEntities[searchResult.m_entityDescIdx];
            auto pNavmeshComponent = Cast<NavmeshComponent>( pEntity->GetComponents()[searchResult.m_componentDescIdx] );
            EE_ASSERT( pNavmeshComponent != nullptr );
            ExtractComponentData( pNavmeshComponent );
        }

        for ( auto const& searchResult : foundCollisionComponents )
        {
            Entity const* pEntity = createdEntities[searchResult.m_entityDescIdx];
            auto pPhysicsComponent = Cast<Physics::CollisionMeshComponent>( pEntity->GetComponents()[searchResult.m_componentDescIdx] );
            EE_ASSERT( pPhysicsComponent != nullptr );
            ExtractComponentData( pPhysicsComponent );
        }

        // Free created entities
        //-------------------------------------------------------------------------

        for ( auto& pEntity : createdEntities )
        {
            EE::Delete( pEntity );
        }
    }

    //-------------------------------------------------------------------------

    void NavmeshBuildData::ExtractComponentData( NavmeshComponent const* pComponent )
    {
        m_buildSettings = pComponent->GetBuildSettings();
    }

    void NavmeshBuildData::ExtractComponentData( Physics::CollisionMeshComponent const* pComponent )
    {
        ResourceID const& resourceID = pComponent->GetCollisionResourceID();
        if ( !resourceID.IsValid() )
        {
            LogWarning( "Detected physics collision component with no collision mesh set ( %s )!", pComponent->GetNameID().c_str() );
            return;
        }

        MeshInstance const instance = { pComponent->GetWorldTransform(), Vector( pComponent->GetWorldNonUniformScale() ) };

        if ( instance.m_nonUniformScale.IsAnyEqualToZero3() )
        {
            LogWarning( "Detected 0 local scale for physics collision component ( %s ). This is not supported so collision will be skipped!", pComponent->GetNameID().c_str() );
            return;
        }

        m_collisionMeshInstances[resourceID].emplace_back( instance );
    }

    //-------------------------------------------------------------------------

    bool NavmeshBuildData::LoadCollisionMeshes( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceDirectoryPath, TFunction<void( ResourceID const&, Import::Source&, TVector<String>& )> customMeshLoadInfoFunction )
    {
        Import::ReaderContext readerCtx =
        {
            []( char const* pString ) { EE_LOG_WARNING( LogCategory::Navmesh, "Generation", pString ); },
            [] ( char const* pString ) { EE_LOG_ERROR( LogCategory::Navmesh, "Generation", pString ); }
        };

        Import::Source collisionFileSource;
        TVector<String> meshesToInclude;

        //-------------------------------------------------------------------------

        for ( auto const& collisionMeshInstances : m_collisionMeshInstances )
        {
            ResourceID const& resourceID = collisionMeshInstances.first;

            // Dont load duplicate meshes
            //-------------------------------------------------------------------------

            auto iter = m_importedCollisionMeshes.find( resourceID );
            if ( iter != m_importedCollisionMeshes.end() )
            {
                continue;
            }

            //-------------------------------------------------------------------------

            if ( customMeshLoadInfoFunction != nullptr )
            {
                customMeshLoadInfoFunction( resourceID, collisionFileSource, meshesToInclude );
            }
            else
            {
                // Load descriptor
                //-------------------------------------------------------------------------

                DataPath const& descriptorDataPath = resourceID.GetDataPath();

                FileSystem::Path meshDescriptorFilePath;
                if ( descriptorDataPath.IsValid() )
                {
                    meshDescriptorFilePath = descriptorDataPath.GetFileSystemPath( sourceDirectoryPath );
                }
                else
                {
                    return LogError( "Invalid source data path (%s) for physics mesh descriptor", resourceID.c_str() );
                }

                Physics::PhysicsCollisionMeshResourceDescriptor physicsCollisionDescriptor;
                if ( !Resource::ResourceDescriptor::TryReadFromFile( typeRegistry, *this, meshDescriptorFilePath, physicsCollisionDescriptor ) )
                {
                    return LogError( "Failed to read physics mesh resource descriptor from file: %s", meshDescriptorFilePath.c_str() );
                }

                meshesToInclude = physicsCollisionDescriptor.m_meshesToInclude;

                // Get collision mesh path
                //-------------------------------------------------------------------------

                FileSystem::Path collisionMeshFilePath;
                if ( physicsCollisionDescriptor.m_sourcePath.IsValid() )
                {
                    collisionMeshFilePath = physicsCollisionDescriptor.m_sourcePath.GetFileSystemPath( sourceDirectoryPath );
                }
                else
                {
                    LogWarning( "Invalid source data path (%) in physics collision descriptor: %s", physicsCollisionDescriptor.m_sourcePath.c_str(), meshDescriptorFilePath.c_str() );
                    continue;
                }

                collisionFileSource.m_path = collisionMeshFilePath;
                collisionFileSource.m_pFileData = nullptr;
            }

            // Load the mesh
            //-------------------------------------------------------------------------

            TUniquePtr<Import::Mesh> pImportedMesh = Import::Importer::ReadStaticMesh( readerCtx, collisionFileSource, meshesToInclude );
            if ( pImportedMesh == nullptr )
            {
                LogWarning( "Failed to read mesh from source file: %s" );
                continue;
            }

            EE_ASSERT( pImportedMesh->IsValid() );
            m_importedCollisionMeshes.emplace( resourceID, pImportedMesh.release() );
        }

        m_collisionMeshesLoaded = true;
        return true;
    }
}