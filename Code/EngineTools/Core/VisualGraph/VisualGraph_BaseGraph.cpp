#include "VisualGraph_BaseGraph.h"
#include "VisualGraph_UserContext.h"
#include "Base/Serialization/TypeSerialization.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Math/Line.h"

//-------------------------------------------------------------------------

namespace EE::VisualGraph
{
    template<typename T>
    static void TraverseHierarchy( T* pNode, TVector<T*>& nodePath )
    {
        EE_ASSERT( pNode != nullptr );
        nodePath.insert( nodePath.begin(), pNode );

        if ( pNode->HasParentGraph() && !pNode->GetParentGraph()->IsRootGraph() )
        {
            TraverseHierarchy( pNode->GetParentGraph()->GetParentNode(), nodePath );
        }
    }

    //-------------------------------------------------------------------------

    Color const BaseNode::s_genericNodeSeparatorColor = Color( 100, 100, 100 );
    Color const BaseNode::s_genericNodeInternalRegionDefaultColor = Color( 40, 40, 40 );

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

    bool BaseNode::HasParentNode() const
    {
        return m_pParentGraph->GetParentNode() != nullptr;
    }

    BaseNode* BaseNode::GetParentNode()
    {
        return m_pParentGraph->GetParentNode();
    }

    BaseNode const* BaseNode::GetParentNode() const
    {
        return m_pParentGraph->GetParentNode();
    }

    String BaseNode::GetStringPathFromRoot() const
    {
        TVector<BaseNode const*> path;
        TraverseHierarchy( this, path );

        //-------------------------------------------------------------------------

        String pathString;
        for ( auto iter = path.begin(); iter != path.end(); ++iter )
        {
            pathString += ( *iter )->GetName();
            if ( iter != ( path.end() - 1 ) )
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
        for ( auto iter = path.begin(); iter != path.end(); ++iter )
        {
            pathFromRoot.emplace_back( ( *iter )->GetID() );
        }

        return pathFromRoot;
    }

    TVector<BaseNode*> BaseNode::GetNodePathFromRoot() const
    {
        TVector<BaseNode*> path;
        TraverseHierarchy( const_cast<BaseNode*>( this ), path );
        return path;
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

    void BaseNode::SetPosition( Float2 const& newPosition )
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

    Color BaseNode::GetNodeBorderColor( DrawContext const& ctx, UserContext* pUserContext, NodeVisualState visualState ) const
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
            return Colors::Transparent;
        }
    }

