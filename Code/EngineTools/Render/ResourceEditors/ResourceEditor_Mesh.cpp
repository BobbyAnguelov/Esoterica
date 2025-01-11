#include "ResourceEditor_Mesh.h"
#include "EngineTools/Core/EditorTool.h"
#include "EngineTools/Render/ResourceDescriptors/ResourceDescriptor_RenderMesh.h"
#include "Engine/Render/Components/Component_RenderMesh.h"
#include "Engine/Render/Components/Component_StaticMesh.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------
// Shared
//-------------------------------------------------------------------------

namespace EE::Render
{
    void DrawGeometrySectionUI( MeshResourceDescriptor* pMeshDescriptor, MeshComponent* pMeshComponent, Mesh const* pMesh )
    {
        EE_ASSERT( pMeshDescriptor != nullptr && pMeshComponent != nullptr && pMesh != nullptr );

        int32_t const numSections = (int32_t) pMesh->GetNumSections();
        for ( int32_t i = 0; i < numSections; i++ )
        {
            ImGui::PushID( i );
            Mesh::GeometrySection const& section = pMesh->GetSection( i );
            ImGui::Text( "Name: %s", section.m_ID.c_str() );

            if ( pMeshComponent != nullptr )
            {
                bool isVisible = pMeshComponent->IsSectionVisible( i );
                if ( ImGui::Checkbox( "Is Visible", &isVisible) )
                {
                    pMeshComponent->SetSectionVisibility( i, isVisible );
                }
            }

            ImGui::Separator();
            ImGui::PopID();
        }
    }
}

