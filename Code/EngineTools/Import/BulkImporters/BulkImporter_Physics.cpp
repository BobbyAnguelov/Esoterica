#include "BulkImporter_Physics.h"
#include "EngineTools/Physics/ResourceDescriptors/ResourceDescriptor_PhysicsCollisionMesh.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    void PhysicsMeshBulkImporter::ImportFile( FileImportContext const& ctx )
    {
        Physics::PhysicsCollisionMeshResourceDescriptor descriptor;
        descriptor.m_sourcePath = ctx.m_fileToImport.m_path;
        descriptor.m_collisionSettings = m_collisionSettings;

        Log log;
        if ( !Resource::ResourceDescriptor::TryWriteToFile( ctx.m_typeRegistry, log, ctx.m_targetFilePath, &descriptor ) )
        {
            ctx.AppendLog( log );
        }
    }
}