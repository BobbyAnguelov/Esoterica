#include "ImportedMaterial.h"

//-------------------------------------------------------------------------

namespace EE::Import
{
    static bool IsTextureFormatSupported( FileSystem::Path const& texturePath )
    {
        for ( char const* pExtension : { "png", "tga", "jpg", "jpeg", "bmp", "hdr", "exr", "gif", "psd" } )
        {
            if ( texturePath.MatchesExtension( pExtension ) )
            {
                return true;
            }
        }

        return false;
    }

    // Find same texture with alternative extension, dds is not supported
    static FileSystem::Path FindWithAlternateExtension( FileSystem::Path const& path )
    {
        EE_ASSERT( path.IsValid() && path.IsFilePath() );

        FileSystem::Path alternatePath = path;

        for ( char const* pExtension : { "png", "tga", "jpg", "jpeg", "bmp" } )
        {
            alternatePath.ReplaceExtension( pExtension );
            if ( alternatePath.IsValid() && alternatePath.Exists() )
            {
                return alternatePath;
            }
        }

        return FileSystem::Path();
    }

    static bool IsRootedPath( String const& path )
    {
        EE_ASSERT( !path.empty() );

        // \texture.png, /texture.png,
        if ( path[0] == '\\' || path[0] == '/' )
        {
            return true;
        }

        // C:\texture.png, C:texture.png
        if ( path.length() >= 2 && isalpha( (unsigned char) path[0] ) && path[1] == ':' )
        {
            return true;
        }

        return false;
    }

    static FileSystem::Path ResolveTexturePath( String originalTexturePathStr, FileSystem::Path const& searchStartDirectory, FileSystem::Path const& sourceDataDirectory )
    {
        EE_ASSERT( sourceDataDirectory.IsValid() && sourceDataDirectory.IsDirectoryPath() );
        EE_ASSERT( searchStartDirectory.IsValid() && searchStartDirectory.IsDirectoryPath() && searchStartDirectory.IsUnderDirectory( sourceDataDirectory ) );

        // Check if the path is immediately correct
        //-------------------------------------------------------------------------

        if ( originalTexturePathStr.empty() )
        {
            return FileSystem::Path();
        }

        // Replace incorrect delimiters
        StringUtils::ReplaceAllOccurrencesInPlace( originalTexturePathStr, "/", "\\" );
        FileSystem::Path originalTexturePath = IsRootedPath( originalTexturePathStr ) ? originalTexturePathStr : searchStartDirectory.GetAppended(originalTexturePathStr);
        if ( !originalTexturePath.IsValid() )
        {
            return FileSystem::Path();
        }

        if ( originalTexturePath.Exists() && originalTexturePath.IsUnderDirectory( sourceDataDirectory ) )
        {
            return IsTextureFormatSupported( originalTexturePath ) ? originalTexturePath : FileSystem::Path();
        }

        // Check in common sub-directories
        //-------------------------------------------------------------------------

        constexpr static char const * const subDirectoryNames[] = {"", "Textures", "textures"};
        constexpr int32_t const numSubDirectoriesToCheck = int32_t( sizeof( subDirectoryNames ) / sizeof( subDirectoryNames[0] ) );

        String const textureFilename = originalTexturePath.GetFilename();

        for ( int32_t i = 0; i < numSubDirectoriesToCheck; i++ )
        {
            FileSystem::Path candidatePath = ( i > 0 ) ? searchStartDirectory.GetAppended( subDirectoryNames[i], true) : searchStartDirectory;
            candidatePath.Append( textureFilename );

            if ( candidatePath.IsValid() && candidatePath.Exists() && IsTextureFormatSupported( candidatePath ) )
            {
                return candidatePath;
            }

            FileSystem::Path const alternateExtensionPath = FindWithAlternateExtension( candidatePath );
            if ( alternateExtensionPath.IsValid() )
            {
                return alternateExtensionPath;
            }
        }

        //-------------------------------------------------------------------------

        return FileSystem::Path();
    }

    //-------------------------------------------------------------------------

    void Materials::Finalize( FileSystem::Path const& sourceDataDirectory, FileSystem::Path const& overrideSearchPathDirectory )
    {
        EE_ASSERT( sourceDataDirectory.IsValid() && sourceDataDirectory.IsDirectoryPath() );

        FileSystem::Path const searchStartDirectory = overrideSearchPathDirectory.IsValid() ? overrideSearchPathDirectory : m_sourcePath.GetParentDirectory();
        EE_ASSERT( searchStartDirectory.IsValid() && searchStartDirectory.IsDirectoryPath() );
        EE_ASSERT( searchStartDirectory.IsUnderDirectory( sourceDataDirectory ) );

        for ( auto& material : m_data )
        {
            auto ResolvePathWithWarning = [this, &searchStartDirectory, &sourceDataDirectory] ( char const* const pPropertyName, Material::Property& materialProperty )
            {
                if ( materialProperty.m_rawTexturePathStr.empty() )
                {
                    return;
                }

                materialProperty.m_resolvedTexturePath = ResolveTexturePath( materialProperty.m_rawTexturePathStr, searchStartDirectory, sourceDataDirectory );
                if ( !materialProperty.HasResolvedTexture() )
                {
                    LogWarning( "Failed to resolve texture path (%s) for Property (%s)", materialProperty.m_rawTexturePathStr.c_str(), pPropertyName );
                }
            };

            ResolvePathWithWarning( "Albedo", material.m_albedo );
            ResolvePathWithWarning( "Normal", material.m_normal );
            ResolvePathWithWarning( "Metalness", material.m_metalness );
            ResolvePathWithWarning( "Roughness", material.m_roughness );
            ResolvePathWithWarning( "Occlusion", material.m_occlusion );
            ResolvePathWithWarning( "Displacement", material.m_displacement );
            ResolvePathWithWarning( "Emissive", material.m_emissive );

            //-------------------------------------------------------------------------

            // Occlusion scalar value is meaningless without an occlusion texture
            if ( !material.m_occlusion.HasTexture() )
            {
                material.m_occlusion.m_isValueSet = false;
                material.m_occlusion.m_value = 0xFFFFFFFF;
            }
        }
    }
}