#include "ResourceEditor_Skeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "Base/Math/MathUtils.h"

//-------------------------------------------------------------------------

namespace EE::Animation
{
    EE_RESOURCE_EDITOR_FACTORY( SkeletonEditorFactory, Skeleton, SkeletonEditor );

    //-------------------------------------------------------------------------

    static bool IsValidWeight( float weight )
    {
        return ( weight >= 0.0f && weight <= 1.0f ) || weight == -1.0f;
    }

    //-------------------------------------------------------------------------

    SkeletonEditor::SkeletonEditor( ToolsContext const* pToolsContext, EntityWorld* pWorld, ResourceID const& resourceID )
        : TResourceEditor<Skeleton>( pToolsContext, pWorld, resourceID )
        , m_animationClipBrowser( pToolsContext )
    {}

    SkeletonEditor::~SkeletonEditor()
    {
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
    }

    void SkeletonEditor::InitializeDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        ImGuiID viewportDockID = 0;
        ImGuiID leftDockID = ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.3f, nullptr, &viewportDockID );
        ImGuiID rightDockID = ImGui::DockBuilderSplitNode( viewportDockID, ImGuiDir_Right, 0.2f, nullptr, &viewportDockID );
        ImGuiID bottomRightDockID = ImGui::DockBuilderSplitNode( rightDockID, ImGuiDir_Down, 0.33f, nullptr, &rightDockID );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Viewport" ).c_str(), viewportDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Skeleton" ).c_str(), leftDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Clip Browser" ).c_str(), rightDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Bone Masks" ).c_str(), rightDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Descriptor" ).c_str(), bottomRightDockID );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Bone Info" ).c_str(), bottomRightDockID );
    }

    //-------------------------------------------------------------------------

    void SkeletonEditor::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TResourceEditor<Skeleton>::Initialize( context );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Skeleton", [this] ( UpdateContext const& context, bool isFocused ) { DrawSkeletonWindow( context, isFocused ); } );
        CreateToolWindow( "Bone Info", [this] ( UpdateContext const& context, bool isFocused ) { DrawBoneInfoWindow( context, isFocused ); } );
        CreateToolWindow( "Bone Masks", [this] ( UpdateContext const& context, bool isFocused ) { DrawBoneMaskWindow( context, isFocused ); } );
        CreateToolWindow( "Clip Browser", [this] ( UpdateContext const& context, bool isFocused ) { DrawClipBrowser( context, isFocused ); } );
    }

    void SkeletonEditor::Shutdown( UpdateContext const& context )
    {
        DestroyPreviewEntity();
        DestroySkeletonTree();
        TResourceEditor<Skeleton>::Shutdown( context );
    }

    void SkeletonEditor::CreatePreviewEntity()
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );
        EE_ASSERT( m_pPreviewMeshComponent == nullptr );
        EE_ASSERT( m_editedResource.IsLoaded() );

        if ( m_editedResource->GetPreviewMeshID().IsValid() )
        {
            m_pPreviewMeshComponent = EE::New<Render::SkeletalMeshComponent>( StringID( "Mesh Component" ) );
            m_pPreviewMeshComponent->SetSkeleton( GetDescriptorID() );
            m_pPreviewMeshComponent->SetMesh( m_editedResource->GetPreviewMeshID() );

            m_pPreviewEntity = EE::New<Entity>( StringID( "Preview" ) );
            m_pPreviewEntity->AddComponent( m_pPreviewMeshComponent );
            AddEntityToWorld( m_pPreviewEntity );
        }
    }

    void SkeletonEditor::DestroyPreviewEntity()
    {
        if ( m_pPreviewEntity != nullptr )
        {
            DestroyEntityInWorld( m_pPreviewEntity );
            m_pPreviewEntity = nullptr;
            m_pPreviewMeshComponent = nullptr;
        }
    }

    void SkeletonEditor::PostUndoRedo( UndoStack::Operation operation, IUndoableAction const* pAction )
    {
        TResourceEditor<Skeleton>::PostUndoRedo( operation, pAction );

        if ( !IsResourceLoaded() )
        {
            if ( IsEditingBoneMask() )
            {
                StopEditingMask();
            }
        }
    }

    void SkeletonEditor::OnDescriptorLoadCompleted()
    {
        if ( IsDescriptorLoaded() )
        {
            m_animationClipBrowser.SetSkeleton( GetDescriptorID() );
        }
    }

    void SkeletonEditor::OnDescriptorUnload()
    {
        m_animationClipBrowser.SetSkeleton( ResourceID() );
    }

    void SkeletonEditor::OnResourceLoadCompleted( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource && IsResourceLoaded() )
        {
            CreatePreviewEntity();

            ValidateDescriptorBoneMaskDefinitions();
            ValidateLODSetup();
            CreateSkeletonTree();
        }
    }

    void SkeletonEditor::OnResourceUnload( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource )
        {
            DestroyPreviewEntity();
            DestroySkeletonTree();
        }
    }

    //-------------------------------------------------------------------------

    void SkeletonEditor::DrawViewportToolbar( UpdateContext const& context, Render::Viewport const* pViewport )
    {
        TResourceEditor<Skeleton>::DrawViewportToolbar( context, pViewport );

        if ( !IsResourceLoaded() )
        {
            return;
        }

        // Skeleton Info
        //-------------------------------------------------------------------------

        ImGui::NewLine();
        ImGui::Indent();

        auto PrintAnimDetails = [this] ( Color color )
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold, color );
            ImGui::Text( "Num Bones: %d", m_editedResource->GetNumBones() );
            ImGui::Text( "Num Bones For Low LOD: %d", m_editedResource->GetNumBones( Skeleton::LOD::Low ) );
        };

        ImVec2 const cursorPos = ImGui::GetCursorPos();
        ImGui::SetCursorPos( cursorPos + ImVec2( 0, 1 ) );
        ImGui::Indent( 1 );
        PrintAnimDetails( Colors::Black );
        ImGui::Unindent( 1 );
        ImGui::SetCursorPos( cursorPos );
        PrintAnimDetails( Colors::Yellow );

        ImGui::Unindent();
    }

    void SkeletonEditor::DrawMenu( UpdateContext const& context )
    {
        TResourceEditor<Skeleton>::DrawMenu( context );

        if ( ImGui::BeginMenu( EE_ICON_TUNE" Options" ) )
        {
            if ( ImGui::Checkbox( "Show Preview Mesh", &m_showPreviewMesh ) )
            {
                m_pPreviewMeshComponent->SetVisible( m_showPreviewMesh );
            }
            ImGui::EndMenu();
        }
    }

    void SkeletonEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        TResourceEditor::Update( context, isVisible, isFocused );

        // Debug drawing in Viewport
        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            if ( IsEditingBoneMask() )
            {
                DrawBoneMaskPreview();
            }
            else
            {
                DrawSkeletonPreview();
            }
        }
    }

    static int FilterMaskNameChars( ImGuiInputTextCallbackData* data )
    {
        if ( isalnum( data->EventChar ) || data->EventChar == '_' || data->EventChar == ' ' )
        {
            return 0;
        }
        return 1;
    }

    // Skeleton
    //-------------------------------------------------------------------------

    void SkeletonEditor::DrawSkeletonWindow( UpdateContext const& context, bool isFocused )
    {
        if ( IsResourceLoaded() )
        {
            EE_ASSERT( m_pSkeletonTreeRoot != nullptr );

            if ( IsEditingBoneMask() )
            {
                if ( ImGuiX::ColoredButton( Colors::OrangeRed, Colors::White, "Stop Editing Bone Mask", ImVec2( -1, 0 ) ) )
                {
                    StopEditingMask();
                }
            }

            static ImGuiTableFlags flags = ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody;
            if ( ImGui::BeginTable( "SkeletonTreeTable", 2, flags ) )
            {
                // The first column will use the default _WidthStretch when ScrollX is Off and _WidthFixed when ScrollX is On
                ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize );

                if ( IsEditingBoneMask() )
                {
                    ImGui::TableSetupColumn( "Weight", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 120.0f );
                }
                else
                {
                    ImGui::TableSetupColumn( "LOD", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 40.0f );
                }

                ImGui::TableHeadersRow();

                //-------------------------------------------------------------------------

                DrawBoneRow( m_pSkeletonTreeRoot );

                ImGui::EndTable();
            }
        }
    }

    void SkeletonEditor::CreateSkeletonTree()
    {
        TVector<BoneInfo*> boneInfos;

        // Create all infos
        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        int32_t const numBones = pSkeleton->GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& pBoneInfo = boneInfos.emplace_back( EE::New<BoneInfo>() );
            pBoneInfo->m_boneIdx = i;
        }

        // Create hierarchy
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = pSkeleton->GetParentBoneIndex( i );
            EE_ASSERT( parentBoneIdx != InvalidIndex );
            boneInfos[parentBoneIdx]->m_children.emplace_back( boneInfos[i] );
        }

        // Set root
        m_pSkeletonTreeRoot = boneInfos[0];
    }

    void SkeletonEditor::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }
    }

    void SkeletonEditor::DrawBoneRow( BoneInfo* pBone )
    {
        EE_ASSERT( pBone != nullptr );

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        int32_t const boneIdx = pBone->m_boneIdx;
        StringID const currentBoneID = pSkeleton->GetBoneID( boneIdx );
        bool const isSelected = m_selectedBoneID == currentBoneID;

        Color rowColor = ImGuiX::Style::s_colorText;

        if ( isSelected )
        {
            rowColor = ImGuiX::Style::s_colorAccent0;
        }
        else if ( IsEditingBoneMask() )
        {
            rowColor = BoneMask::GetColorForWeight( m_editedBoneWeights[boneIdx] );
        }

        //-------------------------------------------------------------------------

        ImGui::TableNextRow();

        //-------------------------------------------------------------------------
        // Draw Label
        //-------------------------------------------------------------------------

        bool const isHighLOD = GetBoneLOD( currentBoneID ) == Skeleton::LOD::High;
        InlineString const boneLabel( InlineString::CtorSprintf(), "%d. %s", boneIdx, currentBoneID.c_str() );

        ImGui::TableNextColumn();

        int32_t treeNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnDoubleClick;
        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        if ( !IsEditingBoneMask() && isSelected )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::SetNextItemOpen( pBone->m_isExpanded );
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor( ImGuiCol_Text, rowColor );
        pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );

        // Handle bone selection
        if ( ImGui::IsItemClicked() )
        {
            m_selectedBoneID = currentBoneID;
        }

        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------
        // Draw Context Menu
        //-------------------------------------------------------------------------

        InlineString const contextMenuID( InlineString::CtorSprintf(), "##%s_ctx", currentBoneID.c_str() );
        if ( ImGui::BeginPopupContextItem( contextMenuID.c_str() ) )
        {
            if ( ImGui::MenuItem( EE_ICON_IDENTIFIER" Copy Bone ID" ) )
            {
                ImGui::SetClipboardText( currentBoneID.c_str() );
            }

            //-------------------------------------------------------------------------

            if ( IsEditingBoneMask() )
            {
                ImGuiX::TextSeparator( "Propagate");

                if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY" Propagate Current Weight" ) )
                {
                    SetAllChildWeights( boneIdx, m_editedBoneWeights[boneIdx] );
                }

                //-------------------------------------------------------------------------

                ImGuiX::TextSeparator( "Fully Masked" );

                if ( boneIdx != 0 && ImGui::MenuItem( EE_ICON_NUMERIC_0_BOX_MULTIPLE_OUTLINE" Set bone and all children to 0" ) )
                {
                    SetAllChildWeights( boneIdx, 0.0f, true );
                }

                if ( ImGui::MenuItem( EE_ICON_NUMERIC_0_BOX_MULTIPLE_OUTLINE" Set all children to 0" ) )
                {
                    SetAllChildWeights( boneIdx, 0.0f );
                }

                //-------------------------------------------------------------------------

                ImGuiX::TextSeparator( "No Mask" );

                if ( boneIdx != 0 && ImGui::MenuItem( EE_ICON_NUMERIC_1_BOX_MULTIPLE_OUTLINE" Set bone and all children to 1" ) )
                {
                    SetAllChildWeights( boneIdx, 1.0f, true );
                }

                if ( ImGui::MenuItem( EE_ICON_NUMERIC_1_BOX_MULTIPLE_OUTLINE" Set all children to 1" ) )
                {
                    SetAllChildWeights( boneIdx, 1.0f );
                }

                //-------------------------------------------------------------------------

                ImGuiX::TextSeparator( "Clear" );

                if ( boneIdx != 0 && ImGui::MenuItem( EE_ICON_CLOSE_CIRCLE_MULTIPLE_OUTLINE" Unset bone and all children" ) )
                {
                    SetAllChildWeights( boneIdx, -1.0f, true );
                }

                if ( ImGui::MenuItem( EE_ICON_CLOSE_CIRCLE_MULTIPLE_OUTLINE" Unset all children" ) )
                {
                    SetAllChildWeights( boneIdx, -1.0f );
                }
            }
            else
            {
                if ( isHighLOD )
                {
                    if ( ImGui::MenuItem( EE_ICON_ALPHA_L" Clear High LOD Flag" ) )
                    {
                        SetBoneHierarchyLOD( currentBoneID, Skeleton::LOD::Low );
                    }
                }
                else
                {
                    if ( ImGui::MenuItem( EE_ICON_ALPHA_L" Set as High LOD Only" ) )
                    {
                        SetBoneHierarchyLOD( currentBoneID, Skeleton::LOD::High );
                    }
                }
            }

            ImGui::EndPopup();
        }

        //-------------------------------------------------------------------------
        // Draw Weight Editor
        //-------------------------------------------------------------------------

        if ( IsEditingBoneMask() )
        {
            ImGui::TableNextColumn();

            InlineString checkboxID;
            checkboxID.sprintf( "##%s_cb", currentBoneID.c_str() );

            bool isSet = m_editedBoneWeights[boneIdx] != -1.0f;
            if ( ImGui::Checkbox( checkboxID.c_str(), &isSet ) )
            {
                SetWeight( boneIdx, isSet ? 1.0f : -1.0f );
            }
            ImGuiX::ItemTooltip( "If you leave a weight unset, it will inherit the weight of the first parent with a set weight. \n\nIf you have unset weights between two set weights, we will interpolate the weights across the unset bones." );

            if ( isSet )
            {
                InlineString weightEditorID;
                weightEditorID.sprintf( "##%s", currentBoneID.c_str() );

                ImGui::SameLine();
                ImGui::SetNextItemWidth( -1 );
                ImGui::SliderFloat( weightEditorID.c_str(), &m_editedBoneWeights[boneIdx], 0.0f, 1.0f, "%.2f" );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    SetWeight( boneIdx, m_editedBoneWeights[boneIdx] );
                }
            }
            else
            {
                ImGui::SameLine();
                float const demoWeight = m_previewBoneMask.GetWeight( boneIdx );
                ImGui::PushStyleColor( ImGuiCol_Text, rowColor );
                ImGui::Text( "%.2f", demoWeight );
                ImGui::PopStyleColor();
            }
        }

        //-------------------------------------------------------------------------
        // Draw LOD
        //-------------------------------------------------------------------------

        if ( !IsEditingBoneMask() )
        {
            ImGui::TableNextColumn();

            if ( isHighLOD )
            {
                ImGui::TextColored( Colors::Lime.ToFloat4(), "High" );
            }
            else
            {
                ImGui::TextColored( Colors::Orange.ToFloat4(), "Low" );
            }
        }

        //-------------------------------------------------------------------------
        // Draw Children
        //-------------------------------------------------------------------------

        if ( pBone->m_isExpanded )
        {
            for ( BoneInfo* pChildBone : pBone->m_children )
            {
                DrawBoneRow( pChildBone );
            }
            ImGui::TreePop();
        }
    }

    void SkeletonEditor::DrawBoneInfoWindow( UpdateContext const& context, bool isFocused )
    {
        auto DrawTransform = [] ( Transform transform )
        {
            Vector const& translation = transform.GetTranslation();
            Quaternion const& rotation = transform.GetRotation();
            EulerAngles const angles = rotation.ToEulerAngles();
            Float4 const quatValues = rotation.ToFloat4();

            ImGui::Text( "Rot (Quat): X: %.3f, Y: %.3f, Z: %.3f, W: %.3f", quatValues.m_x, quatValues.m_y, quatValues.m_z, quatValues.m_w );
            ImGui::Text( "Rot (Euler): X: %.3f, Y: %.3f, Z: %.3f", angles.m_x.ToDegrees().ToFloat(), angles.m_y.ToDegrees().ToFloat(), angles.m_z.ToDegrees().ToFloat() );
            ImGui::Text( "Trans: X: %.3f, Y: %.3f, Z: %.3f", translation.GetX(), translation.GetY(), translation.GetZ() );
            ImGui::Text( "Scl: %.3f", transform.GetScale() );
        };

        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() && m_selectedBoneID.IsValid() )
        {
            Skeleton const* pSkeleton = m_editedResource.GetPtr();
            int32_t const selectedBoneIdx = pSkeleton->GetBoneIndex( m_selectedBoneID );
            EE_ASSERT( selectedBoneIdx != InvalidIndex );

            {
                ImGuiX::ScopedFont sf( ImGuiX::Font::LargeBold );
                ImGui::Text( "%d. %s", selectedBoneIdx, pSkeleton->GetBoneID( selectedBoneIdx ).c_str() );
            }

            ImGui::NewLine();
            ImGuiX::TextSeparator( "Local Transform" );
            Transform const& localBoneTransform = pSkeleton->GetLocalReferencePose()[selectedBoneIdx];
            DrawTransform( localBoneTransform );

            ImGui::NewLine();
            ImGuiX::TextSeparator( "Global Transform" );
            Transform const& globalBoneTransform = pSkeleton->GetGlobalReferencePose()[selectedBoneIdx];
            DrawTransform( globalBoneTransform );
        }
    }

    void SkeletonEditor::DrawSkeletonPreview()
    {
        EE_ASSERT( IsResourceLoaded() );

        auto drawingCtx = GetDrawingContext();

        // Draw skeleton
        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        drawingCtx.Draw( *pSkeleton, Transform::Identity );

        // Draw selected bone
        if ( m_selectedBoneID.IsValid() )
        {
            int32_t const boneIdx = pSkeleton->GetBoneIndex( m_selectedBoneID );
            if ( boneIdx != InvalidIndex )
            {
                Transform const globalBoneTransform = pSkeleton->GetBoneGlobalTransform( boneIdx );
                drawingCtx.DrawAxis( globalBoneTransform, 0.25f, 3.0f );

                Vector textLocation = globalBoneTransform.GetTranslation();
                Vector const textLineLocation = textLocation - Vector( 0, 0, 0.01f );
                drawingCtx.DrawTextBox3D( textLocation, m_selectedBoneID.c_str(), Colors::Lime );
            }
        }
    }

    // LOD
    //-------------------------------------------------------------------------

    void SkeletonEditor::ValidateLODSetup()
    {
        EE_ASSERT( IsDescriptorLoaded() && IsResourceLoaded() );

        auto& highLODBones = GetDescriptor<SkeletonResourceDescriptor>()->m_highLODBones;
        for ( int32_t i = 0; i < (int32_t) highLODBones.size(); i++ )
        {
            int32_t const highLODBoneIdx = m_editedResource->GetBoneIndex( highLODBones[i] );
            if ( highLODBoneIdx == InvalidIndex )
            {
                highLODBones.erase( highLODBones.begin() + i );
                i--;
            }
            else // Ensure all children of this bone are in the high LOD list
            {
                // Children are always listed after their parents
                for ( auto childBoneIdx = i + 1; childBoneIdx < m_editedResource->GetNumBones(); childBoneIdx++ )
                {
                    if ( m_editedResource->IsChildBoneOf( highLODBoneIdx, childBoneIdx ) )
                    {
                        StringID const childBoneID = m_editedResource->GetBoneID( childBoneIdx );
                        if ( !VectorContains( highLODBones, childBoneID ) )
                        {
                            highLODBones.emplace_back( childBoneID );
                        }
                    }
                }
            }
        }
    }

    Skeleton::LOD SkeletonEditor::GetBoneLOD( StringID boneID ) const
    {
        EE_ASSERT( IsDescriptorLoaded() );
        return VectorContains( GetDescriptor<SkeletonResourceDescriptor>()->m_highLODBones, boneID ) ? Skeleton::LOD::High : Skeleton::LOD::Low;
    }

    void SkeletonEditor::SetBoneHierarchyLOD( StringID boneID, Skeleton::LOD lod )
    {
        EE_ASSERT( IsDescriptorLoaded() );
        auto& highLODBones = GetDescriptor<SkeletonResourceDescriptor>()->m_highLODBones;

        ScopedDescriptorModification const sdm( this );

        int32_t const boneIdx = m_editedResource->GetBoneIndex( boneID );
        EE_ASSERT( boneIdx != InvalidIndex );

        if ( lod == Skeleton::LOD::High )
        {
            if ( !VectorContains( highLODBones, boneID ) )
            {
                highLODBones.emplace_back( boneID );
            }

            for ( auto j = boneIdx + 1; j < m_editedResource->GetNumBones(); j++ )
            {
                if ( m_editedResource->IsChildBoneOf( boneIdx, j ) )
                {
                    StringID const childBoneID = m_editedResource->GetBoneID( j );
                    if ( !VectorContains( highLODBones, childBoneID ) )
                    {
                        highLODBones.emplace_back( childBoneID );
                    }
                }
            }
        }
        else
        {
            highLODBones.erase_first( boneID );

            for ( auto j = boneIdx + 1; j < m_editedResource->GetNumBones(); j++ )
            {
                if ( m_editedResource->IsChildBoneOf( boneIdx, j ) )
                {
                    StringID const childBoneID = m_editedResource->GetBoneID( j );
                    highLODBones.erase_first( childBoneID );
                }
            }
        }
    }

    // Bone Masks
    //-------------------------------------------------------------------------

    void SkeletonEditor::StartEditingMask( StringID maskID )
    {
        EE_ASSERT( maskID.IsValid() );
        EE_ASSERT( m_editedMaskIdx == InvalidIndex );

        BoneMaskDefinition* pEditedBoneMaskDefinition = nullptr;
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
        {
            if ( pDescriptor->m_boneMaskDefinitions[i].m_ID == maskID )
            {
                pEditedBoneMaskDefinition = &pDescriptor->m_boneMaskDefinitions[i];
                m_editedMaskIdx = i;
                break;
            }
        }

        EE_ASSERT( pEditedBoneMaskDefinition != nullptr );

        // Reset edited weights
        //-------------------------------------------------------------------------

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        auto const numBones = pSkeleton->GetNumBones();
        m_editedBoneWeights.resize( numBones );
        for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            m_editedBoneWeights[boneIdx] = -1.0f;
        }

        // Transfer definition weights
        //-------------------------------------------------------------------------

        for ( auto const& boneWeight : pEditedBoneMaskDefinition->m_weights )
        {
            int32_t const boneIdx = pSkeleton->GetBoneIndex( boneWeight.m_boneID );
            EE_ASSERT( boneIdx != InvalidIndex );
            m_editedBoneWeights[boneIdx] = boneWeight.m_weight;
        }

        //-------------------------------------------------------------------------

        UpdatePreviewBoneMask();
    }

    void SkeletonEditor::StopEditingMask()
    {
        m_editedMaskIdx = InvalidIndex;
    }

    void SkeletonEditor::CreateBoneMask()
    {
        EE_ASSERT( IsDescriptorLoaded() );

        ScopedDescriptorModification const sdm( this );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        pDescriptor->m_boneMaskDefinitions.emplace_back( StringID( "Bone Mask" ) );
        GenerateUniqueBoneMaskName( (int32_t) pDescriptor->m_boneMaskDefinitions.size() - 1 );
    }

    void SkeletonEditor::DestroyBoneMask( StringID maskID )
    {
        EE_ASSERT( IsDescriptorLoaded() );

        if ( IsEditingBoneMask() )
        {
            StopEditingMask();
        }

        //-------------------------------------------------------------------------

        ScopedDescriptorModification const sdm( this );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        for ( auto iter = pDescriptor->m_boneMaskDefinitions.begin(); iter != pDescriptor->m_boneMaskDefinitions.end(); ++iter )
        {
            if ( iter->m_ID == maskID )
            {
                pDescriptor->m_boneMaskDefinitions.erase( iter );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void SkeletonEditor::RenameBoneMask( StringID oldID, StringID newID )
    {
        EE_ASSERT( IsDescriptorLoaded() );

        ScopedDescriptorModification const sdm( this );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
        {
            if ( pDescriptor->m_boneMaskDefinitions[i].m_ID == oldID )
            {
                pDescriptor->m_boneMaskDefinitions[i].m_ID = newID;
                GenerateUniqueBoneMaskName( i );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void SkeletonEditor::SetWeight( int32_t boneIdx, float weight )
    {
        EE_ASSERT( IsValidWeight( weight ) );
        EE_ASSERT( IsDescriptorLoaded() );
        EE_ASSERT( IsEditingBoneMask() );

        ScopedDescriptorModification const sm( this );
        m_editedBoneWeights[boneIdx] = weight;

        ReflectWeightsToEditedBoneMask();
        UpdatePreviewBoneMask();
    }

    void SkeletonEditor::SetAllChildWeights( int32_t parentBoneIdx, float weight, bool bIncludeParent )
    {
        EE_ASSERT( IsValidWeight( weight ) );
        EE_ASSERT( IsDescriptorLoaded() );
        EE_ASSERT( IsEditingBoneMask() );

        ScopedDescriptorModification const sm( this );

        if ( bIncludeParent )
        {
            m_editedBoneWeights[parentBoneIdx] = weight;
        }

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        auto const numBones = pSkeleton->GetNumBones();
        for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            if ( pSkeleton->IsChildBoneOf( parentBoneIdx, boneIdx ) )
            {
                m_editedBoneWeights[boneIdx] = weight;
            }
        }

        ReflectWeightsToEditedBoneMask();
        UpdatePreviewBoneMask();
    }

    void SkeletonEditor::ReflectWeightsToEditedBoneMask()
    {
        EE_ASSERT( IsDescriptorLoaded() );
        EE_ASSERT( IsEditingBoneMask() );

        ScopedDescriptorModification const sm( this );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        auto pEditedBoneMaskDefinition = &pDescriptor->m_boneMaskDefinitions[m_editedMaskIdx];

        // Clear existing weights
        pEditedBoneMaskDefinition->m_weights.clear();

        // Reflect the current edited weights
        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        auto const numBones = pSkeleton->GetNumBones();
        for ( auto boneIdx = 0; boneIdx < numBones; boneIdx++ )
        {
            if ( m_editedBoneWeights[boneIdx] >= 0.0f )
            {
                pEditedBoneMaskDefinition->m_weights.emplace_back( pSkeleton->GetBoneID( boneIdx ), m_editedBoneWeights[boneIdx] );
            }
        }
    }

    void SkeletonEditor::UpdatePreviewBoneMask()
    {
        EE_ASSERT( IsEditingBoneMask() );
        Skeleton const* pSkeleton = m_editedResource.GetPtr();

        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        auto pEditedBoneMaskDefinition = &pDescriptor->m_boneMaskDefinitions[m_editedMaskIdx];
        m_previewBoneMask = BoneMask( pSkeleton, *pEditedBoneMaskDefinition );
    }

    void SkeletonEditor::DrawBoneMaskPreview()
    {
        EE_ASSERT( IsResourceLoaded() );
        EE_ASSERT( IsEditingBoneMask() );
        EE_ASSERT( m_previewBoneMask.IsValid() );

        //-------------------------------------------------------------------------

        auto drawingCtx = GetDrawingContext();

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        auto const& localRefPose = pSkeleton->GetLocalReferencePose();
        auto const& parentIndices = pSkeleton->GetParentBoneIndices();

        auto const numBones = localRefPose.size();
        if ( numBones > 0 )
        {
            TInlineVector<Transform, 256> globalTransforms;
            globalTransforms.resize( numBones );

            globalTransforms[0] = localRefPose[0];
            for ( auto i = 1; i < numBones; i++ )
            {
                auto const& parentIdx = parentIndices[i];
                auto const& parentTransform = globalTransforms[parentIdx];
                globalTransforms[i] = localRefPose[i] * parentTransform;
            }

            //-------------------------------------------------------------------------

            for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
            {
                auto const& parentIdx = parentIndices[boneIdx];
                auto const& parentTransform = globalTransforms[parentIdx];
                auto const& boneTransform = globalTransforms[boneIdx];

                Color const boneColor = BoneMask::GetColorForWeight( m_previewBoneMask.GetWeight( boneIdx ) );
                drawingCtx.DrawLine( boneTransform.GetTranslation().ToFloat3(), parentTransform.GetTranslation().ToFloat3(), boneColor, 5.0f );
            }
        }
    }

    void SkeletonEditor::DrawBoneMaskWindow( UpdateContext const& context, bool isFocused )
    {
        StringID maskToDelete;

        //-------------------------------------------------------------------------

        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();

        ImGui::BeginDisabled( !IsDescriptorLoaded() );
        if ( ImGuiX::ColoredIconButton( Colors::Green, Colors::White, Colors::White, EE_ICON_PLUS, "Add New Mask", ImVec2( -1, 0 ) ) )
        {
            CreateBoneMask();
        }
        ImGui::EndDisabled();

        //-------------------------------------------------------------------------

        static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
        if ( ImGui::BeginTable( "BoneMasks", 2, flags, ImGui::GetContentRegionAvail() ) )
        {
            static ImVec2 const buttonSize( 30, 0 );

            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide );
            ImGui::TableSetupColumn( "Controls", ImGuiTableColumnFlags_WidthFixed, ( buttonSize.x * 3 + ( ImGui::GetStyle().ItemSpacing.x * 2 ) ) );

            //-------------------------------------------------------------------------
            if ( pDescriptor != nullptr )
            {
                for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
                {
                    auto const& boneMaskDefinition = pDescriptor->m_boneMaskDefinitions[i];
                    ImGui::PushID( &boneMaskDefinition );
                    ImGui::TableNextRow();

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();

                    ImGui::AlignTextToFramePadding();

                    bool const isSelected = ( m_editedMaskIdx != InvalidIndex ) ? ( m_editedMaskIdx == i ) : false;
                    ImGui::PushStyleColor( ImGuiCol_Text, isSelected ? Colors::Lime : ImGuiX::Style::s_colorText );
                    InlineString const label( InlineString::CtorSprintf(), EE_ICON_DRAMA_MASKS" %s", boneMaskDefinition.m_ID.c_str() );
                    if ( ImGui::Selectable( label.c_str(), isSelected ) )
                    {
                        if ( m_editedMaskIdx != i )
                        {
                            if ( IsEditingBoneMask() )
                            {
                                StopEditingMask();
                            }

                            StartEditingMask( boneMaskDefinition.m_ID );
                        }
                    }
                    ImGui::PopStyleColor();

                    //-------------------------------------------------------------------------

                    if ( ImGui::BeginPopupContextItem( "MaskContextMenu" ) )
                    {
                        if ( ImGui::MenuItem( EE_ICON_IDENTIFIER" Copy Bone Mask ID" ) )
                        {
                            ImGui::SetClipboardText( boneMaskDefinition.m_ID.c_str() );
                        }

                        if ( ImGui::MenuItem( EE_ICON_SQUARE_EDIT_OUTLINE" Rename" ) )
                        {
                            EE::Printf( m_renameBuffer, 255, boneMaskDefinition.m_ID.c_str() );
                            m_dialogMaskID = boneMaskDefinition.m_ID;
                            m_dialogManager.CreateModalDialog( "Rename Mask", [this] ( UpdateContext const& context ) { return DrawRenameBoneMaskDialog( context ); } );
                        }

                        if ( ImGui::MenuItem( EE_ICON_DELETE" Delete" ) )
                        {
                            m_dialogMaskID = boneMaskDefinition.m_ID;
                            m_dialogManager.CreateModalDialog( "Delete Mask?", [this] ( UpdateContext const& context ) { return DrawDeleteBoneMaskDialog( context ); } );
                        }

                        ImGui::EndPopup();
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();

                    if ( isSelected )
                    {
                        if ( ImGuiX::IconButton( EE_ICON_CHECK, "##StopEdit", Colors::Lime, buttonSize ) )
                        {
                            StopEditingMask();
                        }
                        ImGuiX::ItemTooltip( "Stop Editing Mask" );
                    }
                    else
                    {
                        ImGui::Dummy( buttonSize );
                    }

                    ImGui::SameLine();

                    if ( ImGui::Button( EE_ICON_SQUARE_EDIT_OUTLINE"##Rename", buttonSize ) )
                    {
                        EE::Printf( m_renameBuffer, 255, boneMaskDefinition.m_ID.c_str() );
                        m_dialogMaskID = boneMaskDefinition.m_ID;
                        m_dialogManager.CreateModalDialog( "Rename Mask", [this] ( UpdateContext const& context ) { return DrawRenameBoneMaskDialog( context ); } );
                    }
                    ImGuiX::ItemTooltip( "Rename" );

                    ImGui::SameLine();

                    if ( ImGui::Button( EE_ICON_DELETE"##Delete", buttonSize ) )
                    {
                        m_dialogMaskID = boneMaskDefinition.m_ID;
                        m_dialogManager.CreateModalDialog( "Delete Mask?", [this] ( UpdateContext const& context ) { return DrawDeleteBoneMaskDialog( context ); } );
                    }
                    ImGuiX::ItemTooltip( "Delete" );

                    //-------------------------------------------------------------------------

                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }
    }

    void SkeletonEditor::ValidateDescriptorBoneMaskDefinitions()
    {
        EE_ASSERT( IsDescriptorLoaded() );
        EE_ASSERT( IsResourceLoaded() );

        bool fixupPerformed = false;

        //-------------------------------------------------------------------------

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        for ( auto& descriptorDefinition : pDescriptor->m_boneMaskDefinitions )
        {
            // Remove invalid weights;
            for ( auto i = 0; i < descriptorDefinition.m_weights.size(); i++ )
            {
                int32_t const boneIdx = pSkeleton->GetBoneIndex( descriptorDefinition.m_weights[i].m_boneID );
                if ( boneIdx == InvalidIndex )
                {
                    descriptorDefinition.m_weights.erase( descriptorDefinition.m_weights.begin() + i );
                    i--;
                    fixupPerformed = true;
                    continue;
                }

                //-------------------------------------------------------------------------

                if ( descriptorDefinition.m_weights[i].m_weight < 0.0f || descriptorDefinition.m_weights[i].m_weight > 1.0f )
                {
                    descriptorDefinition.m_weights[i].m_weight = Math::Clamp( descriptorDefinition.m_weights[i].m_weight, 0.0f, 1.0f );
                    fixupPerformed = true;
                }
            }

            // Check for unique name
            for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
            {
                if ( &pDescriptor->m_boneMaskDefinitions[i] == &descriptorDefinition )
                {
                    continue;
                }

                // Duplicate name detected!
                if ( descriptorDefinition.m_ID == pDescriptor->m_boneMaskDefinitions[i].m_ID )
                {

                    fixupPerformed = true;
                }
            }
        }

        //-------------------------------------------------------------------------

        if ( fixupPerformed )
        {
            MarkDirty();
        }
    }

    void SkeletonEditor::GenerateUniqueBoneMaskName( int32_t boneMaskIdx )
    {
        // Do not used a scoped descriptor modification here as this function is used as part of the validation

        EE_ASSERT( IsDescriptorLoaded() );
        auto pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();
        EE_ASSERT( boneMaskIdx >= 0 && boneMaskIdx < pDescriptor->m_boneMaskDefinitions.size() );

        InlineString desiredName = pDescriptor->m_boneMaskDefinitions[boneMaskIdx].m_ID.c_str();
        InlineString finalName = desiredName;

        uint32_t counter = 0;
        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            isUniqueName = true;

            for ( auto i = 0; i < pDescriptor->m_boneMaskDefinitions.size(); i++ )
            {
                if ( boneMaskIdx == i )
                {
                    continue;
                }

                if ( finalName.comparei( pDescriptor->m_boneMaskDefinitions[i].m_ID.c_str()) == 0 )
                {
                    isUniqueName = false;
                    break;
                }
            }

            if ( !isUniqueName )
            {
                // Check if the last three characters are a numeric set, if so then increment the value and replace them
                if ( desiredName.length() > 3 && isdigit( desiredName[desiredName.length() - 1] ) && isdigit( desiredName[desiredName.length() - 2] ) && isdigit( desiredName[desiredName.length() - 3] ) )
                {
                    finalName.sprintf( "%s%03u", desiredName.substr( 0, desiredName.length() - 3 ).c_str(), counter );
                }
                else // Default name generation
                {
                    finalName.sprintf( "%s %03u", desiredName.c_str(), counter );
                }

                counter++;
            }
        }

        pDescriptor->m_boneMaskDefinitions[boneMaskIdx].m_ID = StringID( finalName.c_str() );
    }

    bool SkeletonEditor::DrawDeleteBoneMaskDialog( UpdateContext const& context )
    {
        ImGui::Text( "Are you sure you want to delete this bone mask?" );
        ImGui::NewLine();

        float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
        ImGui::SameLine( 0, dialogWidth - 64 );

        if ( ImGui::Button( "Yes", ImVec2( 30, 0 ) ) )
        {
            DestroyBoneMask( m_dialogMaskID );
            m_dialogMaskID.Clear();
        }

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "No", ImVec2( 30, 0 ) ) )
        {
            m_dialogMaskID.Clear();
        }

        return m_dialogMaskID.IsValid();
    }

    bool SkeletonEditor::DrawRenameBoneMaskDialog( UpdateContext const& context )
    {
        SkeletonResourceDescriptor* pDescriptor = GetDescriptor<SkeletonResourceDescriptor>();

        auto ValidateName = [this, pDescriptor] ( StringID ID )
        {
            // Invalid IDs are invalid :P
            if ( !ID.IsValid() )
            {
                return false;
            }

            // This is our original name
            if ( ID == m_dialogMaskID )
            {
                return true;
            }

            // Check for uniqueness
            for ( auto const& def : pDescriptor->m_boneMaskDefinitions )
            {
                if ( def.m_ID == ID )
                {
                    return false;
                }
            }

            return true;
        };

        bool renameRequested = false;
        bool isValidName = ValidateName( StringID( m_renameBuffer ) );

        ImGui::PushStyleColor( ImGuiCol_Text, isValidName ? ImGuiX::Style::s_colorText : Colors::Red );

        if ( ImGui::InputText( "##MaskName", m_renameBuffer, 255, ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCharFilter, FilterMaskNameChars ) )
        {
            isValidName = ValidateName( StringID( m_renameBuffer ) );
            if ( isValidName )
            {
                renameRequested = true;
            }
        }

        ImGui::PopStyleColor();

        //-------------------------------------------------------------------------

        ImGui::SetCursorPosY( ImGui::GetCursorPosY() + 4 );
        ImGui::NewLine();

        float const dialogWidth = ( ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() ).x;
        ImGui::SameLine( 0, dialogWidth - 104 );

        ImGui::BeginDisabled( !isValidName );
        if ( ImGui::Button( "Ok", ImVec2( 50, 0 ) ) || renameRequested )
        {
            RenameBoneMask( m_dialogMaskID, StringID( m_renameBuffer ) );
            m_dialogMaskID.Clear();
        }
        ImGui::EndDisabled();

        ImGui::SameLine( 0, 4 );

        if ( ImGui::Button( "Cancel", ImVec2( 50, 0 ) ) )
        {
            m_dialogMaskID.Clear();
        }

        return m_dialogMaskID.IsValid();
    }

    void SkeletonEditor::DrawClipBrowser( UpdateContext const& context, bool isFocused )
    {
        m_animationClipBrowser.Draw( context, isFocused );
    }
}