#include "NodeGraph_BaseGraph.h"
#include "NodeGraph_UserContext.h"
#include "Base/Serialization/TypeSerialization.h"
#include "Base/TypeSystem/TypeRegistry.h"
#include "Base/Math/Line.h"
#include "NodeGraph_Style.h"

//-------------------------------------------------------------------------

namespace EE::NodeGraph
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

    BaseNode::~BaseNode()
    {
        EE_ASSERT( m_pParentGraph == nullptr );
        EE_ASSERT( !m_ID.IsValid() );

        if ( HasChildGraph() )
        {
            m_childGraph.DestroyInstance();
        }

        if ( HasSecondaryGraph() )
        {
            m_secondaryGraph.DestroyInstance();
        }
    }

    void BaseNode::PostDeserialize()
    {
        IReflectedType::PostDeserialize();

        // Set child graph parent ptrs
        if ( HasChildGraph() )
        {
            GetChildGraph()->m_pParentNode = this;
        }

        // Set secondary graph parent ptrs
        if ( HasSecondaryGraph() )
        {
            GetSecondaryGraph()->m_pParentNode = this;
        }
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

    void BaseNode::Rename( String const& newName )
    {
        EE_ASSERT( IsRenameable() );
        NodeGraph::ScopedNodeModification const snm( this );

        m_name = newName.empty() ? GetTypeName() : newName;

        if ( RequiresUniqueName() )
        {
            m_name = CreateUniqueNodeName( m_name );
        }
    }

    String BaseNode::CreateUniqueNodeName( String const& desiredName ) const
    {
        String uniqueName = desiredName;

        if ( HasParentGraph() )
        {
            uniqueName = GetParentGraph()->GetUniqueNameForRenameableNode( desiredName, this );
        }

        return uniqueName;
    }

    bool BaseNode::HasParentNode() const
    {
        if ( m_pParentGraph == nullptr )
        {
            return false;
        }

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

        if ( HasChildGraph() )
        {
            GetChildGraph()->RegenerateIDs( IDMapping );
        }

        if ( HasSecondaryGraph() )
        {
            GetSecondaryGraph()->RegenerateIDs( IDMapping );
        }

        return m_ID;
    }

    Color BaseNode::GetTitleBarColor() const
    {
        return Style::s_defaultTitleColor;
    }

    ImRect BaseNode::GetRect() const
    {
        ImVec2 const nodeMargin = GetNodeMargin();
        ImVec2 const rectMin = ImVec2( m_canvasPosition ) - nodeMargin;
        ImVec2 const rectMax = ImVec2( m_canvasPosition ) + nodeMargin + m_size;
        return ImRect( rectMin, rectMax );
    }

    void BaseNode::DrawInternalSeparatorColored( DrawContext const& ctx, Color color, float preMarginY, float postMarginY ) const
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

    void BaseNode::DrawInternalSeparator( DrawContext const& ctx, float unscaledPreMarginY, float unscaledPostMarginY ) const
    {
        DrawInternalSeparatorColored( ctx, Style::s_genericNodeSeparatorColor, unscaledPreMarginY, unscaledPostMarginY );
    }

    void BaseNode::BeginDrawInternalRegionColored( DrawContext const& ctx, Color color, float preMarginY, float postMarginY ) const
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

    void BaseNode::BeginDrawInternalRegion( DrawContext const& ctx, float unscaledPreMarginY, float unscaledPostMarginY ) const
    {
        BeginDrawInternalRegionColored( ctx, Style::s_genericNodeInternalRegionDefaultColor, unscaledPreMarginY, unscaledPostMarginY );
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

    BaseGraph* BaseNode::GetNavigationTarget()
    {
        return GetChildGraph();
    }

    //-------------------------------------------------------------------------

    TEvent<BaseGraph*> BaseGraph::s_onEndRootGraphModification;
    TEvent<BaseGraph*> BaseGraph::s_onBeginRootGraphModification;

    //-------------------------------------------------------------------------

    BaseGraph::BaseGraph( BaseNode* pParentNode )
        : m_ID( UUID::GenerateID() )
        , m_pParentNode( pParentNode )
    {
        EE_ASSERT( m_pParentNode != nullptr );
    }

    BaseGraph::~BaseGraph()
    {
        for ( TTypeInstance<BaseNode>& nodeInstance : m_nodes )
        {
            if ( !nodeInstance.IsSet() )
            {
                continue;
            }

            nodeInstance->m_pParentGraph = nullptr;
            nodeInstance->m_ID.Clear();
            nodeInstance.DestroyInstance();
        }

        m_nodes.clear();
        m_pParentNode = nullptr;
        m_ID.Clear();
    }

    void BaseGraph::PostDeserialize()
    {
        IReflectedType::PostDeserialize();

        // Set parent graph ptrs
        for ( TTypeInstance<BaseNode>& nodeInstance : m_nodes )
        {
            nodeInstance->m_pParentGraph = this;
        }
    }

    BaseGraph* BaseGraph::GetRootGraph()
    {
        auto pRootGraph = this;
        while ( pRootGraph != nullptr && pRootGraph->m_pParentNode != nullptr )
        {
            pRootGraph = pRootGraph->m_pParentNode->GetParentGraph();
        }

        return pRootGraph;
    }

    void BaseGraph::CreateNodeInternal( BaseNode* pCreatedNode, Float2 const& position )
    {
        pCreatedNode->m_ID = UUID::GenerateID();
        pCreatedNode->m_pParentGraph = this;
        pCreatedNode->m_canvasPosition = position;

        // Start an undo action, the initialize call is placed within the modification since some nodes initialization might mess with parent graph state
        BeginModification();
        {
            m_nodes.emplace_back( TTypeInstance( pCreatedNode ) );

            if ( pCreatedNode->IsRenameable() )
            {
                pCreatedNode->Rename( pCreatedNode->GetName() );
            }

            OnNodeAdded( pCreatedNode );
        }
        EndModification();
    }

    void BaseGraph::BeginModification()
    {
        auto pRootGraph = GetRootGraph();
        if ( pRootGraph == nullptr )
        {
            return;
        }

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
        if ( pRootGraph == nullptr )
        {
            return;
        }

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
        for ( TTypeInstance<BaseNode> const& nodeInstance : m_nodes )
        {
            BaseNode* pNode = const_cast<BaseNode*>( nodeInstance.Get() );

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
        for ( TTypeInstance<BaseNode> const& nodeInstance : m_nodes )
        {
            BaseNode* pNode = const_cast<BaseNode*>( nodeInstance.Get() );

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

    void BaseGraph::DestroyNode( UUID const& nodeID )
    {
        for ( auto iter = m_nodes.begin(); iter != m_nodes.end(); ++iter )
        {
            BaseNode* pNode = iter->Get();
            if ( pNode->GetID() == nodeID )
            {
                BeginModification();
                PreDestroyNode( pNode );

                // Clear node data
                pNode->m_pParentGraph = nullptr;
                pNode->m_ID.Clear();

                iter->DestroyInstance();
                m_nodes.erase( iter );

                PostDestroyNode( nodeID );
                EndModification();
                return;
            }
        }

        EE_UNREACHABLE_CODE();
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

        for ( auto const& pNode : path )
        {
            pathFromRoot.emplace_back( pNode->GetID() );
        }

        return pathFromRoot;
    }

    UUID BaseGraph::RegenerateIDs( THashMap<UUID, UUID>& IDMapping )
    {
        UUID const originalID = m_ID;
        m_ID = UUID::GenerateID();

        EE_ASSERT( IDMapping.find( originalID ) == IDMapping.end() );
        IDMapping.insert( TPair<UUID, UUID>( originalID, m_ID ) );

        for ( TTypeInstance<BaseNode>& nodeInstance : m_nodes )
        {
            nodeInstance->RegenerateIDs( IDMapping );
        }

        return m_ID;
    }

    void BaseGraph::OnShowGraph()
    {
        for ( TTypeInstance<BaseNode>& nodeInstance : m_nodes )
        {
            nodeInstance->OnShowNode();
        }
    }

    bool BaseGraph::IsUniqueNodeName( String const &name ) const
    {
        for ( TTypeInstance<BaseNode> const& nodeInstance : m_nodes )
        {
            BaseNode const* pNode = nodeInstance.Get();

            // Only check other renameable nodes
            if ( !pNode->IsRenameable() )
            {
                continue;
            }

            if ( pNode->GetName() == name )
            {
                return false;
            }
        }

        return true;
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
            for ( TTypeInstance<BaseNode> const& nodeInstance : m_nodes )
            {
                // Ignore specified node
                if ( nodeInstance.Get() == m_pNodeToIgnore )
                {
                    continue;
                }

                // Only check other renameable nodes
                if ( !nodeInstance->IsRenameable() )
                {
                    continue;
                }

                if ( nodeInstance->GetName() == uniqueName )
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

    BaseGraph* BaseGraph::GetNavigationTarget()
    {
        BaseNode* pParentNode = GetParentNode();
        if ( pParentNode != nullptr )
        {
            return pParentNode->GetParentGraph();
        }

        return nullptr;
    }
}