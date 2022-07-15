#include "gltfSceneContext.h"

//-------------------------------------------------------------------------

#define CGLTF_IMPLEMENTATION
#include "EngineTools/ThirdParty/cgltf/cgltf.h"

//-------------------------------------------------------------------------

namespace EE::gltf
{
    static char const* const g_errorStrings[] =
    {
        ""
        "data_too_short",
        "unknown_format",
        "invalid_json",
        "invalid_gltf",
        "invalid_options",
        "file_not_found",
        "io_error",
        "out_of_memory",
        "legacy_gltf",
    };

    //-------------------------------------------------------------------------

    gltfSceneContext::gltfSceneContext( FileSystem::Path const& filePath )
    {
        cgltf_options options = { cgltf_file_type_invalid, 0 };

        // Parse gltf/glb files
        //-------------------------------------------------------------------------

        cgltf_result const parseResult = cgltf_parse_file( &options, filePath.c_str(), &m_pSceneData );
        if ( parseResult != cgltf_result_success )
        {
            m_error.sprintf( "Failed to load specified gltf file ( %s ) : %s", filePath.c_str(), g_errorStrings[parseResult] );
            return;
        }

        // Load all data buffers
        //-------------------------------------------------------------------------

        cgltf_result bufferLoadResult = cgltf_load_buffers( &options, m_pSceneData, filePath.c_str() );
        if ( bufferLoadResult != cgltf_result_success )
        {
            m_error.sprintf( "Failed to load gltf file buffers ( %s ) : %s", filePath.c_str(), g_errorStrings[parseResult] );
            return;
        }
    }

    gltfSceneContext::~gltfSceneContext()
    {
        if ( m_pSceneData != nullptr )
        {
            cgltf_free( m_pSceneData );
            m_pSceneData = nullptr;
        }
    }
}