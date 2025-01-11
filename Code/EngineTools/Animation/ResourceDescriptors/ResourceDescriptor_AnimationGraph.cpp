#include "ResourceDescriptor_AnimationGraph.h"
#include "Base/ThirdParty/pugixml/src/pugixml.hpp"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    bool GraphResourceDescriptor::UpgradeSourceData( TypeSystem::TypeRegistry const& typeRegistry, pugi::xml_document& document, int32_t versionDetected ) const
    {
        auto UpgradeClipVariationData = [] ( pugi::xml_node propertyNode, bool isOverride = false )
        {
            ResourceID oldValue = propertyNode.attribute( "Value" ).as_string();

            propertyNode.set_name( "Type" );
            propertyNode.remove_attribute( "Value" );

            auto attr = propertyNode.attribute( "ID" );
            attr.set_value( isOverride ? "m_variationData" : "m_defaultVariationData" );

            attr = propertyNode.append_attribute( "TypeID" );
            attr.set_value( "EE::Animation::AnimationClipNode::VariationData" );

            //-------------------------------------------------------------------------

            auto newPropNode = propertyNode.append_child( "Property" );
            newPropNode.append_attribute( "ID" ).set_value( "m_animClipResourcePtr" );
            newPropNode.append_attribute( "Value" ).set_value( oldValue.c_str() );
        };

        auto UpgradeRefVariationData = [] ( pugi::xml_node propertyNode, bool isOverride = false )
        {
            ResourceID oldValue = propertyNode.attribute( "Value" ).as_string();

            propertyNode.set_name( "Type" );
            propertyNode.remove_attribute( "Value" );

            auto attr = propertyNode.attribute( "ID" );
            attr.set_value( isOverride ? "m_variationData" : "m_defaultVariationData" );

            attr = propertyNode.append_attribute( "TypeID" );
            attr.set_value( "EE::Animation::ReferencedGraphNode::VariationData" );

            //-------------------------------------------------------------------------

            auto newPropNode = propertyNode.append_child( "Property" );
            newPropNode.append_attribute( "ID" ).set_value( "m_referencedGraphResourcePtr" );
            newPropNode.append_attribute( "Value" ).set_value( oldValue.c_str() );
        };

        //-------------------------------------------------------------------------

        pugi::xpath_node_set clipNodes = document.select_nodes( "//Type[@TypeID='EE::Animation::AnimationClipToolsNode']/Property[@ID='m_defaultResourceID']" );
        for ( pugi::xpath_node const& propertyNode : clipNodes )
        {
            UpgradeClipVariationData( propertyNode.node() );
        }

        pugi::xpath_node_set refNodes = document.select_nodes( "//Type[@TypeID='EE::Animation::ReferencedGraphToolsNode']/Property[@ID='m_defaultResourceID']" );
        for ( pugi::xpath_node const& propertyNode : refNodes )
        {
            UpgradeRefVariationData( propertyNode.node() );
        }

        //-------------------------------------------------------------------------

        clipNodes = document.select_nodes( "//Type[@TypeID='EE::Animation::AnimationClipToolsNode']/Property[@ID='m_overrides']/Type[@TypeID='EE::Animation::DataSlotToolsNode::OverrideValue']/Property[@ID='m_resourceID']" );
        for ( pugi::xpath_node const& propertyNode : clipNodes )
        {
            UpgradeClipVariationData( propertyNode.node(), true );
        }

        refNodes = document.select_nodes( "//Type[@TypeID='EE::Animation::ReferencedGraphToolsNode']/Property[@ID='m_overrides']/Type[@TypeID='EE::Animation::DataSlotToolsNode::OverrideValue']/Property[@ID='m_resourceID']" );
        for ( pugi::xpath_node const& propertyNode : refNodes )
        {
            UpgradeRefVariationData( propertyNode.node(), true );
        }

        return true;
    }
}