    ImRect BaseNode::GetRect() const
    {
        ImVec2 const nodeMargin = GetNodeMargin();
        ImVec2 const rectMin = ImVec2( m_canvasPosition ) - nodeMargin;
        ImVec2 const rectMax = ImVec2( m_canvasPosition ) + nodeMargin + m_size;
        return ImRect( rectMin, rectMax );
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

    void BaseNode::DrawInternalSeparator( DrawContext const& ctx, Color color, float preMarginY, float postMarginY ) const
    {
        auto const& style = ImGui::GetStyle();
        float const scaledPreMargin = ctx.CanvasToWindow( ( preMarginY < 0 ) ? 0 : preMarginY );
        float const scaledPostMargin = ctx.CanvasToWindow( ( postMarginY < 0 ) ? style.ItemSpacing.y : postMarginY );

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + scaledPreMargin );

        float const nodeWindowWidth = ctx.CanvasToWindow( GetWidth() );
        if ( nodeWindowWidth != 0 )
        {
            ImVec2 const cursorScreenPos = ctx.WindowToScreenPosition( ImGui::GetCursorPos() );
            ctx.m_pDrawList->AddLine( cursorScreenPos, cursorScreenPos + ImVec2( nodeWindowWidth, 0 ), color );
        }

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + scaledPostMargin + 1 );
    }

    void BaseNode::BeginDrawInternalRegion( DrawContext const& ctx, Color color, float preMarginY, float postMarginY ) const
    {
        EE_ASSERT( !m_regionStarted );

        auto const& style = ImGui::GetStyle();
        float const scaledPreMargin = ctx.CanvasToWindow( ( preMarginY < 0 ) ? 0 : preMarginY );
        float const scaledPostMargin = ctx.CanvasToWindow( ( postMarginY < 0 ) ? 0 : postMarginY );

        m_internalRegionStartY = ImGui::GetCursorPosY();
        m_internalRegionColor = color;
        m_internalRegionMargins[0] = scaledPreMargin;
        m_internalRegionMargins[1] = scaledPostMargin;
        m_regionStarted = true;

        //-------------------------------------------------------------------------

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + style.FramePadding.y + scaledPreMargin );
        ImGui::Indent();
    }

    void BaseNode::EndDrawInternalRegion( DrawContext const& ctx ) const
    {
        EE_ASSERT( m_regionStarted );

        auto const& style = ImGui::GetStyle();

        ImGui::SameLine();
        ImGui::Dummy( ImVec2( style.IndentSpacing, 0 ) );
        ImGui::Unindent();

        //-------------------------------------------------------------------------

        float const scaledRectRounding = ctx.CanvasToWindow( 3.0f );

        ImVec2 const& scaledFramePadding = style.FramePadding;
        float const nodeWindowWidth = ctx.CanvasToWindow( GetWidth() );
        ImVec2 const nodeMargin = ctx.CanvasToWindow( GetNodeMargin() );

        ImVec2 const rectMin = ctx.WindowToScreenPosition( ImVec2( ImGui::GetCursorPosX(), m_internalRegionStartY + m_internalRegionMargins[0] ) );
        ImVec2 const rectMax = rectMin + ImVec2( nodeWindowWidth, ImGui::GetCursorPosY() - m_internalRegionStartY + scaledFramePadding.y - m_internalRegionMargins[0] );
        
        int32_t const previousChannel = ctx.m_pDrawList->_Splitter._Current;
        ctx.SetDrawChannel( (uint8_t) DrawChannel::ContentBackground );
        ctx.m_pDrawList->AddRectFilled( rectMin, rectMax, m_internalRegionColor, scaledRectRounding );
        ctx.m_pDrawList->ChannelsSetCurrent( previousChannel );

        ImGui::Dummy( ImVec2( nodeWindowWidth, scaledFramePadding.y + m_internalRegionMargins[1] ) );

        m_internalRegionStartY = -1.0f;
        m_internalRegionMargins[0] = m_internalRegionMargins[1] = 0.0f;
        m_regionStarted = false;
    }

    void BaseNode::ResetCalculatedNodeSizes()
    {
        m_size = m_titleRectSize = Float2::Zero;
    }

    //-------------------------------------------------------------------------

    ResizeHandle CommentNode::GetHoveredResizeHandle( DrawContext const& ctx ) const
    {
        float const selectionThreshold = ctx.WindowToCanvas( CommentNode::s_resizeSelectionRadius );
        float const cornerSelectionThreshold = selectionThreshold * 2;

        ImRect nodeRect = GetRect();
        nodeRect.Expand( selectionThreshold / 2 );

        // Corner tests first
        //-------------------------------------------------------------------------

        Vector const mousePos( ctx.m_mouseCanvasPos );
        float const distanceToCornerTL = mousePos.GetDistance2( nodeRect.GetTL() );
        float const distanceToCornerBL = mousePos.GetDistance2( nodeRect.GetBL() );
        float const distanceToCornerTR = mousePos.GetDistance2( nodeRect.GetTR() );
        float const distanceToCornerBR = mousePos.GetDistance2( nodeRect.GetBR() );

        if ( distanceToCornerBR < cornerSelectionThreshold )
        {
            return ResizeHandle::SE;
        }
        else if ( distanceToCornerBL < cornerSelectionThreshold )
        {
            return ResizeHandle::SW;
        }
        else if( distanceToCornerTR < cornerSelectionThreshold )
        {
            return ResizeHandle::NE;
        }
        else if ( distanceToCornerTL < cornerSelectionThreshold )
        {
            return ResizeHandle::NW;
        }

        // Edge tests
        //-------------------------------------------------------------------------

        ImVec2 const points[4] =
        {
            ImLineClosestPoint( nodeRect.GetTL(), nodeRect.GetTR(), ctx.m_mouseCanvasPos ),
            ImLineClosestPoint( nodeRect.GetBL(), nodeRect.GetBR(), ctx.m_mouseCanvasPos ),
            ImLineClosestPoint( nodeRect.GetTL(), nodeRect.GetBL(), ctx.m_mouseCanvasPos ),
            ImLineClosestPoint( nodeRect.GetTR(), nodeRect.GetBR(), ctx.m_mouseCanvasPos )
        };

        float distancesSq[4] =
        {
            ImLengthSqr( points[0] - ctx.m_mouseCanvasPos ),
            ImLengthSqr( points[1] - ctx.m_mouseCanvasPos ),
            ImLengthSqr( points[2] - ctx.m_mouseCanvasPos ),
            ImLengthSqr( points[3] - ctx.m_mouseCanvasPos )
        };

        float const detectionDistanceSq = Math::Sqr( selectionThreshold );

        if ( distancesSq[0] < detectionDistanceSq )
        {
            return ResizeHandle::N;
        }
        else if( distancesSq[1] < detectionDistanceSq )
        {
            return ResizeHandle::S;
        }
        else if ( distancesSq[2] < detectionDistanceSq )
        {
            return ResizeHandle::W;
        }
        else if ( distancesSq[3] < detectionDistanceSq )
        {
            return ResizeHandle::E;
        }

        return ResizeHandle::None;
    }

    void CommentNode::AdjustSizeBasedOnMousePosition( DrawContext const& ctx, ResizeHandle handle )
    {
        Float2 const unscaledBoxTL = GetPosition();
        Float2 const unscaledBoxBR = unscaledBoxTL + GetCommentBoxSize();

        switch ( handle )
        {
            case ResizeHandle::N:
            {
                m_canvasPosition.m_y = ctx.m_mouseCanvasPos.y;
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_y - m_canvasPosition.m_y );
                m_canvasPosition.m_y = unscaledBoxBR.m_y - m_commentBoxSize.m_y;
            }
            break;

            case ResizeHandle::NE:
            {
                m_canvasPosition.m_y = ctx.m_mouseCanvasPos.y;
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_y - m_canvasPosition.m_y );
                m_canvasPosition.m_y = unscaledBoxBR.m_y - m_commentBoxSize.m_y;
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.x - unscaledBoxTL.m_x );
            }
            break;

            case ResizeHandle::E:
            {
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.x - unscaledBoxTL.m_x );
            }
            break;

            case ResizeHandle::SE:
            {
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.x - unscaledBoxTL.m_x );
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.y - unscaledBoxTL.m_y );
            }
            break;

            case ResizeHandle::S:
            {
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.y - unscaledBoxTL.m_y );
            }
            break;

            case ResizeHandle::SW:
            {
                m_canvasPosition.m_x = ctx.m_mouseCanvasPos.x;
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_x - m_canvasPosition.m_x );
                m_canvasPosition.m_x = unscaledBoxBR.m_x - m_commentBoxSize.m_x;
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, ctx.m_mouseCanvasPos.y - unscaledBoxTL.m_y );
            }
            break;

            case ResizeHandle::W:
            {
                m_canvasPosition.m_x = ctx.m_mouseCanvasPos.x;
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_x - m_canvasPosition.m_x );
                m_canvasPosition.m_x = unscaledBoxBR.m_x - m_commentBoxSize.m_x;
            }
            break;

            case ResizeHandle::NW:
            {
                m_canvasPosition.m_x = ctx.m_mouseCanvasPos.x;
                m_commentBoxSize.m_x = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_x - m_canvasPosition.m_x );
                m_canvasPosition.m_x = unscaledBoxBR.m_x - m_commentBoxSize.m_x;

                m_canvasPosition.m_y = ctx.m_mouseCanvasPos.y;
                m_commentBoxSize.m_y = Math::Max( s_minBoxDimensions, unscaledBoxBR.m_y - m_canvasPosition.m_y );
                m_canvasPosition.m_y = unscaledBoxBR.m_y - m_commentBoxSize.m_y;
            }
            break;

            default:
            EE_UNREACHABLE_CODE();
            break;
        }
    }

    Color CommentNode::GetCommentBoxColor( DrawContext const& ctx, UserContext* pUserContext, NodeVisualState visualState ) const
    {
        if ( visualState == NodeVisualState::Hovered )
        {
            return m_nodeColor.GetScaledColor( 1.5f );
        }
        else if ( visualState == NodeVisualState::Selected )
        {
            return m_nodeColor.GetScaledColor( 1.25f );
        }
        else
        {
            return m_nodeColor;
        }
    }

    bool CommentNode::DrawContextMenuOptions( DrawContext const& ctx, UserContext* pUserContext, Float2 const& mouseCanvasPos )
    {
        // Generate a default palette. The palette will persist and can be edited.
        constexpr static int32_t const paletteSize = 32;
        static bool wasPaletteInitialized = false;
        static ImVec4 palette[paletteSize] = {};
        if ( !wasPaletteInitialized )
        {
            palette[0] = Color( 0xFF4C4C4C ).ToFloat4();

            for ( int32_t n = 1; n < paletteSize; n++ )
            {
                ImGui::ColorConvertHSVtoRGB( n / 31.0f, 0.8f, 0.8f, palette[n].x, palette[n].y, palette[n].z );
            }
            wasPaletteInitialized = true;
        }

        //-------------------------------------------------------------------------

        bool result = false;

        if ( ImGui::BeginMenu( EE_ICON_PALETTE" Color" ) )
        {
            for ( int32_t n = 0; n < paletteSize; n++ )
            {
                ImGui::PushID( n );
                if ( ( n % 8 ) != 0 )
                {
                    ImGui::SameLine( 0.0f, ImGui::GetStyle().ItemSpacing.y );
                }

                ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
                if ( ImGui::ColorButton( "##paletteOption", palette[n], palette_button_flags, ImVec2( 20, 20 ) ) )
                {
                    m_nodeColor = Color( ImVec4( palette[n].x, palette[n].y, palette[n].z, 1.0f ) ); // Preserve alpha!
                    result = true;
                }

                ImGui::PopID();
            }

            ImGui::EndMenu();
        }

        return result;
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
        auto GeneratePotentiallyUniqueName = [] ( InlineString const& baseName, int32_t counterValue )
        {
            int32_t suffixLength = 0;
            while ( isdigit( baseName[ baseName.length() - 1 - suffixLength ] ) )
            {
                suffixLength++;
            }

            InlineString finalName = baseName.substr( 0, baseName.length() - suffixLength );
            finalName.append_sprintf( "%u", counterValue );
            return finalName;
        };

        //-------------------------------------------------------------------------

        EE_ASSERT( !desiredName.empty() );

        String uniqueName = desiredName.c_str();
        bool isNameUnique = false;
        int32_t suffixCounter = 0;

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
                uniqueName = GeneratePotentiallyUniqueName( desiredName.c_str(), suffixCounter ).c_str();
                suffixCounter++;
            }
        }

        return uniqueName;
    }
}