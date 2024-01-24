#include "EngineTools/Resource/ResourcePicker.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"
#include "EngineTools/Resource/ResourceDatabase.h"
#include "EngineTools/Core/ToolsContext.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphVariationResourceOptionProvider : public Resource::ResourcePicker::OptionProvider
    {

    public:

        virtual ResourceTypeID GetApplicableResourceTypeID() const override
        {
            return GraphVariation::GetStaticResourceTypeID();
        }

        virtual void GenerateOptions( ToolsContext const& m_toolsContext, TVector<ResourceID>& outOptions ) const override
        {
            TInlineString<5> const resourceTypeIDString = GraphVariation::GetStaticResourceTypeID().ToString();

            TVector<Resource::ResourceDatabase::FileEntry const*> graphResources = m_toolsContext.m_pResourceDatabase->GetAllFileEntriesOfType( GraphDefinition::GetStaticResourceTypeID() );
            for ( auto const& fileEntry : graphResources )
            {
                GraphResourceDescriptor const* pGraphDescriptor = Cast<GraphResourceDescriptor>( fileEntry->m_pDescriptor );
                for ( StringID variationID : pGraphDescriptor->m_variations )
                {
                    FileSystem::Path const variationVirtualFilePath = Variation::GenerateResourceFilePath( fileEntry->m_filePath, variationID );
                    ResourceID variationResourceID = ResourceID::FromFileSystemPath( m_toolsContext.GetRawResourceDirectory(), variationVirtualFilePath );
                    outOptions.emplace_back( variationResourceID );
                }
            }
        }

        virtual bool ValidatePath( ToolsContext const& m_toolsContext, ResourceID const& resourceID ) const override
        {
            // Check if specified graph exists
            //-------------------------------------------------------------------------

            StringID variationID;
            ResourceID const graphResourceID = Variation::GetGraphResourceID( resourceID, &variationID );
            if ( !variationID.IsValid() )
            {
                return false;
            }

            Resource::ResourceDatabase::FileEntry const* pGraphFileEntry = m_toolsContext.m_pResourceDatabase->GetFileEntry( graphResourceID );
            if ( pGraphFileEntry == nullptr )
            {
                return false;
            }

            // Case insensitive variation search
            //-------------------------------------------------------------------------

            GraphResourceDescriptor const* pGraphDescriptor = Cast<GraphResourceDescriptor>( pGraphFileEntry->m_pDescriptor );
            String const variationStr( variationID.c_str() );
            for ( auto const& variation : pGraphDescriptor->m_variations )
            {
                int32_t const result = variationStr.comparei( variation.c_str() );
                if ( result == 0 )
                {
                    return true;
                }
            }

            //-------------------------------------------------------------------------

            return false;
        }
    };

    static GraphVariationResourceOptionProvider const g_optionProvider;
}