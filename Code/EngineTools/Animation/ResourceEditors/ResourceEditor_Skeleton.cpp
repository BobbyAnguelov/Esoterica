#include "ResourceEditor_Skeleton.h"
#include "EngineTools/Animation/ResourceDescriptors/ResourceDescriptor_AnimationSkeleton.h"
#include "Engine/Render/Components/Component_SkeletalMesh.h"
#include "Engine/Animation/AnimationPose.h"
#include "Engine/Entity/EntityWorld.h"
#include "Engine/UpdateContext.h"
#include "Base/Math/MathUtils.h"
#include "Base/Imgui/ImguiStyle.h"
#include "EngineTools/Core/DialogManager.h"

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

    SkeletonEditor::SkeletonEditor( ToolsContext const* pToolsContext, ResourceID const& resourceID, EntityWorld* pWorld )
        : TResourceEditor<Skeleton>( pToolsContext, resourceID, pWorld )
        , m_animationClipBrowser( pToolsContext )
    {}

    SkeletonEditor::~SkeletonEditor()
    {
        EE_ASSERT( m_pSkeletonTreeRoot == nullptr );
    }

    void SkeletonEditor::SetupDockingLayout( ImGuiID dockspaceID, ImVec2 const& dockspaceSize ) const
    {
        // Split
        //-------------------------------------------------------------------------

        ImGuiID leftDockID0 = 0;
        ImGuiID leftDockID1 = 0;
        ImGuiID leftDockID2 = 0;
        ImGuiID centerDockID = 0;
        ImGuiID rightDockID0 = 0;
        ImGuiID rightDockID1 = 0;

        ImGui::DockBuilderSplitNode( dockspaceID, ImGuiDir_Left, 0.25f, &leftDockID0, &centerDockID );
        ImGui::DockBuilderSplitNode( centerDockID, ImGuiDir_Right, 0.33f, &rightDockID0, &centerDockID );

        ImGui::DockBuilderSplitNode( leftDockID0, ImGuiDir_Up, 0.20f, &leftDockID0, &leftDockID1 );
        ImGui::DockBuilderSplitNode( leftDockID1, ImGuiDir_Up, 0.75f, &leftDockID1, &leftDockID2 );

        ImGui::DockBuilderSplitNode( rightDockID0, ImGuiDir_Up, 0.5f, &rightDockID0, &rightDockID1 );

        // Dock windows
        //-------------------------------------------------------------------------

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Bone Masks" ).c_str(), leftDockID0 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Skeleton Outline" ).c_str(), leftDockID1 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( "Bone Info" ).c_str(), leftDockID2 );

        ImGui::DockBuilderDockWindow( GetToolWindowName( "Clip Browser" ).c_str(), rightDockID0 );
        ImGui::DockBuilderDockWindow( GetToolWindowName( s_dataFileWindowName ).c_str(), rightDockID1 );
    }

    //-------------------------------------------------------------------------

    void SkeletonEditor::Initialize( UpdateContext const& context )
    {
        EE_ASSERT( m_pPreviewEntity == nullptr );

        TResourceEditor<Skeleton>::Initialize( context );

        //-------------------------------------------------------------------------

        CreateToolWindow( "Skeleton Outline", [this] ( UpdateContext const& context, bool isFocused ) { DrawSkeletonOutlinerWindow( context, isFocused ); } );
        CreateToolWindow( "Bone Masks", [this] ( UpdateContext const& context, bool isFocused ) { DrawBoneMasksWindow( context, isFocused ); } );
        CreateToolWindow( "Bone Info", [this] ( UpdateContext const& context, bool isFocused ) { DrawBoneInfoWindow( context, isFocused ); } );
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
            m_pPreviewMeshComponent->SetSkeleton( GetDataFilePath() );
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
        else
        {
            // Update weights
            if ( IsEditingBoneMask() )
            {
                ReflectEditedWeights();
            }
        }
    }

    void SkeletonEditor::OnDataFileLoadCompleted()
    {
        if ( IsDataFileLoaded() )
        {
            m_animationClipBrowser.SetSkeleton( GetDataFilePath() );
        }
    }

    void SkeletonEditor::OnDataFileUnload()
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

            Skeleton const* pSkeleton = m_editedResource.GetPtr();
            m_previewBoneMask = BoneMask( pSkeleton, 1.0f );

            m_selectedBoneIDs.clear();
        }
    }

    void SkeletonEditor::OnResourceUnload( Resource::ResourcePtr* pResourcePtr )
    {
        if ( pResourcePtr == &m_editedResource )
        {
            m_selectedBoneIDs.clear();
            m_previewBoneMask = BoneMask();
            DestroyPreviewEntity();
            DestroySkeletonTree();
        }
    }

    //-------------------------------------------------------------------------

    void SkeletonEditor::ExtendViewportToolBar( UpdateContext const& context, Viewport* pViewport )
    {
        // Skeleton Info
        //-------------------------------------------------------------------------

        if ( !IsResourceLoaded() )
        {
            return;
        }

        ImGui::NewLine();

        auto PrintAnimDetails = [this] ( Color color )
        {
            ImGuiX::ScopedFont const sf( ImGuiX::Font::SmallBold, color );
            ImGui::Text( "Num Bones: %d", m_editedResource->GetNumBones() );
            ImGui::Text( "Num Bones For Low LOD: %d", m_editedResource->GetNumBones( Skeleton::LOD::Low ) );
        };

        ImGuiX::DrawShadowedText( Colors::Yellow, PrintAnimDetails );
    }

    bool SkeletonEditor::ExtendViewportToolBar_VisualizationControls( UpdateContext const& context, Viewport* pViewport )
    {
        if ( ImGui::Checkbox( "Show Preview Mesh", &m_showPreviewMesh ) )
        {
            if ( m_pPreviewMeshComponent != nullptr )
            {
                m_pPreviewMeshComponent->SetVisible( m_showPreviewMesh );
            }
        }

        return true;
    }

    void SkeletonEditor::Update( UpdateContext const& context, bool isVisible, bool isFocused )
    {
        TResourceEditor::Update( context, isVisible, isFocused );

        // Debug drawing in Viewport
        //-------------------------------------------------------------------------

        if ( IsResourceLoaded() )
        {
            Skeleton const* pSkeleton = m_editedResource.GetPtr();

            // Update preview bone mask
            //-------------------------------------------------------------------------

            if ( IsEditingBoneMask() )
            {
                auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();
                BoneMaskSetDefinition const& editedBoneMaskSet = pDescriptor->m_boneMaskSetDefinitions[m_editedMaskIdx];
                if ( editedBoneMaskSet.m_primaryWeightList.IsValid() )
                {
                    m_previewBoneMask = BoneMask( pSkeleton, editedBoneMaskSet.m_primaryWeightList );
                }
            }
            else
            {
                if ( !m_previewBoneMask.IsFullWeightMask() )
                {
                    m_previewBoneMask = BoneMask( pSkeleton, 1.0f );
                }
            }

            // Draw skeletons
            //-------------------------------------------------------------------------

            auto drawingCtx = GetDebugDrawContext();

            auto DrawSkeleton = [this, pSkeleton, &drawingCtx] ( Transform const& worldTransform, Skeleton::LOD lod, Color color, Color secondaryColor )
            {
                DrawOptions options;
                options.m_boneColor = color;
                options.m_pBoneWeights = IsEditingBoneMask() ? &m_previewBoneMask.GetWeights() : nullptr;
                pSkeleton->DrawDebug( drawingCtx, worldTransform, lod, options );

                // Draw secondary skeletons
                for ( auto const& secondarySkeleton : pSkeleton->GetPotentialSecondarySkeletons() )
                {
                    Transform secondarySkeletonAttachmentTransform = Transform::Identity;

                    int32_t const boneIdx = pSkeleton->GetBoneIndex( secondarySkeleton.m_attachmentBoneID );
                    if ( boneIdx != InvalidIndex )
                    {
                        secondarySkeletonAttachmentTransform = pSkeleton->GetBoneModelSpaceTransform( boneIdx ) * worldTransform;
                    }

                    options.m_boneColor = secondaryColor;
                    Skeleton const* pSecondarySkeleton = secondarySkeleton.m_skeleton.GetPtr<Skeleton>();
                    pSecondarySkeleton->DrawDebug( drawingCtx, secondarySkeletonAttachmentTransform, lod, options );
                }

                // Draw selected bones
                if ( !m_selectedBoneIDs.empty() )
                {
                    for ( StringID const& boneID : m_selectedBoneIDs )
                    {
                        int32_t const selectedBoneIdx = pSkeleton->GetBoneIndex( boneID );
                        EE_ASSERT( selectedBoneIdx != InvalidIndex );

                        Transform const globalBoneTransform = pSkeleton->GetBoneModelSpaceTransform( selectedBoneIdx ) * worldTransform;
                        drawingCtx.DrawAxis( globalBoneTransform, 0.1f, 3.0f );

                        Vector textLocation = globalBoneTransform.GetTranslation();
                        Vector const textLineLocation = textLocation - Vector( 0, 0, 0.01f );
                        drawingCtx.DrawTextBox3D( textLocation, boneID.c_str(), Colors::Lime, DebugFont::Small );
                    }
                }
            };

            bool const bHasLOD = pSkeleton->GetNumBones( Skeleton::LOD::Low ) != pSkeleton->GetNumBones();

            Transform worldTransform = Transform::Identity;

            if ( bHasLOD )
            {
                worldTransform.AddTranslation( Vector( -1, 0, 0 ) );
                drawingCtx.DrawTextBox3D( worldTransform.GetTranslation() + Vector( 0, 0, 2 ), "LOD: High", Colors::HotPink );
            }

            DrawSkeleton( worldTransform, Skeleton::LOD::High, Colors::HotPink, Colors::Cyan );

            if ( m_pPreviewMeshComponent != nullptr )
            {
                m_pPreviewMeshComponent->SetWorldTransform( worldTransform );
            }

            if ( bHasLOD )
            {
                worldTransform.AddTranslation( Vector( 2, 0, 0 ) );
                DrawSkeleton( worldTransform, Skeleton::LOD::Low, Colors::Yellow, Colors::Teal );
                drawingCtx.DrawTextBox3D( worldTransform.GetTranslation() + Vector( 0, 0, 2 ), "LOD: Low", Colors::Yellow );
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

    void SkeletonEditor::DrawSkeletonOutlinerWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        EE_ASSERT( m_pSkeletonTreeRoot != nullptr );

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        int32_t const numBones = pSkeleton->GetNumBones();

        //-------------------------------------------------------------------------

        auto ApplySelectionRequests = [this, pSkeleton, numBones] ( ImGuiMultiSelectIO* pMSIO )
        {
            for ( ImGuiSelectionRequest const& req : pMSIO->Requests )
            {
                if ( req.Type == ImGuiSelectionRequestType_SetAll )
                {
                    m_selectedBoneIDs.clear();

                    if ( req.Selected )
                    {
                        for ( int32_t i = 0; i < numBones; i++ )
                        {
                            m_selectedBoneIDs.emplace_back( pSkeleton->GetBoneID( i ) );
                        }
                    }
                }
                else if ( req.Type == ImGuiSelectionRequestType_SetRange )
                {
                    if ( req.Selected )
                    {
                        for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                        {
                            StringID const boneID = pSkeleton->GetBoneID( (int32_t) i );
                            VectorEmplaceBackUnique( m_selectedBoneIDs, boneID );
                        }
                    }
                    else
                    {
                        for ( int64_t i = req.RangeFirstItem; i <= req.RangeLastItem; i++ )
                        {
                            StringID const boneID = pSkeleton->GetBoneID( (int32_t) i );
                            m_selectedBoneIDs.erase_first_unsorted( boneID );
                        }
                    }
                }
            }
        };

        //-------------------------------------------------------------------------

        ImGuiMultiSelectFlags const selectionFlags = ImGuiMultiSelectFlags_ClearOnEscape | ImGuiMultiSelectFlags_BoxSelect1d | ImGuiMultiSelectFlags_ClearOnClickVoid;
        ImGuiMultiSelectIO* pMSIO = ImGui::BeginMultiSelect( selectionFlags, -1, numBones );
        ApplySelectionRequests( pMSIO );

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

            DrawSkeletonTreeRow( m_pSkeletonTreeRoot );

            ImGui::EndTable();
        }

        pMSIO = ImGui::EndMultiSelect();
        ApplySelectionRequests( pMSIO );
    }

    void SkeletonEditor::CreateSkeletonTree()
    {
        // Create all bones
        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        int32_t const numBones = pSkeleton->GetNumBones();
        for ( auto i = 0; i < numBones; i++ )
        {
            auto& pBoneInfo = m_bones.emplace_back( EE::New<BoneItem>() );
            pBoneInfo->m_skeletonID = pSkeleton->GetResourceID();
            pBoneInfo->m_boneID = pSkeleton->GetBoneID( i );
            pBoneInfo->m_boneIdx = i;
        }

        // Create hierarchy
        for ( auto i = 1; i < numBones; i++ )
        {
            int32_t const parentBoneIdx = pSkeleton->GetParentBoneIndex( i );
            EE_ASSERT( parentBoneIdx != InvalidIndex );
            m_bones[parentBoneIdx]->m_children.emplace_back( m_bones[i] );
        }

        // Set root
        m_pSkeletonTreeRoot = m_bones[0];
    }

    void SkeletonEditor::DestroySkeletonTree()
    {
        if ( m_pSkeletonTreeRoot != nullptr )
        {
            m_pSkeletonTreeRoot->DestroyChildren();
            EE::Delete( m_pSkeletonTreeRoot );
        }

        m_bones.clear();
    }

    void SkeletonEditor::DrawSkeletonTreeRow( BoneItem* pBone )
    {
        EE_ASSERT( pBone != nullptr );

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        int32_t const boneIdx = pBone->m_boneIdx;
        StringID const currentBoneID = pSkeleton->GetBoneID( boneIdx );
        bool const isSelected = VectorContains( m_selectedBoneIDs, currentBoneID );

        //-------------------------------------------------------------------------

        ImGui::TableNextRow();

        //-------------------------------------------------------------------------
        // Draw Label
        //-------------------------------------------------------------------------

        bool const isHighLOD = GetBoneLOD( currentBoneID ) == Skeleton::LOD::High;
        InlineString const boneLabel( InlineString::CtorSprintf(), "%d. %s", boneIdx, currentBoneID.c_str() );

        ImGui::TableNextColumn();

        int32_t treeNodeFlags = ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_OpenOnArrow;
        if ( pBone->m_children.empty() )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
        }

        if ( isSelected )
        {
            treeNodeFlags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::SetNextItemOpen( pBone->m_isExpanded );
        ImGui::AlignTextToFramePadding();

        ImGui::SetNextItemSelectionUserData( boneIdx );
        Color const rowColor = IsEditingBoneMask() ? Skeleton::GetColorForBoneWeight( m_previewBoneMask.GetWeight( boneIdx ) ) : ImGuiX::Style::s_colorText;
        {
            ImGuiX::ScopedFont const sf( isSelected ? ImGuiX::Font::MediumBold : ImGuiX::Font::Default, rowColor );
            pBone->m_isExpanded = ImGui::TreeNodeEx( boneLabel.c_str(), treeNodeFlags );
        }

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
                ImGui::SeparatorText( "Propagate");

                if ( ImGui::MenuItem( EE_ICON_CONTENT_COPY" Propagate Current Weight" ) )
                {
                    SetAllChildWeights( boneIdx, pBone->m_weight );
                }

                //-------------------------------------------------------------------------

                ImGui::SeparatorText( "Fully Masked" );

                if ( boneIdx != 0 && ImGui::MenuItem( EE_ICON_NUMERIC_0_BOX_MULTIPLE_OUTLINE" Set bone and all children to 0" ) )
                {
                    SetAllChildWeights( boneIdx, 0.0f, true );
                }

                if ( ImGui::MenuItem( EE_ICON_NUMERIC_0_BOX_MULTIPLE_OUTLINE" Set all children to 0" ) )
                {
                    SetAllChildWeights( boneIdx, 0.0f );
                }

                //-------------------------------------------------------------------------

                ImGui::SeparatorText( "No Mask" );

                if ( boneIdx != 0 && ImGui::MenuItem( EE_ICON_NUMERIC_1_BOX_MULTIPLE_OUTLINE" Set bone and all children to 1" ) )
                {
                    SetAllChildWeights( boneIdx, 1.0f, true );
                }

                if ( ImGui::MenuItem( EE_ICON_NUMERIC_1_BOX_MULTIPLE_OUTLINE" Set all children to 1" ) )
                {
                    SetAllChildWeights( boneIdx, 1.0f );
                }

                //-------------------------------------------------------------------------

                ImGui::SeparatorText( "Clear" );

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

            bool isSet = pBone->m_weight != -1.0f;
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
                ImGui::SliderFloat( weightEditorID.c_str(), &pBone->m_weight, 0.0f, 1.0f, "%.2f" );
                if ( ImGui::IsItemDeactivatedAfterEdit() )
                {
                    SetWeight( boneIdx, pBone->m_weight );
                }
            }
            else
            {
                ImGui::SameLine();
                float const weight = m_previewBoneMask.GetWeight( boneIdx );
                ImGui::PushStyleColor( ImGuiCol_Text, rowColor );
                ImGui::Text( "%.2f", weight );
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
            for ( BoneItem* pChildBone : pBone->m_children )
            {
                DrawSkeletonTreeRow( pChildBone );
            }
            ImGui::TreePop();
        }
    }

    void SkeletonEditor::DrawBoneInfoWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        Skeleton const* pSkeleton = m_editedResource.GetPtr();

        if ( ImGui::BeginTable( "BoneTreeTable", 3, ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersOuterH | ImGuiTableFlags_RowBg | ImGuiTableFlags_NoBordersInBody | ImGuiTableFlags_ScrollY, ImGui::GetContentRegionAvail() ) )
        {
            ImGui::TableSetupColumn( "Bone", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Parent Space", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupColumn( "Model Space", ImGuiTableColumnFlags_NoHide | ImGuiTableColumnFlags_WidthStretch );
            ImGui::TableSetupScrollFreeze( 0, 1 );

            ImGui::TableHeadersRow();

            //-------------------------------------------------------------------------

            for ( StringID const& boneID : m_selectedBoneIDs )
            {
                int32_t const selectedBoneIdx = pSkeleton->GetBoneIndex( boneID );
                EE_ASSERT( selectedBoneIdx != InvalidIndex );
                Transform const& parentSpaceBoneTransform = pSkeleton->GetParentSpaceReferencePose()[selectedBoneIdx];
                Transform const& modelSpaceBoneTransform = pSkeleton->GetModelSpaceReferencePose()[selectedBoneIdx];

                ImGui::TableNextRow();

                ImGui::TableNextColumn();
                ImGui::Text( "%d. %s", selectedBoneIdx, pSkeleton->GetBoneID( selectedBoneIdx ).c_str() );

                ImGui::TableNextColumn();
                ImGuiX::DrawTransform( parentSpaceBoneTransform );

                ImGui::TableNextColumn();
                ImGuiX::DrawTransform( modelSpaceBoneTransform );
            }

            //-------------------------------------------------------------------------

            ImGui::EndTable();
        }
    }

    // LOD
    //-------------------------------------------------------------------------

    void SkeletonEditor::ValidateLODSetup()
    {
        EE_ASSERT( IsDataFileLoaded() && IsResourceLoaded() );

        auto& highLODBones = GetDataFile<SkeletonResourceDescriptor>()->m_highLODBones;
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
        EE_ASSERT( IsDataFileLoaded() );
        return VectorContains( GetDataFile<SkeletonResourceDescriptor>()->m_highLODBones, boneID ) ? Skeleton::LOD::High : Skeleton::LOD::Low;
    }

    void SkeletonEditor::SetBoneHierarchyLOD( StringID boneID, Skeleton::LOD lod )
    {
        EE_ASSERT( IsDataFileLoaded() );
        auto& highLODBones = GetDataFile<SkeletonResourceDescriptor>()->m_highLODBones;

        ScopedDataFileModification const sdm( this );

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

    void SkeletonEditor::DrawBoneMasksWindow( UpdateContext const& context, bool isFocused )
    {
        if ( !IsResourceLoaded() )
        {
            return;
        }

        //-------------------------------------------------------------------------

        auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();

        ImGui::BeginDisabled( !IsDataFileLoaded() );
        if ( ImGuiX::IconButtonColored( EE_ICON_PLUS, "Add New Mask", Colors::Green, Colors::White, Colors::White, ImVec2( -1, 0 ) ) )
        {
            CreateBoneMask();
        }
        ImGui::EndDisabled();

        //-------------------------------------------------------------------------

        static ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;
        ImGui::PushStyleVar( ImGuiStyleVar_CellPadding, ImVec2( 8, 4 ) );
        if ( ImGui::BeginTable( "BoneMasks", 2, flags, ImGui::GetContentRegionAvail() ) )
        {
            static ImVec2 const buttonSize( 45, 0 );

            ImGui::TableSetupColumn( "Name", ImGuiTableColumnFlags_NoHide );
            ImGui::TableSetupColumn( "Controls", ImGuiTableColumnFlags_WidthFixed, ( buttonSize.x * 2 + ( ImGui::GetStyle().ItemSpacing.x ) ) );

            //-------------------------------------------------------------------------

            if ( pDescriptor != nullptr )
            {
                for ( auto i = 0; i < pDescriptor->m_boneMaskSetDefinitions.size(); i++ )
                {
                    auto const& boneMaskDefinition = pDescriptor->m_boneMaskSetDefinitions[i];
                    ImGui::PushID( &boneMaskDefinition );
                    ImGui::TableNextRow();

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();

                    ImGui::AlignTextToFramePadding();

                    bool const isSelected = ( m_editedMaskIdx != InvalidIndex ) ? ( m_editedMaskIdx == i ) : false;
                    ImGui::PushStyleColor( ImGuiCol_Text, isSelected ? Colors::Lime : ImGuiX::Style::s_colorText );
                    InlineString const label( InlineString::CtorSprintf(), EE_ICON_DRAMA_MASKS" %s", boneMaskDefinition.m_ID.c_str() );
                    if ( ImGui::Selectable( label.c_str(), isSelected, 0 ) )
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
                            m_pToolsContext->m_pDialogManager->StartModalDialog( "Rename Mask", [this] () { return DrawRenameBoneMaskDialog(); } );
                        }

                        if ( ImGui::MenuItem( EE_ICON_DELETE" Delete" ) )
                        {
                            m_dialogMaskID = boneMaskDefinition.m_ID;
                            m_pToolsContext->m_pDialogManager->StartModalDialog( "Delete Mask?", [this] () { return DrawDeleteBoneMaskDialog(); } );
                        }

                        ImGui::EndPopup();
                    }

                    //-------------------------------------------------------------------------

                    ImGui::TableNextColumn();

                    if ( isSelected )
                    {
                        if ( ImGuiX::IconButton( EE_ICON_CANCEL, "Stop Edit", Colors::Red, ImVec2( -1, 0 ) ) )
                        {
                            StopEditingMask();
                        }
                        ImGuiX::ItemTooltip( "Stop Editing Mask" );
                    }
                    else
                    {
                        if ( ImGui::Button( EE_ICON_SQUARE_EDIT_OUTLINE"##Rename", buttonSize ) )
                        {
                            EE::Printf( m_renameBuffer, 255, boneMaskDefinition.m_ID.c_str() );
                            m_dialogMaskID = boneMaskDefinition.m_ID;
                            m_pToolsContext->m_pDialogManager->StartModalDialog( "Rename Mask", [this] () { return DrawRenameBoneMaskDialog(); } );
                        }
                        ImGuiX::ItemTooltip( "Rename" );

                        ImGui::SameLine();

                        if ( ImGui::Button( EE_ICON_DELETE"##Delete", buttonSize ) )
                        {
                            m_dialogMaskID = boneMaskDefinition.m_ID;
                            m_pToolsContext->m_pDialogManager->StartModalDialog( "Delete Mask?", [this] () { return DrawDeleteBoneMaskDialog(); } );
                        }
                        ImGuiX::ItemTooltip( "Delete" );
                    }

                    //-------------------------------------------------------------------------

                    ImGui::PopID();
                }
            }

            ImGui::EndTable();
        }
        ImGui::PopStyleVar( 1 );
    }

    void SkeletonEditor::ReflectEditedWeights()
    {
        auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();
        BoneMaskSetDefinition& editedBoneMaskDefinition = pDescriptor->m_boneMaskSetDefinitions[m_editedMaskIdx];

        // Reset all weights
        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        auto const numBones = pSkeleton->GetNumBones();
        for ( int32_t i = 0; i < numBones; i++ )
        {
            m_bones[i]->m_weight = -1.0f;
        }

        // Transfer any set weights
        int32_t const numWeights = editedBoneMaskDefinition.m_primaryWeightList.GetNumWeights();
        for ( int32_t i = 0; i < numWeights; i++ )
        {
            int32_t const boneIdx = pSkeleton->GetBoneIndex( editedBoneMaskDefinition.m_primaryWeightList.m_boneIDs[i] );
            if( boneIdx != InvalidIndex && editedBoneMaskDefinition.m_primaryWeightList.m_weights[i] >= 0.0f )
            {
                m_bones[boneIdx]->m_weight = editedBoneMaskDefinition.m_primaryWeightList.m_weights[i];
            }
        }
    }

    void SkeletonEditor::StartEditingMask( StringID maskID )
    {
        EE_ASSERT( maskID.IsValid() );
        EE_ASSERT( m_editedMaskIdx == InvalidIndex );

        BoneMaskSetDefinition* pEditedBoneMaskDefinition = nullptr;
        auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();
        for ( auto i = 0; i < pDescriptor->m_boneMaskSetDefinitions.size(); i++ )
        {
            if ( pDescriptor->m_boneMaskSetDefinitions[i].m_ID == maskID )
            {
                pEditedBoneMaskDefinition = &pDescriptor->m_boneMaskSetDefinitions[i];
                m_editedMaskIdx = i;
                break;
            }
        }

        EE_ASSERT( pEditedBoneMaskDefinition != nullptr );

        ReflectEditedWeights();
    }

    void SkeletonEditor::StopEditingMask()
    {
        m_editedMaskIdx = InvalidIndex;
    }

    void SkeletonEditor::CreateBoneMask()
    {
        EE_ASSERT( IsDataFileLoaded() );

        ScopedDataFileModification const sdm( this );
        auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();
        pDescriptor->m_boneMaskSetDefinitions.emplace_back( StringID( "Bone Mask" ) );
        GenerateUniqueBoneMaskName( (int32_t) pDescriptor->m_boneMaskSetDefinitions.size() - 1 );
    }

    void SkeletonEditor::DestroyBoneMask( StringID maskID )
    {
        EE_ASSERT( IsDataFileLoaded() );

        if ( IsEditingBoneMask() )
        {
            StopEditingMask();
        }

        //-------------------------------------------------------------------------

        ScopedDataFileModification const sdm( this );
        auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();
        for ( auto iter = pDescriptor->m_boneMaskSetDefinitions.begin(); iter != pDescriptor->m_boneMaskSetDefinitions.end(); ++iter )
        {
            if ( iter->m_ID == maskID )
            {
                pDescriptor->m_boneMaskSetDefinitions.erase( iter );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void SkeletonEditor::RenameBoneMask( StringID oldID, StringID newID )
    {
        EE_ASSERT( IsDataFileLoaded() );

        ScopedDataFileModification const sdm( this );
        auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();
        for ( auto i = 0; i < pDescriptor->m_boneMaskSetDefinitions.size(); i++ )
        {
            if ( pDescriptor->m_boneMaskSetDefinitions[i].m_ID == oldID )
            {
                pDescriptor->m_boneMaskSetDefinitions[i].m_ID = newID;
                GenerateUniqueBoneMaskName( i );
                return;
            }
        }

        EE_UNREACHABLE_CODE();
    }

    void SkeletonEditor::SetWeight( int32_t boneIdx, float weight )
    {
        EE_ASSERT( IsValidWeight( weight ) );
        EE_ASSERT( IsDataFileLoaded() );
        EE_ASSERT( IsEditingBoneMask() );

        //-------------------------------------------------------------------------

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        EE_ASSERT( pSkeleton != nullptr );
        StringID const editedBoneID = pSkeleton->GetBoneID( boneIdx );

        if ( m_selectedBoneIDs.empty() )
        {
            m_selectedBoneIDs.emplace_back( editedBoneID );
        }
        else // Have a pre-existing selection
        {
            if ( !VectorContains( m_selectedBoneIDs, editedBoneID ) )
            {
                m_selectedBoneIDs.clear();
                m_selectedBoneIDs.emplace_back( editedBoneID );
            }
        }

        //-------------------------------------------------------------------------

        ScopedDataFileModification const sm( this );

        auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();
        BoneMaskSetDefinition& editedBoneMaskDefinition = pDescriptor->m_boneMaskSetDefinitions[m_editedMaskIdx];

        for ( StringID const& boneID : m_selectedBoneIDs )
        {
            int32_t const selectedBoneIdx = pSkeleton->GetBoneIndex( boneID );
            EE_ASSERT( selectedBoneIdx != InvalidIndex );

            if ( weight < 0.0f )
            {
                m_bones[selectedBoneIdx]->m_weight = -1.0f;
                editedBoneMaskDefinition.m_primaryWeightList.UnsetWeightForBone( boneID );
            }
            else
            {
                m_bones[selectedBoneIdx]->m_weight = weight;
                editedBoneMaskDefinition.m_primaryWeightList.SetWeightForBone( boneID, weight );
            }
        }
    }

    void SkeletonEditor::SetAllChildWeights( int32_t parentBoneIdx, float weight, bool bIncludeParent )
    {
        EE_ASSERT( IsValidWeight( weight ) );
        EE_ASSERT( IsDataFileLoaded() );
        EE_ASSERT( IsEditingBoneMask() );

        ScopedDataFileModification const sm( this );

        if ( bIncludeParent )
        {
            //m_editedBoneWeights[parentBoneIdx] = weight;
        }

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        auto const numBones = pSkeleton->GetNumBones();
        for ( auto boneIdx = 1; boneIdx < numBones; boneIdx++ )
        {
            if ( pSkeleton->IsChildBoneOf( parentBoneIdx, boneIdx ) )
            {
                //m_editedBoneWeights[boneIdx] = weight;
            }
        }
    }

    void SkeletonEditor::ValidateDescriptorBoneMaskDefinitions()
    {
        EE_ASSERT( IsDataFileLoaded() );
        EE_ASSERT( IsResourceLoaded() );

        bool fixupPerformed = false;

        //-------------------------------------------------------------------------

        Skeleton const* pSkeleton = m_editedResource.GetPtr();
        auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();

        for ( int32_t i = 0; i < (int32_t) pDescriptor->m_boneMaskSetDefinitions.size(); i++ )
        {
            auto& boneMaskSetDefinition = pDescriptor->m_boneMaskSetDefinitions[i];

            // Fix invalid names
            if ( !boneMaskSetDefinition.m_ID.IsValid() )
            {
                boneMaskSetDefinition.m_ID = StringID( "BoneMask" );
                fixupPerformed = true;
            }

            // Check for unique name
            for ( auto j = 0; j < pDescriptor->m_boneMaskSetDefinitions.size(); j++ )
            {
                if ( &pDescriptor->m_boneMaskSetDefinitions[j] == &boneMaskSetDefinition )
                {
                    continue;
                }

                // Duplicate name detected!
                if ( boneMaskSetDefinition.m_ID == pDescriptor->m_boneMaskSetDefinitions[j].m_ID )
                {
                    GenerateUniqueBoneMaskName( i );
                    fixupPerformed = true;
                }
            }

            // Check Weight lists
            //-------------------------------------------------------------------------

            if ( !boneMaskSetDefinition.m_primaryWeightList.m_skeletonID.IsValid() )
            {
                boneMaskSetDefinition.m_primaryWeightList.m_skeletonID = m_editedResource.GetResourceID();
                fixupPerformed = true;
            }

            if ( boneMaskSetDefinition.m_primaryWeightList.RemoveDuplicateWeights() )
            {
                fixupPerformed = true;
            }

            if ( boneMaskSetDefinition.m_primaryWeightList.RemoveWeightsForMissingBones( pSkeleton ) )
            {
                fixupPerformed = true;
            }

            for ( auto& secondaryWeightList : boneMaskSetDefinition.m_secondaryWeightLists )
            {
                /*if ( !boneMaskSetDefinition.m_primaryWeightList.m_skeletonID.IsValid() )
                {
                    boneMaskSetDefinition.m_primaryWeightList.m_skeletonID = m_editedResource.GetResourceID();
                    fixupPerformed = true;
                }*/

                if ( boneMaskSetDefinition.m_primaryWeightList.RemoveDuplicateWeights() )
                {
                    fixupPerformed = true;
                }

                if ( boneMaskSetDefinition.m_primaryWeightList.RemoveWeightsForMissingBones( pSkeleton ) )
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

        EE_ASSERT( IsDataFileLoaded() );
        auto pDescriptor = GetDataFile<SkeletonResourceDescriptor>();
        EE_ASSERT( boneMaskIdx >= 0 && boneMaskIdx < pDescriptor->m_boneMaskSetDefinitions.size() );

        InlineString desiredName = pDescriptor->m_boneMaskSetDefinitions[boneMaskIdx].m_ID.c_str();
        InlineString finalName = desiredName;

        uint32_t counter = 0;
        bool isUniqueName = false;
        while ( !isUniqueName )
        {
            isUniqueName = true;

            for ( auto i = 0; i < pDescriptor->m_boneMaskSetDefinitions.size(); i++ )
            {
                if ( boneMaskIdx == i )
                {
                    continue;
                }

                if ( finalName.comparei( pDescriptor->m_boneMaskSetDefinitions[i].m_ID.c_str()) == 0 )
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

        pDescriptor->m_boneMaskSetDefinitions[boneMaskIdx].m_ID = StringID( finalName.c_str() );
    }

    bool SkeletonEditor::DrawDeleteBoneMaskDialog()
    {
        constexpr static char const * const yesStr = "Yes";
        constexpr static char const * const noStr = "No";
        float const buttonWidth = Math::Max( ImGuiX::CalculateButtonWidth( yesStr ), ImGuiX::CalculateButtonWidth( noStr ) );

        ImGui::Dummy( ImVec2( 400, 0 ) );

        ImGui::Text( "Are you sure you want to delete this bone mask?" );

        ImGui::NewLine();

        float const dialogWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SameLine( 0, dialogWidth - ( buttonWidth * 2 ) - ImGui::GetStyle().ItemSpacing.x );

        if ( ImGui::Button( yesStr, ImVec2( buttonWidth, 0 ) ) )
        {
            DestroyBoneMask( m_dialogMaskID );
            m_dialogMaskID.Clear();
        }

        ImGui::SameLine();

        if ( ImGui::Button( noStr, ImVec2( buttonWidth, 0 ) ) )
        {
            m_dialogMaskID.Clear();
        }

        return m_dialogMaskID.IsValid();
    }

    bool SkeletonEditor::DrawRenameBoneMaskDialog()
    {
        SkeletonResourceDescriptor* pDescriptor = GetDataFile<SkeletonResourceDescriptor>();

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
            for ( auto const& def : pDescriptor->m_boneMaskSetDefinitions )
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

        ImGui::SetNextItemWidth( 400 );
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

        constexpr static char const * const okStr = "Ok";
        constexpr static char const * const cancelStr = "Cancel";
        float const buttonWidth = Math::Max( ImGuiX::CalculateButtonWidth( okStr ), ImGuiX::CalculateButtonWidth( cancelStr ) );

        ImGui::NewLine();

        float const dialogWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SameLine( 0, dialogWidth - ( buttonWidth * 2 ) - ImGui::GetStyle().ItemSpacing.x );

        ImGui::BeginDisabled( !isValidName );
        if ( ImGui::Button( okStr, ImVec2( buttonWidth, 0 ) ) || renameRequested )
        {
            RenameBoneMask( m_dialogMaskID, StringID( m_renameBuffer ) );
            m_dialogMaskID.Clear();
        }
        ImGui::EndDisabled();

        ImGui::SameLine();

        if ( ImGui::Button( cancelStr, ImVec2( buttonWidth, 0 ) ) )
        {
            m_dialogMaskID.Clear();
        }

        return m_dialogMaskID.IsValid();
    }

    // Clip Browser
    //-------------------------------------------------------------------------

    void SkeletonEditor::DrawClipBrowser( UpdateContext const& context, bool isFocused )
    {
        m_animationClipBrowser.Draw( context, isFocused );
    }
}