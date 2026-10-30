#include "ResourceDescriptor_AnimationGraph.h"
#include "EngineTools/Animation/ToolsGraph/Animation_ToolsGraph_Compilation.h"
#include "Base/ThirdParty/pugixml/src/pugixml.hpp"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    void GraphResourceDescriptor::GetAllSubResources( TVector<String>& outSubResources ) const
    {
        for ( StringID variationID : m_graphDefinition.GetVariationIDs() )
        {
            outSubResources.emplace_back( String( String::CtorSprintf(), "%s.ag", variationID.c_str() ) );
        }
    }

    void GraphResourceDescriptor::GetInstallDependencies( TypeSystem::TypeRegistry const& typeRegistry, FileSystem::Path const& sourceResourceDirectoryPath, String const& subResourceName, TVector<ResourceID>& outDependencies ) const
    {
        auto const& variationHierarachy = m_graphDefinition.GetVariationHierarchy();

        StringID variationID = Variation::s_defaultVariationID;
        if ( !subResourceName.empty() )
        {
            variationID = variationHierarachy.TryGetCaseCorrectVariationID( StringUtils::StripExtension( subResourceName ) );
        }

        //-------------------------------------------------------------------------

        GraphDefinitionCompiler definitionCompiler( typeRegistry, sourceResourceDirectoryPath );
        if ( !definitionCompiler.CompileGraph( m_graphDefinition, variationID ) )
        {
            return;
        }

        //-------------------------------------------------------------------------

        GraphDefinition const *pCompiledGraphDef = definitionCompiler.GetCompiledGraph();

        VectorEmplaceBackUnique( outDependencies, pCompiledGraphDef->GetPrimarySkeletonResourceID() );

        int32_t const numSlots = pCompiledGraphDef->GetNumSlots();
        for ( int16_t i = 0; i < numSlots; i++ )
        {
            ResourceID const& referencedResourceID = pCompiledGraphDef->GetResourceIDForSlot( i );
            EE_ASSERT( referencedResourceID.IsValid() );
            VectorEmplaceBackUnique( outDependencies, referencedResourceID );
        }
    }

    void GraphResourceDescriptor::GetAdditionalReferencedPaths( TVector<DataPath>& outReferencedPaths ) const
    {
        m_graphDefinition.GetReferencedPaths( outReferencedPaths );
    }
}