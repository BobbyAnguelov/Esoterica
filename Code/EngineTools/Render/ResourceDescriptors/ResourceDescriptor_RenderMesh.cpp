#include "ResourceDescriptor_RenderMesh.h"
#include "Base/ThirdParty/pugixml/src/pugixml.hpp"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void MeshResourceDescriptor::GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const
    {
        if ( m_meshGroup.IsValid() )
        {
            outDependencies.emplace_back( m_meshGroup, false );

            Log log;
            MeshGroup meshGroup;
            IDataFile::ReadResult const result = MeshGroup::TryReadFromFile( typeRegistry, log, m_meshGroup.GetFileSystemPath( sourceResourceDirectoryPath ), meshGroup );
            if ( result.m_succeeded )
            {
                DataPath const meshPathDir = m_meshPath.GetParentDirectory();
                String const meshFilename = m_meshPath.GetFilenameWithoutExtension();
                FileSystem::Extension const extension = m_meshPath.GetExtension();

                for ( MeshLODSettings const& lod : meshGroup.m_lodSettings )
                {
                    if ( !lod.m_filenameSuffix.empty() )
                    {
                        DataPath meshPath( String( String::CtorSprintf(), "%s%s%s.%s", meshPathDir.c_str(), meshFilename.c_str(), lod.m_filenameSuffix.c_str(), extension.c_str() ) );
                        outDependencies.emplace_back( meshPath, false );
                    }
                }
            }
        }

        if ( m_meshPath.IsValid() )
        {
            outDependencies.emplace_back( m_meshPath, false );
        }
    }

    void MeshResourceDescriptor::GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const
    {
        for ( auto const& materialMapping : m_materialMappings )
        {
            if ( materialMapping.m_material.GetResourceID().IsValid() )
            {
                VectorEmplaceBackUnique( outDependencies, materialMapping.m_material.GetResourceID() );
            }
        }
    }

    void MeshResourceDescriptor::Clear()
    {
        m_meshPath.Clear();
        m_meshesToInclude.clear();
        m_materialMappings.clear();
        m_meshGroup.Clear();
    }

    bool MeshResourceDescriptor::FixUpMaterialMappings( Mesh const* pMesh )
    {
        TVector<MeshMaterialMapping> currentMaterialMappings = m_materialMappings;
        TVector<MeshMaterialMapping> desiredMappings;

        bool allMaterialIDsUnset = true;
        for ( auto const& mapping : m_materialMappings )
        {
            if ( mapping.m_mappingID.IsValid() )
            {
                allMaterialIDsUnset = false;
                break;
            }
        }

        // Create required mappings
        for ( Mesh::Submesh const& submesh : pMesh->GetSubmeshes() )
        {
            MeshMaterialMapping& mapping = desiredMappings.emplace_back();
            mapping.m_mappingID = MeshMaterialMapping::GetMappingID( submesh );
            EE_ASSERT( mapping.m_mappingID.IsValid() );
        }

        //-------------------------------------------------------------------------

        bool needsUpdate = false;

        if ( allMaterialIDsUnset )
        {
            for ( size_t i = 0; i < Math::Min( m_materialMappings.size(), desiredMappings.size() ); i++ )
            {
                desiredMappings[i].m_material = m_materialMappings[i].m_material;
            }

            // Duplicate the last material across all new mappings
            m_materialMappings.resize( desiredMappings.size() );
            if ( desiredMappings.size() > m_materialMappings.size() )
            {
                for ( size_t i = m_materialMappings.size(); i < desiredMappings.size(); i++ )
                {
                    desiredMappings[i].m_material = m_materialMappings.back().m_material;
                }
            }

            m_materialMappings = desiredMappings;
            needsUpdate = true;
        }
        else
        {
            // Generate valid mappings
            //-------------------------------------------------------------------------

            for ( MeshMaterialMapping& mapping : desiredMappings )
            {
                // Try to find a matching material
                for ( int32_t d = int32_t( currentMaterialMappings.size() ) - 1; d >= 0; d-- )
                {
                    if ( currentMaterialMappings[d].m_mappingID == mapping.m_mappingID )
                    {
                        mapping.m_material = currentMaterialMappings[d].m_material;
                        break;
                    }
                }
            }

            // Check if any differences
            //-------------------------------------------------------------------------

            needsUpdate = m_materialMappings.size() != desiredMappings.size();
            if ( !needsUpdate )
            {
                for ( size_t i = 0; i < m_materialMappings.size(); i++ )
                {
                    if ( m_materialMappings[i].m_mappingID != desiredMappings[i].m_mappingID )
                    {
                        needsUpdate = true;
                        break;
                    }

                    if ( m_materialMappings[i].m_material != desiredMappings[i].m_material )
                    {
                        needsUpdate = true;
                        break;
                    }
                }
            }

            if ( needsUpdate )
            {
                m_materialMappings = desiredMappings;
            }
        }

        return needsUpdate;
    }
}