//-------------------------------------------------------------------------
// Static Mesh Editor
//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_EDITOR_FACTORY( StaticMeshEditorFactory, StaticMesh, StaticMeshEditor );

    //-------------------------------------------------------------------------

    void StaticMeshEditor::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TResourceEditor<StaticMesh>::Initialize( context );

        //-------------------------------------------------------------------------

        m_pMeshComponent = EE::New<StaticMeshComponent>( StringID( "Static Mesh Component" ) );
        m_pMeshComponent->SetMesh( m_editedResource.GetResourceID() );

        // We dont own the entity as soon as we add it to the map
        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        m_pPreviewEntity->AddComponent( m_pMeshComponent );

        AddEntityToWorld( m_pPreviewEntity );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Info", [this] ( UpdateContext const& context, bool isFocused ) { DrawInfoWindow( context, isFocused ); } );
    }

    void StaticMeshEditor::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID topDockID = 0;
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Down, 0.5f, nullptr, &topDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_viewportWindowName ).c_str(), topDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Info" ).c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), bottomDockID );
    }

    void StaticMeshEditor::DrawMenu( UpdateContext const& context )
    {
        TResourceEditor<StaticMesh>::DrawMenu( context );

        ImGui::Separator();

        if ( ImGui::BeginMenu( EE_ICON_TUNE_VERTICAL"Options" ) )
        {
            ImGui::MenuItem( "Show Origin", nullptr, &m_showOrigin );
            ImGui::MenuItem( "Show Bounds", nullptr, &m_showBounds );
            ImGui::MenuItem( "Show Normals", nullptr, &m_showNormals );
            ImGui::MenuItem( "Show Vertices", nullptr, &m_showVertices );

            ImGui::EndMenu();
        }
    }

    void StaticMeshEditor::DrawHelpMenu() const
    {
        DrawHelpTextRow( "(Viewport) Reset Camera", "Backspace" );
        DrawHelpTextRow( "(Viewport) Focus Preview Entity", "F" );
    }

    void StaticMeshEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        TResourceEditor::Update( context, isVisible, isFocused );

        //-------------------------------------------------------------------------

        if ( m_isViewportFocused && m_pPreviewEntity != nullptr )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Backspace ) )
            {
                ResetCameraView();
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_F ) )
            {
                FocusCameraView( m_pPreviewEntity );
            }
        }

        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            auto drawingContext = GetDrawingContext();
            StaticMesh const* pMesh = m_editedResource.GetPtr();

            //-------------------------------------------------------------------------

            if ( m_showOrigin )
            {
                drawingContext.DrawAxis( Transform::Identity, 0.25f, 3.0f );
            }

            //-------------------------------------------------------------------------

            if ( m_showBounds )
            {
                drawingContext.DrawWireBox( pMesh->GetBounds(), Colors::Cyan );
            }

            //-------------------------------------------------------------------------

            if ( ( m_showVertices || m_showNormals ) )
            {
                auto pVertex = reinterpret_cast<StaticMeshVertex const*>( pMesh->GetVertexData().data() );
                for ( auto i = 0; i < pMesh->GetNumVertices(); i++ )
                {
                    if ( m_showVertices )
                    {
                        drawingContext.DrawPoint( pVertex->m_position, Colors::Cyan );
                    }

                    if ( m_showNormals )
                    {
                        drawingContext.DrawLine( pVertex->m_position, pVertex->m_position + ( pVertex->m_normal * 0.15f ), Colors::Yellow );
                    }

                    pVertex++;
                }
            }
        }
    }

    void StaticMeshEditor::DrawInfoWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsWaitingForResource() )
        {
            ImGui::Text( "Loading:" );
            ImGui::SameLine();
            ImGuiX::DrawSpinner( "Loading" );
            return;
        }

        if ( HasLoadingFailed() )
        {
            ImGui::Text( "Loading Failed: %s", m_editedResource.GetResourceID().c_str() );
            return;
        }

        //-------------------------------------------------------------------------

        auto pMesh = m_editedResource.GetPtr();
        EE_ASSERT( m_editedResource.IsLoaded() );
        EE_ASSERT( pMesh != nullptr );

        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
            ImGui::SeparatorText( "Mesh Data" );
        }

        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 2 ) );
        if ( ImGui::BeginTable( "MeshInfoTable", 2, ImGuiTableFlags_Borders ) )
        {
            ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthFixed, 100 );
            ImGui::TableSetupColumn( "Data", ImGuiTableColumnFlags_NoHide );

            //-------------------------------------------------------------------------

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text( "Data Path" );

            ImGui::TableNextColumn();
            ImGui::Text( pMesh->GetResourceID().c_str() );

            //-------------------------------------------------------------------------

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text( "Num Vertices" );

            ImGui::TableNextColumn();
            ImGui::Text( "%d", pMesh->GetNumVertices() );

            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::Text( "Num Indices" );

            ImGui::TableNextColumn();
            ImGui::Text( "%d", pMesh->GetNumIndices() );

            ImGui::EndTable();
        }
        ImGui::PopStyleVar();

        //-------------------------------------------------------------------------
        // Sections
        //-------------------------------------------------------------------------

        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::Large );
            ImGui::SeparatorText( "Geometry Sections" );
        }

        DrawGeometrySectionUI( GetDataFile<MeshResourceDescriptor>(), m_pMeshComponent, pMesh );
    }

    void StaticMeshEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource && IsResourceLoaded() )
        {
            auto pMeshDescriptor = GetDataFile<StaticMeshResourceDescriptor>();
            if ( pMeshDescriptor->m_materials.size() != m_editedResource->GetNumSections() )
            {
                BeginDataFileModification();
                pMeshDescriptor->m_materials.resize( m_editedResource->GetNumSections() );
                EndDataFileModification();
            }
        }
    }
}

//-------------------------------------------------------------------------
// Skeletal Mesh Editor
//-------------------------------------------------------------------------

namespace EE::Render
{
    EE_RESOURCE_EDITOR_FACTORY( SkeletalMeshEditorFactory, SkeletalMesh, SkeletalMeshEditor );

    //-------------------------------------------------------------------------

    SkeletalMeshEditor::~SkeletalMeshEditor()
    {
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
    }

