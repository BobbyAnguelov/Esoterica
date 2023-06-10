#include "VisualGraph_BaseGraph.h"
#include "VisualGraph_UserContext.h"
#include "System/Serialization/TypeSerialization.h"
#include "System/TypeSystem/TypeRegistry.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    static void TraverseHierarchy( BaseNode const* pNode, TVector<BaseNode const*>& nodePath )
    {
        EE_ASSERT( pNode != nullptr );
        nodePath.emplace_back( pNode );

        if ( pNode->HasParentGraph() && !pNode->GetParentGraph()->IsRootGraph() )
        {
            TraverseHierarchy( pNode->GetParentGraph()->GetParentNode(), nodePath );
        }
    }

    //-------------------------------------------------------------------------

    Color const BaseNode::s_genericNodeSeparatorColor = Color( 100, 100, 100 );
    Color const BaseNode::s_genericNodeInternalRegionDefaultColor = Color( 60, 60, 60 );

    //-------------------------------------------------------------------------

    BaseNode::~BaseNode()
    {
        EE_ASSERT( m_pParentGraph == nullptr );
        EE_ASSERT( m_pChildGraph == nullptr );
        EE_ASSERT( m_pSecondaryGraph == nullptr );
        EE_ASSERT( !m_ID.IsValid() );
    }

    void BaseNode::Initialize( BaseGraph* pParentGraph )
    {
        m_ID = UUID::GenerateID();
        m_pParentGraph = pParentGraph;
    }

    void BaseNode::Shutdown()
    {
        if ( m_pChildGraph != nullptr )
        {
            m_pChildGraph->Shutdown();
            EE::Delete( m_pChildGraph );
        }

        if ( m_pSecondaryGraph != nullptr )
        {
            m_pSecondaryGraph->Shutdown();
            EE::Delete( m_pSecondaryGraph );
        }

        m_pParentGraph = nullptr;
        m_ID.Clear();
    }

    BaseNode* BaseNode::TryCreateNodeFromSerializedData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue, BaseGraph* pParentGraph )
    {
        auto const& typeDataObjectValue = nodeObjectValue[s_typeDataKey];
        if ( !typeDataObjectValue.IsObject() )
        {
            return nullptr;
        }

        auto const& typeIDValue = typeDataObjectValue[Serialization::s_typeIDKey];
        if ( !typeIDValue.IsString() )
        {
            return nullptr;
        }

        TypeSystem::TypeID const nodeTypeID = typeIDValue.GetString();
        auto pTypeInfo = typeRegistry.GetTypeInfo( nodeTypeID );
        if ( pTypeInfo == nullptr )
        {
            return nullptr;
        }

        BaseNode* pNode = Serialization::TryCreateAndReadNativeType<BaseNode>( typeRegistry, nodeObjectValue[s_typeDataKey] );
        if ( pNode != nullptr )
        {
            pNode->m_pParentGraph = pParentGraph;
            pNode->Serialize( typeRegistry, nodeObjectValue );
        }
        return pNode;
    }

    void BaseNode::Destroy()
    {
        EE_ASSERT( HasParentGraph() );
        GetParentGraph()->DestroyNode( m_ID );
    }

    BaseGraph* BaseNode::GetRootGraph()
    {
        BaseGraph* pRootGraph = nullptr;

        auto pParentGraph = GetParentGraph();
        while ( pParentGraph != nullptr )
        {
            pRootGraph = pParentGraph;
            auto pParentNode = pParentGraph->GetParentNode();
            pParentGraph = ( pParentNode != nullptr ) ? pParentNode->GetParentGraph() : nullptr;
        }

        return pRootGraph;
    }

    String BaseNode::GetStringPathFromRoot() const
    {
        TVector<BaseNode const*> path;
        TraverseHierarchy( this, path );

        //-------------------------------------------------------------------------

        String pathString;
        for ( auto iter = path.rbegin(); iter != path.rend(); ++iter )
        {
            pathString += ( *iter )->GetName();
            if ( iter != ( path.rend() - 1 ) )
            {
                pathString += "/";
            }
        }

        return pathString;
    }

    TVector<UUID> BaseNode::GetIDPathFromRoot() const
    {
        TVector<BaseNode const*> path;
        TraverseHierarchy( this, path );

        //-------------------------------------------------------------------------

        TVector<UUID> pathFromRoot;
        for ( auto iter = path.rbegin(); iter != path.rend(); ++iter )
        {
            pathFromRoot.emplace_back( ( *iter )->GetID() );
        }

        return pathFromRoot;
    }

    void BaseNode::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& nodeObjectValue )
    {
        EE_ASSERT( nodeObjectValue.IsObject() );

        //-------------------------------------------------------------------------

        // Note serialization is not symmetric for the nodes, since the node creation also deserializes registered values

        //-------------------------------------------------------------------------


        if ( GetID() == UUID( "7ab864a5-2267-a142-8121-fce7b7f3e560" ) )
        {
            EE_HALT();
        }

        EE::Delete( m_pChildGraph );

        auto const childGraphValueIter = nodeObjectValue.FindMember( s_childGraphKey );
        if ( childGraphValueIter != nodeObjectValue.MemberEnd() )
        {
            EE_ASSERT( childGraphValueIter->value.IsObject() );
            auto& graphObjectValue = childGraphValueIter->value;
            m_pChildGraph = BaseGraph::CreateGraphFromSerializedData( typeRegistry, graphObjectValue, this );
        }

        //-------------------------------------------------------------------------

        EE::Delete( m_pSecondaryGraph );

        auto const secondaryGraphValueIter = nodeObjectValue.FindMember( s_secondaryChildGraphKey );
        if ( secondaryGraphValueIter != nodeObjectValue.MemberEnd() )
        {
            EE_ASSERT( secondaryGraphValueIter->value.IsObject() );
            auto& graphObjectValue = secondaryGraphValueIter->value;
            m_pSecondaryGraph = BaseGraph::CreateGraphFromSerializedData( typeRegistry, graphObjectValue, this );
        }

        //-------------------------------------------------------------------------

        SerializeCustom( typeRegistry, nodeObjectValue );
    }

    void BaseNode::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        writer.StartObject();

        writer.Key( s_typeDataKey );
        Serialization::WriteNativeType( typeRegistry, this, writer );

        //-------------------------------------------------------------------------

        SerializeCustom( typeRegistry, writer );

        //-------------------------------------------------------------------------

        if ( HasChildGraph() )
        {
            writer.Key( s_childGraphKey );
            GetChildGraph()->Serialize( typeRegistry, writer );
        }

        //-------------------------------------------------------------------------

        if ( HasSecondaryGraph() )
        {
            writer.Key( s_secondaryChildGraphKey );
            GetSecondaryGraph()->Serialize( typeRegistry, writer );
        }

        writer.EndObject();
    }

    void BaseNode::SetSecondaryGraph( BaseGraph* pGraph )
    {
        EE_ASSERT( pGraph != nullptr && m_pSecondaryGraph == nullptr );
        pGraph->Initialize( this );
        m_pSecondaryGraph = pGraph;
    }

    void BaseNode::SetChildGraph( BaseGraph* pGraph )
    {
        EE_ASSERT( pGraph != nullptr && m_pChildGraph == nullptr );
        pGraph->Initialize( this );
        m_pChildGraph = pGraph;
    }

    void BaseNode::BeginModification()
    {
        // Parent graphs should only be null during construction
        if ( m_pParentGraph )
        {
            m_pParentGraph->BeginModification();
        }
    }

    void BaseNode::EndModification()
    {
        // Parent graphs should only be null during construction
        if ( m_pParentGraph )
        {
            m_pParentGraph->EndModification();
        }
    }

    void BaseNode::SetCanvasPosition( Float2 const& newPosition )
    {
        BeginModification();
        m_canvasPosition = newPosition;
        EndModification();
    }

    UUID BaseNode::RegenerateIDs( THashMap<UUID, UUID>& IDMapping )
    {
        UUID const originalID = m_ID;
        m_ID = UUID::GenerateID();

        EE_ASSERT( IDMapping.find( originalID ) == IDMapping.end() );
        IDMapping.insert( TPair<UUID, UUID>( originalID, m_ID ) );

        if ( m_pChildGraph != nullptr )
        {
            m_pChildGraph->RegenerateIDs( IDMapping );
        }

        if ( m_pSecondaryGraph != nullptr )
        {
            m_pSecondaryGraph->RegenerateIDs( IDMapping );
        }

        return m_ID;
    }

    ImColor BaseNode::GetNodeBorderColor( DrawContext const& ctx, UserContext* pUserContext, NodeVisualState visualState ) const
    {
        if ( visualState == NodeVisualState::Active )
        {
            return s_defaultActiveColor;
        }
        else if ( visualState == NodeVisualState::Hovered )
        {
            return s_defaultHoveredColor;
        }
        else if ( visualState == NodeVisualState::Selected )
        {
            return s_defaultSelectedColor;
        }
        else
        {
            return ImColor( 0 );
        }
    }

    void BaseNode::OnDoubleClick( UserContext* pUserContext )
    {
        auto pChildGraph = GetChildGraph();
        if ( pChildGraph != nullptr )
        {
            pUserContext->NavigateTo( pChildGraph );
        }

        pUserContext->DoubleClick( this );
    }

    ImRect BaseNode::GetCanvasRect() const
    {
        ImVec2 const nodeMargin = GetNodeMargin();
        ImVec2 const rectMin = ImVec2( m_canvasPosition ) - nodeMargin;
        ImVec2 const rectMax = ImVec2( m_canvasPosition ) + m_size + nodeMargin;
        return ImRect( rectMin, rectMax );
    }

    ImRect BaseNode::GetWindowRect( Float2 const& canvasToWindowOffset ) const
    {
        ImVec2 const nodeMargin = GetNodeMargin();
        ImVec2 const rectMin = ImVec2( m_canvasPosition ) - nodeMargin - canvasToWindowOffset;
        ImVec2 const rectMax = ImVec2( m_canvasPosition ) + m_size + nodeMargin - canvasToWindowOffset;
        return ImRect( rectMin, rectMax );
    }

    void BaseNode::DrawInternalSeparator( DrawContext const& ctx, Color color, float preMarginY, float postMarginY ) const
    {
        constexpr static float const minimumNodeWidth = 30;
        float const separatorWidth = GetWidth() == 0 ? minimumNodeWidth : GetWidth();
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + preMarginY );
        ImVec2 const cursorScreenPos = ctx.WindowToScreenPosition( ImGui::GetCursorPos() );
        ctx.m_pDrawList->AddLine( cursorScreenPos, cursorScreenPos + ImVec2( separatorWidth, 0 ), ImGuiX::ToIm( color ) );
        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + postMarginY + 1 );
    }

    void BaseNode::BeginDrawInternalRegion( DrawContext const& ctx, Color color, float preMarginY, float postMarginY ) const
    {
        EE_ASSERT( !m_regionStarted );

        m_internalRegionStartY = ImGui::GetCursorPosY();
        m_internalRegionColor = color;
        m_internalRegionMargins[0] = preMarginY;
        m_internalRegionMargins[1] = postMarginY;
        m_regionStarted = true;

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + preMarginY + ImGui::GetStyle().ItemSpacing.y );
    }

    void BaseNode::EndDrawInternalRegion( DrawContext const& ctx ) const
    {
        EE_ASSERT( m_regionStarted );

        ImVec2 const& framePadding = ImGui::GetStyle().FramePadding;
        ImVec2 const rectMin = ctx.WindowToScreenPosition( ImVec2( ImGui::GetCursorPosX() - framePadding.x, m_internalRegionStartY + m_internalRegionMargins[0] ));
        ImVec2 const rectMax = rectMin + ImVec2( GetWidth() + ( framePadding.x * 2 ), ImGui::GetCursorPosY() - m_internalRegionStartY + framePadding.y - m_internalRegionMargins[0] - 1 );
        
        int32_t const previousChannel = ctx.m_pDrawList->_Splitter._Current;
        ctx.SetDrawChannel( (uint8_t) DrawChannel::ContentBackground );
        ctx.m_pDrawList->AddRectFilled( rectMin, rectMax, ImGuiX::ToIm( m_internalRegionColor ), 3.0f );
        ctx.m_pDrawList->ChannelsSetCurrent( previousChannel );

        ImGui::Dummy( ImVec2( GetWidth(), framePadding.y + m_internalRegionMargins[1] ) );

        m_internalRegionStartY = -1.0f;
        m_internalRegionMargins[0] = m_internalRegionMargins[1] = 0.0f;
        m_regionStarted = false;
    }

    //-------------------------------------------------------------------------

    TEvent<BaseGraph*> BaseGraph::s_onEndRootGraphModification;
    TEvent<BaseGraph*> BaseGraph::s_onBeginRootGraphModification;

    //-------------------------------------------------------------------------

    BaseGraph::~BaseGraph()
    {
        EE_ASSERT( m_pParentNode == nullptr );
        EE_ASSERT( m_nodes.empty() );
        EE_ASSERT( !m_ID.IsValid() );
    }

    void BaseGraph::Initialize( BaseNode* pParentNode )
    {
        m_ID = UUID::GenerateID();
        m_pParentNode = pParentNode;
    }

    void BaseGraph::Shutdown()
    {
        for ( auto pNode : m_nodes )
        {
            pNode->Shutdown();
            EE::Delete( pNode );
        }

        m_nodes.clear();
        m_pParentNode = nullptr;
        m_ID.Clear();
    }

    BaseGraph* BaseGraph::GetRootGraph()
    {
        auto pRootGraph = this;
        while ( pRootGraph->m_pParentNode != nullptr )
        {
            pRootGraph = pRootGraph->m_pParentNode->GetParentGraph();
        }

        return pRootGraph;
    }

    void BaseGraph::BeginModification()
    {
        auto pRootGraph = GetRootGraph();

        if ( pRootGraph->m_beginModificationCallCount == 0 )
        {
            if ( s_onBeginRootGraphModification.HasBoundUsers() )
            {
                s_onBeginRootGraphModification.Execute( pRootGraph );
            }
        }
        pRootGraph->m_beginModificationCallCount++;
    }

    void BaseGraph::EndModification()
    {
        auto pRootGraph = GetRootGraph();

        EE_ASSERT( pRootGraph->m_beginModificationCallCount > 0 );
        pRootGraph->m_beginModificationCallCount--;

        if ( pRootGraph->m_beginModificationCallCount == 0 )
        {
            if ( s_onEndRootGraphModification.HasBoundUsers() )
            {
                s_onEndRootGraphModification.Execute( pRootGraph );
            }
        }
    }

    void BaseGraph::FindAllNodesOfType( TypeSystem::TypeID typeID, TInlineVector<BaseNode*, 20>& results, SearchMode mode, SearchTypeMatch typeMatch ) const
    {
        for ( auto pNode : m_nodes )
        {
            if ( pNode->GetTypeID() == typeID )
            {
                results.emplace_back( pNode );
            }
            else if ( typeMatch == SearchTypeMatch::Derived )
            {
                if ( pNode->GetTypeInfo()->IsDerivedFrom( typeID ) )
                {
                    results.emplace_back( pNode );
                }
            }

            // If recursion is allowed
            if ( mode == SearchMode::Recursive )
            {
                if ( pNode->HasChildGraph() )
                {
                    pNode->GetChildGraph()->FindAllNodesOfType( typeID, results, mode, typeMatch );
                }

                //-------------------------------------------------------------------------

                if ( pNode->HasSecondaryGraph() )
                {
                    pNode->GetSecondaryGraph()->FindAllNodesOfType( typeID, results, mode, typeMatch );
                }
            }
        }
    }

    void BaseGraph::FindAllNodesOfTypeAdvanced( TypeSystem::TypeID typeID, TFunction<bool( BaseNode const* )> const& matchFunction, TInlineVector<BaseNode*, 20>& results, SearchMode mode, SearchTypeMatch typeMatch ) const
    {
        for ( auto pNode : m_nodes )
        {
            if ( pNode->GetTypeID() == typeID )
            {
                results.emplace_back( pNode );
            }
            else if ( typeMatch == SearchTypeMatch::Derived )
            {
                if ( pNode->GetTypeInfo()->IsDerivedFrom( typeID ) && matchFunction( pNode ) )
                {
                    results.emplace_back( pNode );
                }
            }

            // If recursion is allowed
            if ( mode == SearchMode::Recursive )
            {
                if ( pNode->HasChildGraph() )
                {
                    pNode->GetChildGraph()->FindAllNodesOfType( typeID, results, mode, typeMatch );
                }

                //-------------------------------------------------------------------------

                if ( pNode->HasSecondaryGraph() )
                {
                    pNode->GetSecondaryGraph()->FindAllNodesOfType( typeID, results, mode, typeMatch );
                }
            }
        }
    }

    void BaseGraph::DestroyNode( UUID nodeID )
    {
        for ( auto iter = m_nodes.begin(); iter != m_nodes.end(); ++iter )
        {
            auto pNode = *iter;
            if ( pNode->GetID() == nodeID )
            {
                BeginModification();
                PreDestroyNode( pNode );

                // Delete the node
                pNode->Shutdown();
                m_nodes.erase( iter );
                EE::Delete( pNode );

                PostDestroyNode( nodeID );
                EndModification();
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    BaseGraph* BaseGraph::CreateGraphFromSerializedData( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue, BaseNode* pParentNode )
    {
        TypeSystem::TypeID const graphTypeID = graphObjectValue[BaseNode::s_typeDataKey][Serialization::s_typeIDKey].GetString();
        auto pTypeInfo = typeRegistry.GetTypeInfo( graphTypeID );
        EE_ASSERT( pTypeInfo != nullptr );

        BaseGraph* pGraph = Serialization::TryCreateAndReadNativeType<BaseGraph>( typeRegistry, graphObjectValue[BaseNode::s_typeDataKey] );
        EE_ASSERT( pGraph != nullptr );
        pGraph->m_ID = UUID::GenerateID();
        pGraph->m_pParentNode = pParentNode;
        pGraph->Serialize( typeRegistry, graphObjectValue );
        return pGraph;
    }

    void BaseGraph::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonValue const& graphObjectValue )
    {
        EE_ASSERT( graphObjectValue.IsObject() );

        // Type Serialization
        //-------------------------------------------------------------------------

        // Note serialization is not symmetric for the graphs, since the graph creation also deserializes registered values

        // View Offset
        //-------------------------------------------------------------------------

        auto viewOffsetIter = graphObjectValue.FindMember( "ViewOffsetX" );
        if ( viewOffsetIter != graphObjectValue.MemberEnd() )
        {
            m_viewOffset.m_x = (float) viewOffsetIter->value.GetDouble();
        }

        viewOffsetIter = graphObjectValue.FindMember( "ViewOffsetY" );
        if ( viewOffsetIter != graphObjectValue.MemberEnd() )
        {
            m_viewOffset.m_y = (float) viewOffsetIter->value.GetDouble();
        }

        // Nodes
        //-------------------------------------------------------------------------

        for ( auto pNode : m_nodes )
        {
            EE::Delete( pNode );
        }
        m_nodes.clear();

        for ( auto& nodeObjectValue : graphObjectValue[s_nodesKey].GetArray() )
        {
            auto pNode = BaseNode::TryCreateNodeFromSerializedData( typeRegistry, nodeObjectValue, this );
            if ( pNode != nullptr )
            {
                m_nodes.emplace_back( pNode );
            }
        }

        // Custom Data
        //-------------------------------------------------------------------------

        SerializeCustom( typeRegistry, graphObjectValue );
    }

    void BaseGraph::Serialize( TypeSystem::TypeRegistry const& typeRegistry, Serialization::JsonWriter& writer ) const
    {
        writer.StartObject();

        // Type Serialization
        //-------------------------------------------------------------------------

        writer.Key( BaseNode::s_typeDataKey );
        Serialization::WriteNativeType( typeRegistry, this, writer );

        // View Offset
        //-------------------------------------------------------------------------

        writer.Key( "ViewOffsetX" );
        writer.Double( m_viewOffset.m_x );
        writer.Key( "ViewOffsetY" );
        writer.Double( m_viewOffset.m_y );

        // Nodes
        //-------------------------------------------------------------------------

        writer.Key( s_nodesKey );
        writer.StartArray();

        for ( auto pNode : m_nodes )
        {
            pNode->Serialize( typeRegistry, writer );
        }

        writer.EndArray();

        // Custom Data
        //-------------------------------------------------------------------------

        SerializeCustom( typeRegistry, writer );

        //-------------------------------------------------------------------------

        writer.EndObject();
    }

    String BaseGraph::GetStringPathFromRoot() const
    {
        String pathString;
        if ( IsRootGraph() )
        {
            return pathString;
        }

        //-------------------------------------------------------------------------

        TVector<BaseNode const*> path;
        TraverseHierarchy( GetParentNode(), path );

        //-------------------------------------------------------------------------

        for ( auto iter = path.rbegin(); iter != path.rend(); ++iter )
        {
            pathString += ( *iter )->GetName();
            if ( iter != ( path.rend() - 1 ) )
            {
                pathString += "/";
            }
        }

        return pathString;
    }

    TVector<UUID> BaseGraph::GetIDPathFromRoot() const
    {
        TVector<UUID> pathFromRoot;
        if ( IsRootGraph() )
        {
            return pathFromRoot;
        }

        TVector<BaseNode const*> path;
        TraverseHierarchy( GetParentNode(), path );

        //-------------------------------------------------------------------------

        for ( auto iter = path.rbegin(); iter != path.rend(); ++iter )
        {
            pathFromRoot.emplace_back( ( *iter )->GetID() );
        }

        return pathFromRoot;
    }

    UUID BaseGraph::RegenerateIDs( THashMap<UUID, UUID>& IDMapping )
    {
        UUID const originalID = m_ID;
        m_ID = UUID::GenerateID();

        EE_ASSERT( IDMapping.find( originalID ) == IDMapping.end() );
        IDMapping.insert( TPair<UUID, UUID>( originalID, m_ID ) );

        for ( auto pNode : m_nodes )
        {
            pNode->RegenerateIDs( IDMapping );
        }

        return m_ID;
    }

    void BaseGraph::OnShowGraph()
    {
        for ( auto pNode : m_nodes )
        {
            pNode->OnShowNode();
        }
    }

    void BaseGraph::OnDoubleClick( UserContext* pUserContext )
    {
        auto pParentNode = GetParentNode();
        if ( pParentNode != nullptr )
        {
            pUserContext->NavigateTo( pParentNode->GetParentGraph() );
        }

        pUserContext->DoubleClick( this );
    }

    String BaseGraph::GetUniqueNameForRenameableNode( String const& desiredName, BaseNode const* m_pNodeToIgnore ) const
    {
        String uniqueName = desiredName;
        bool isNameUnique = false;
        int32_t suffix = 0;

        while ( !isNameUnique )
        {
            isNameUnique = true;

            // Check control parameters
            for ( auto pNode : m_nodes )
            {
                // Ignore specified node
                if ( pNode == m_pNodeToIgnore )
                {
                    continue;
                }

                // Only check other renameable nodes
                if ( !pNode->IsRenameable() )
                {
                    continue;
                }

                if ( pNode->GetName() == uniqueName )
                {
                    isNameUnique = false;
                    break;
                }
            }

            if ( !isNameUnique )
            {
                uniqueName.sprintf( "%s_%d", desiredName.c_str(), suffix);
                suffix++;
            }
        }

        return uniqueName;
    }
}