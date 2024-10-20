#include "EngineTools/Widgets/Pickers.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Variations.h"
#include "EngineTools/FileSystem/FileRegistry.h"
#include "EngineTools/Core/ToolsContext.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    class GraphVariationResourceOptionProvider : public ResourcePicker::OptionProvider
    {

    public:

        virtual ResourceTypeID GetApplicableResourceTypeID() const override
        {
            return GraphVariation::GetStaticResourceTypeID();
        }

        virtual void GenerateOptions( ToolsContext const& m_toolsContext, TVector<ResourceID>& outOptions ) const override
        {
            FileSystem::Extension const resourceTypeIDString = GraphVariation::GetStaticResourceTypeID().ToString();

            TVector<FileRegistry::FileInfo const*> graphResources = m_toolsContext.m_pFileRegistry->GetAllResourceFileEntries( GraphDefinition::GetStaticResourceTypeID() );
            for ( auto const& fileEntry : graphResources )
            {
                GraphResourceDescriptor const* pGraphDescriptor = Cast<GraphResourceDescriptor>( fileEntry->m_pDataFile );
                TInlineVector<EE::StringID, 10> variationIDs = pGraphDescriptor->m_graphDefinition.GetVariationIDs();
                for ( StringID variationID : variationIDs )
                {
                    DataPath const variationVirtualFilePath = Variation::GenerateResourceDataPath( m_toolsContext.GetSourceDataDirectory(), fileEntry->m_filePath, variationID );
                    outOptions.emplace_back( variationVirtualFilePath );
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

            FileRegistry::FileInfo const* pGraphFileEntry = m_toolsContext.m_pFileRegistry->GetFileEntry( graphResourceID );
            if ( pGraphFileEntry == nullptr )
            {
                return false;
            }

            // Case insensitive variation search
            //-------------------------------------------------------------------------

            GraphResourceDescriptor const* pGraphDescriptor = Cast<GraphResourceDescriptor>( pGraphFileEntry->m_pDataFile );
            String const variationStr( variationID.c_str() );

            TInlineVector<EE::StringID, 10> variationIDs = pGraphDescriptor->m_graphDefinition.GetVariationIDs();
            for ( auto const& variation : variationIDs )
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