    void SkeletalMeshEditor::Initialize( UpdateContext const& context )
    {
        TResourceEditor<SkeletalMesh>::Initialize( context );

        //-------------------------------------------------------------------------

        m_pMeshComponent = EE::New<SkeletalMeshComponent>( StringID( "Skeletal Mesh Component" ) );
        m_pMeshComponent->SetMesh( m_editedResource.GetResourceID() );

        // We dont own the entity as soon as we add it to the map
        m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
        m_pPreviewEntity->AddComponent( m_pMeshComponent );
        AddEntityToWorld( m_pPreviewEntity );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Skeleton", [this] ( UpdateContext const& context, bool isFocused ) { DrawSkeletonTreeWindow( context, isFocused ); } );
        CreateToolWindow( "Info", [this] ( UpdateContext const& context, bool isFocused ) { DrawInfoWindow( context, isFocused ); } );
    }

    void SkeletalMeshEditor::Shutdown( UpdateContext const& context )
    {
        DestroySkeletonTree();
        TResourceEditor<SkeletalMesh>::Shutdown( context );
    }

    void SkeletalMeshEditor::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID viewportDockID = 0;
        ImGuiID leftTopDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &viewportDockID );
        ImGuiID bottomDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Down, 0.2f, nullptr, &viewportDockID );
        ImGuiID leftBottomDockID = ImGui::DockBuilderSplitNode( leftTopDockID, ImGuiDir_Down, 0.2f, nullptr, &leftTopDockID );

        // Dock windows
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_viewportWindowName ).c_str(), viewportDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Info" ).c_str(), bottomDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Skeleton" ).c_str(), leftTopDockID );
    }

    void SkeletalMeshEditor::DrawMenu( UpdateContext const& context )
    {
        TResourceEditor<SkeletalMesh>::DrawMenu( context );

        //-------------------------------------------------------------------------

        ImGui::Separator();

        if ( ImGui::BeginMenu( EE_ICON_TUNE_VERTICAL"Options" ) )
        {
            ImGui::MenuItem( "Show Origin", nullptr, &m_showOrigin );
            ImGui::MenuItem( "Show Bounds", nullptr, &m_showBounds );
            ImGui::MenuItem( "Show Normals", nullptr, &m_showNormals );
            ImGui::MenuItem( "Show Vertices", nullptr, &m_showVertices );
            ImGui::MenuItem( "Show Bind Pose", nullptr, &m_showBindPose );

            ImGui::EndMenu();
        }

        if ( !IsResourceLoaded() )
        {
            return;
        }
    }

    void SkeletalMeshEditor::DrawHelpMenu() const
    {
        DrawHelpTextRow( "(Viewport) Reset Camera", "Backspace" );
        DrawHelpTextRow( "(Viewport) Focus Preview Entity", "F" );
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource && IsResourceLoaded() )
        {
            auto pMeshDescriptor = GetDataFile<SkeletalMeshResourceDescriptor>();
            if ( pMeshDescriptor->m_materials.size() != m_editedResource->GetNumSections() )
            {
                BeginDataFileModification();
                pMeshDescriptor->m_materials.resize( m_editedResource->GetNumSections() );
                EndDataFileModification();
            }
        }
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        TResourceEditor::Update( context, isVisible, isFocused );

        //-------------------------------------------------------------------------

        if ( m_isViewportFocused && m_pPreviewEntity != nullptr )
        {
            if ( ImGui::IsKeyPressed( ImGuiKey_Backspace ) )
            {
                ResetCameraView();
            }

            if ( ImGui::IsKeyPressed( ImGuiKey_F ) )
            {
                FocusCameraView( m_pPreviewEntity );
            }
        }

        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            auto drawingCtx = GetDrawingContext();
            SkeletalMesh const* pMesh = m_editedResource.GetPtr();

            //-------------------------------------------------------------------------

            if ( m_showOrigin )
            {
                drawingCtx.DrawAxis( Transform::Identity, 0.25f, 3.0f );
            }

            //-------------------------------------------------------------------------

            if ( m_showBounds )
            {
                drawingCtx.DrawWireBox( pMesh->GetBounds(), Colors::Cyan );
            }

            //-------------------------------------------------------------------------

            if ( ( m_showVertices || m_showNormals ) )
            {
                auto pVertex = reinterpret_cast<StaticMeshVertex const*>( pMesh->GetVertexData().data() );
                for ( auto i = 0; i < pMesh->GetNumVertices(); i++ )
                {
                    if ( m_showVertices )
                    {
                        drawingCtx.DrawPoint( pVertex->m_position, Colors::Cyan );
                    }

                    if ( m_showNormals )
                    {
                        drawingCtx.DrawLine( pVertex->m_position, pVertex->m_position + ( pVertex->m_normal * 0.15f ), Colors::Yellow );
                    }

                    pVertex++;
                }
            }

            //-------------------------------------------------------------------------

            if ( m_showBindPose )
            {
                m_editedResource->DrawBindPose( drawingCtx, Transform::Identity );

                if ( m_pMeshComponent != nullptr && m_pMeshComponent->IsInitialized() )
                {
                    m_pMeshComponent->DrawPose( drawingCtx );
                }
            }

            //-------------------------------------------------------------------------

            if ( m_selectedBoneID.IsValid() )
            {
                int32_t const selectedBoneIdx = m_editedResource->GetBoneIndex( m_selectedBoneID );
                if ( selectedBoneIdx != InvalidIndex )
                {
                    Transform const& globalBoneTransform = m_editedResource->GetBindPose()[selectedBoneIdx];
                    drawingCtx.DrawAxis( globalBoneTransform, 0.25f, 3.0f );

                    Vector textLocation = globalBoneTransform.GetTranslation();
                    Vector const textLineLocation = textLocation - Vector( 0, 0, 0.01f );
                    drawingCtx.DrawText3D( textLocation, m_selectedBoneID.c_str(), Colors::Yellow );
                }
            }
        }
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshEditor::DrawInfoWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsWaitingForResource() )
        {
            ImGui::Text( "Loading:" );
            ImGui::SameLine();
            ImGuiX::DrawSpinner( "Loading" );
        }
        else if ( HasLoadingFailed() )
        {
            ImGui::Text( "Loading Failed: %s", m_editedResource.GetResourceID().c_str() );
        }
        else // Draw UI
        {
            auto pSkeletalMesh = m_editedResource.GetPtr();
            EE_ASSERT( pSkeletalMesh != nullptr );

            //-------------------------------------------------------------------------
            // Draw Mesh Data
            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Mesh Data" );

            ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 4, 2 ) );
            if ( ImGui::BeginTable( "MeshInfoTable", 2, ImGuiTableFlags_Borders ) )
            {
                ImGui::TableSetupColumn( "Label", ImGuiTableColumnFlags_WidthFixed, 100 );
                ImGui::TableSetupColumn( "Data", ImGuiTableColumnFlags_NoHide );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "Data Path" );

                ImGui::TableNextColumn();
                ImGui::Text( pSkeletalMesh->GetResourceID().c_str() );

                //-------------------------------------------------------------------------

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "Num Bones" );

                ImGui::TableNextColumn();
                ImGui::Text( "%d", pSkeletalMesh->GetNumBones() );

                ImGui::TableNextColumn();
                ImGui::Text( "Num Vertices" );

                ImGui::TableNextColumn();
                ImGui::Text( "%d", pSkeletalMesh->GetNumVertices() );

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "Num Indices" );

                ImGui::TableNextColumn();
                ImGui::Text( "%d", pSkeletalMesh->GetNumIndices() );

                ImGui::EndTable();
            }
            ImGui::PopStyleVar();

            //-------------------------------------------------------------------------
            // Sections
            //-------------------------------------------------------------------------

            ImGui::SeparatorText( "Geometry Sections" );

            DrawGeometrySectionUI( GetDataFile<MeshResourceDescriptor>(), m_pMeshComponent, pSkeletalMesh );
        }
    }

    void SkeletalMeshEditor::DrawSkeletonTreeWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsResourceLoaded() )
        {
            ImVec2 const availableRegion = ImGui::GetContentRegionAvail();

            // Skeleton View
            //-------------------------------------------------------------------------

            if ( m_pSkeletonTreeRoot == nullptr )
            {
                CreateSkeletonTree();
            }

            EE_ASSERT( m_pSkeletonTreeRoot != nullptr );
            if ( ImGui::BeginChild( "Tree", ImVec2( -1, m_skeletonViewPanelProportionalHeight * availableRegion.y ) ) )
            {
                RenderSkeletonTree( m_pSkeletonTreeRoot );
            }
            ImGui::EndChild();

            // Splitter
            //-------------------------------------------------------------------------

            ImGui::PushStyleVar( ImGuiStyleVar_FrameRounding, 0.0f );
            ImGui::Button( "##GraphViewSplitter", ImVec2( -1, 5 ) );
            ImGui::PopStyleVar();

            if ( ImGui::IsItemHovered() )
            {
                ImGui::SetMouseCursor( ImGuiMouseCursor_ResizeNS );
            }

            if ( ImGui::IsItemActive() )
            {
                m_skeletonViewPanelProportionalHeight += ( ImGui::GetIO().MouseDelta.y / availableRegion.y );
                m_skeletonViewPanelProportionalHeight = Math::Max( 0.1f, m_skeletonViewPanelProportionalHeight );
            }

            // Details
            //-------------------------------------------------------------------------

            if ( ImGui::BeginChild( "Details", ImVec2( -1, ( 1.0f - m_skeletonViewPanelProportionalHeight ) * availableRegion.y ) ) )
            {
                auto DrawTransform = [] ( Transform transform )
                {
                    Vector const& translation = transform.GetTranslation();
                    Quaternion const& rotation = transform.GetRotation();
                    EulerAngles const angles = rotation.ToEulerAngles();
                    Float4 const quatValues = rotation.ToFloat4();

                    ImGuiX::ScopedFont const sf( ImGuiX::Font::Tiny );
                    ImGui::Text( "Rot (Quat): X: %.3f, Y: %.3f, Z: %.3f, W: %.3f", quatValues.m_x, quatValues.m_y, quatValues.m_z, quatValues.m_w );
                    ImGui::Text( "Rot (Euler): X: %.3f, Y: %.3f, Z: %.3f", angles.m_x.ToDegrees().ToFloat(), angles.m_y.ToDegrees().ToFloat(), angles.m_z.ToDegrees().ToFloat() );
                    ImGui::Text( "Trans: X: %.3f, Y: %.3f, Z: %.3f", translation.GetX(), translation.GetY(), translation.GetZ() );
                    ImGui::Text( "Scl: %.3f", transform.GetScale() );
                };

                //-------------------------------------------------------------------------

                if ( m_selectedBoneID.IsValid() )
                {
                    int32_t const selectedBoneIdx = m_editedResource->GetBoneIndex( m_selectedBoneID );
                    if ( selectedBoneIdx != InvalidIndex )
                    {
                        {
                            ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                            ImGui::Text( "%d. %s", selectedBoneIdx, m_editedResource->GetBoneID( selectedBoneIdx ).c_str() );
                        }

                        int32_t const parentBoneIdx = m_editedResource->GetParentBoneIndex( selectedBoneIdx );
                        if ( parentBoneIdx != InvalidIndex )
                        {
                            ImGui::NewLine();
                            ImGui::Text( "Local Transform" );
                            Transform const& localBoneTransform = Transform::Delta( m_editedResource->GetBindPose()[parentBoneIdx], m_editedResource->GetBindPose()[selectedBoneIdx] );
                            DrawTransform( localBoneTransform );
                        }

                        ImGui::NewLine();
                        ImGui::Text( "Global Transform" );
                        Transform const& globalBoneTransform = m_editedResource->GetBindPose()[selectedBoneIdx];
                        DrawTransform( globalBoneTransform );
                    }
                }
            }
            ImGui::EndChild();
        }
    }

    //-------------------------------------------------------------------------

    void SkeletalMeshEditor::CreateSkeletonTree()
    {
        TVector<BoneInfo*> boneInfos;

        // Create all infos
        int32_t const numBones = m_editedResource->GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& pBoneInfo = boneInfos.emplace_back( EE::New<BoneInfo>() );
            pBoneInfo->m_boneIdx = i;
        }

        // Create hierarchy
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = m_editedResource->GetParentBoneIndex( i );
            EE_ASSERT( parentBoneIdx != InvalidIndex );
            boneInfos[parentBoneIdx]->m_children.emplace_back( boneInfos[i] );
        }

        // Set root
        m_pSkeletonTreeRoot = boneInfos[0];
    }

    void SkeletalMeshEditor::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    ImRect SkeletalMeshEditor::RenderSkeletonTree( BoneInfo* pBone )
    {
        StringID const currentBoneID = m_editedResource->GetBoneID( pBone->m_boneIdx );
        ImGui::SetNextItemOpen( pBone->m_isExpanded );

        int32_t treeNodeFlags = ImGuiTreeNodeFlags_OpenOnDoubleClick;

        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        Color rowColor = ImGuiX::Style::s_colorText;
        if ( currentBoneID == m_selectedBoneID )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
            rowColor = ImGuiX::Style::s_colorAccent0;
        }

        InlineString boneLabel;
        boneLabel.sprintf( "%d. %s", pBone->m_boneIdx, m_editedResource->GetBoneID( pBone->m_boneIdx ).c_str() );
        ImGui::PushStyleColor( ImGuiCol_Text, rowColor );
        pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        ImGui::PopStyleColor();

        // Handle bone selection
        if ( ImGui::IsItemClicked() )
        {
            m_selectedBoneID = m_editedResource->GetBoneID( pBone->m_boneIdx );
        }

        //-------------------------------------------------------------------------

        InlineString const contextMenuID( InlineString::CtorSprintf(), "##%s_ctx", currentBoneID.c_str() );
        if ( ImGui::BeginPopupContextItem( contextMenuID.c_str() ) )
        {
            if ( ImGui::MenuItem( EE_ICON_IDENTIFIER" Copy Bone ID" ) )
            {
                ImGui::SetClipboardText( currentBoneID.c_str() );
            }

            ImGui::EndPopup();
        }

        //-------------------------------------------------------------------------

        ImRect const nodeRect = ImRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

        if ( pBone->m_isExpanded )
        {
            Color const treeLineColor = ImGui::GetColorU32( ImGuiCol_TextDisabled );
            float const smallOffsetX = 2;
            ImDrawList* drawList = ImGui::GetWindowDrawList();

            ImVec2 verticalLineStart = ImGui::GetCursorScreenPos();
            verticalLineStart.x += smallOffsetX;
            ImVec2 verticalLineEnd = verticalLineStart;

            for ( BoneInfo* pChild : pBone->m_children )
            {
                const float HorizontalTreeLineSize = 4.0f;
                const ImRect childRect = RenderSkeletonTree( pChild );
                const float midpoint = ( childRect.Min.y + childRect.Max.y ) / 2.0f;
                drawList->AddLine( ImVec2( verticalLineStart.x, midpoint ), ImVec2( verticalLineStart.x + HorizontalTreeLineSize, midpoint ), treeLineColor );
                verticalLineEnd.y = midpoint;
            }

            drawList->AddLine( verticalLineStart, verticalLineEnd, treeLineColor );
            ImGui::TreePop();
        }

        return nodeRect;
    }
}