#include "ResourceDescriptor_RenderTexture.h"
#include "EngineTools/Core/ToolsContext.h"
#include "EngineTools/FileSystem/DataFileRegistry.h"
#include "EngineTools/PropertyGrid/PropertyGridTypeEditingRules.h"

//-------------------------------------------------------------------------

namespace EE::Render
{
    void TextureResourceDescriptor::GetCompileDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<Resource::CompileDependency>& outDependencies ) const
    {
        for ( DataPath const& path : m_sourcePaths )
        {
            if ( path.IsValid() )
            {
                VectorEmplaceBackUnique( outDependencies, Resource::CompileDependency( path, false ) );
            }
        }

        if ( m_computeOcclusionFromHeight && m_occlusionHeightSource.IsValid() )
        {
            VectorEmplaceBackUnique( outDependencies, Resource::CompileDependency( m_occlusionHeightSource, false ) );
        }

        if ( m_textureGroup.IsValid() )
        {
            VectorEmplaceBackUnique( outDependencies, Resource::CompileDependency( m_textureGroup, false ) );
        }
    }

    //-------------------------------------------------------------------------

    class TextureResourceDescriptorEditingRules final : public PG::TTypeEditingRules<TextureResourceDescriptor>
    {
        using PG::TTypeEditingRules<TextureResourceDescriptor>::TTypeEditingRules;

        virtual InlineString GetPropertyNameOverride( StringID const& propertyID, int32_t arrayElementIdx ) override
        {
            static StringID const s_sourcePathsPropertyID = StringID( "m_sourcePaths" );
            if ( propertyID == s_sourcePathsPropertyID && m_pTypeInstance->m_textureGroup.IsValid() )
            {
                if ( DataFileRegistry::FileInfo const* pTextureGroupFile = m_pToolsContext->m_pDataFileRegistry->GetFileEntry( m_pTypeInstance->m_textureGroup ); pTextureGroupFile && pTextureGroupFile->IsDataFile() )
                {
                    TextureGroup const* pTextureGroup = static_cast<TextureGroup const*>( pTextureGroupFile->m_pDataFile );
                    return InlineString( pTextureGroup->m_fileMasks[arrayElementIdx].m_channelName.c_str() );
                }
            }
            return InlineString();
        }
    };

    EE_PROPERTY_GRID_EDITING_RULES( TextureResourceDescriptor, TextureResourceDescriptorEditingRules );
}
