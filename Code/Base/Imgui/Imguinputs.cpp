#include "ImguiInputs.h"
#include "ImguiX.h"
#include "ImguiFilter.h"
#include "EASTL/sort.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    bool InputTextWithClearButton( const char* pInputTextID, char const* pHelpText, char* pBuffer, size_t bufferSize, ImGuiInputTextFlags flags, ImGuiInputTextCallback pInputTextCallback, void* pUserData )
    {
        ImGuiStyle const& style = ImGui::GetStyle();
        ImGuiContext& g = *GImGui;
        auto pDrawList = ImGui::GetWindowDrawList();

        ImGui::PushID( pInputTextID );

        // Calculate sizes
        //-------------------------------------------------------------------------

        float const clearButtonWidth = ImGui::CalcTextSize( EE_ICON_CLOSE ).x + ( style.ItemSpacing.x * 2 );

        float totalWidgetWidth = 0;
        if ( g.NextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth )
        {
            totalWidgetWidth = g.NextItemData.Width;
        }

        float inputWidth = 0;
        if ( totalWidgetWidth > 0 )
        {
            inputWidth = Math::Max( totalWidgetWidth - clearButtonWidth, 1.0f );
        }
        else if ( totalWidgetWidth == 0 )
        {
            inputWidth = Math::Max( ImGui::GetContentRegionAvail().x - clearButtonWidth, 1.0f ); // Max allowable size
            inputWidth = Math::Min( inputWidth, 100.0f ); // arbitrary choice
        }
        else if ( totalWidgetWidth < 0 )
        {
            inputWidth = Math::Max( ImGui::GetContentRegionAvail().x - clearButtonWidth, 1.0f );
        }

        // Draw Background
        //-------------------------------------------------------------------------

        pDrawList->AddRectFilled( ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2( inputWidth + clearButtonWidth, ImGui::GetFrameHeight() ), ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_FrameBg] ), style.FrameRounding );

        // Draw Input
        //-------------------------------------------------------------------------

        InlineString const inputFinalID( InlineString::CtorSprintf(), "##%s", pInputTextID ); // Ensure we have no label!

        ImGui::SetNextItemWidth( inputWidth );
        bool inputResult = ImGui::InputText( inputFinalID.c_str(), pBuffer, bufferSize, flags, pInputTextCallback, pUserData );
        inputResult |= ImGui::IsItemDeactivatedAfterEdit();
        ImRect const inputRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        ImVec2 const windowStartPos = inputRect.GetBL() - ImVec2( 0, style.FrameRounding );

        // Clear Button
        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 0 );

        // Draw clear button
        bool const isInputFocused = ImGui::IsItemFocused() && ImGui::IsItemActive();
        bool const isbufferEmpty = strlen( pBuffer ) == 0;
        if ( !isbufferEmpty )
        {
            ImVec2 const buttonSize = ImVec2( clearButtonWidth, ImGui::GetFrameHeight() );
            ImVec2 const cursorScreenPos = ImGui::GetCursorScreenPos();
            bool const isButtonHovered = ImGui::IsMouseHoveringRect( cursorScreenPos, cursorScreenPos + buttonSize );

            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, style.FramePadding.y ) );
            ImGui::PushStyleColor( ImGuiCol_Button, Colors::Transparent );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, Colors::Transparent );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Colors::Transparent );
            ImGui::PushStyleColor( ImGuiCol_Text, isButtonHovered ? Colors::White : Colors::Gray );

            if ( ImGui::Button( EE_ICON_CLOSE"##Clear", buttonSize ) )
            {
                Memory::MemsetZero( pBuffer, bufferSize );
                inputResult = true;
            }

            ImGui::PopStyleColor( 4 );
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::Dummy( ImVec2( clearButtonWidth, 0 ) );
        }

        // Draw help text
        if ( pHelpText != nullptr && isbufferEmpty && !isInputFocused )
        {
            ImVec2 const textPos = inputRect.GetTL() + ImVec2( style.ItemSpacing.x, style.FramePadding.y );
            pDrawList->AddText( textPos, ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_TextDisabled] ), pHelpText );
        }

        ImGui::PopID();

        return inputResult;
    }

    //-------------------------------------------------------------------------

    bool InputTextWithOptions( const char* pInputTextID, char* pBuffer, size_t bufferSize, OptionData* pOptionData, ImGuiInputTextFlags extraFlags, ImGuiInputTextCallback pInputTextCallback, void* pUserData )
    {
        EE_ASSERT( pOptionData != nullptr );

        if ( pOptionData->m_optionsNeedRebuild )
        {
            pOptionData->RebuildOptionsListFromProvider();
        }

        // Inspired by https://github.com/ocornut/imgui/issues/718

        //-------------------------------------------------------------------------

        ImGui::PushID( pInputTextID );

        ImGuiStyle const& style = ImGui::GetStyle();
        ImGuiContext& g = *GImGui;
        auto pDrawList = ImGui::GetWindowDrawList();

        // Calculate sizes
        //-------------------------------------------------------------------------

        float const clearButtonWidth = ImGui::CalcTextSize( EE_ICON_CLOSE ).x + ( style.ItemSpacing.x * 2 );
        float const comboButtonWidth = ImGui::GetFrameHeight();
        float const requiredControlsWidth = clearButtonWidth + comboButtonWidth;

        float totalWidgetWidth = 0;
        if ( g.NextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth )
        {
            totalWidgetWidth = g.NextItemData.Width;
        }

        float inputWidth = 0;
        if ( totalWidgetWidth > 0 )
        {
            inputWidth = Math::Max( totalWidgetWidth - requiredControlsWidth, 1.0f );
        }
        else if ( totalWidgetWidth == 0 )
        {
            inputWidth = Math::Max( ImGui::GetContentRegionAvail().x - requiredControlsWidth, 1.0f ); // Max allowable size
            inputWidth = Math::Min( inputWidth, 100.0f ); // arbitrary choice
        }
        else if ( totalWidgetWidth < 0 )
        {
            inputWidth = Math::Max( ImGui::GetContentRegionAvail().x - requiredControlsWidth, 1.0f );
        }

        // Draw Background
        //-------------------------------------------------------------------------

        pDrawList->AddRectFilled( ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2( inputWidth + clearButtonWidth, ImGui::GetFrameHeight() ), ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_FrameBg] ), style.FrameRounding );

        // Draw Input
        //-------------------------------------------------------------------------

        struct CallbackContext
        {
            OptionData*                 m_pOptionData = nullptr;
            ImGuiInputTextFlags         m_userFlags = 0;
            ImGuiInputTextCallback      m_pUserCallback = nullptr;
            void*                       m_pUserData = nullptr;
            char*                       m_pResizedBuffer = nullptr;
            size_t                      m_bufferSize = 0;
        };

        auto InputCallback = [] ( ImGuiInputTextCallbackData* pData ) -> int
        {
            CallbackContext* pCtx = (CallbackContext*) pData->UserData;

            if ( pData->EventFlag == ImGuiInputTextFlags_CallbackEdit )
            {
                pCtx->m_pOptionData->SetFilter( pData->Buf );
            }

            // Forward callback to user callback
            if ( pCtx->m_pUserCallback != nullptr && ( pCtx->m_userFlags & pData->EventFlag ) != 0 )
            {
                pData->UserData = pCtx->m_pUserData;
                pCtx->m_pUserCallback( pData );
                pData->UserData = pCtx;
            }

            // Get back the resized buffer
            if( pData->EventFlag == ImGuiInputTextFlags_CallbackResize )
            {
                pCtx->m_pResizedBuffer = pData->Buf;
                pCtx->m_bufferSize = pData->BufSize;
            }

            return 0;
        };

        CallbackContext ctx;
        ctx.m_pOptionData = pOptionData;
        ctx.m_pUserCallback = pInputTextCallback;
        ctx.m_userFlags = extraFlags;
        ctx.m_pUserData = pUserData;
        ctx.m_pResizedBuffer = pBuffer;
        ctx.m_bufferSize = bufferSize;

        ImGui::SetNextItemWidth( inputWidth );

        if ( pOptionData->m_preWidgetFunc != nullptr )
        {
            pOptionData->m_preWidgetFunc();
        }

        ImGui::PushItemFlag( ImGuiItemFlags_LiveEditOnInputText, false );
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackEdit | extraFlags;
        bool inputResult = ImGui::InputText( "##IT", pBuffer, bufferSize, flags, InputCallback, &ctx );
        ImGui::PopItemFlag();

        if ( pOptionData->m_postWidgetFunc != nullptr )
        {
            pOptionData->m_postWidgetFunc();
        }

        bool const isInputActive = ImGui::IsItemActive();
        ImGuiID const inputTextID = ImGui::GetItemID();
        ImGuiInputTextState* pInputState = isInputActive ? ImGui::GetInputTextState( inputTextID ) : nullptr;

        inputResult |= ImGui::IsItemDeactivatedAfterEdit();
        ImRect const inputRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );
        ImVec2 const windowStartPos = inputRect.GetBL();

        // Handle buffer resizes
        if ( pBuffer != ctx.m_pResizedBuffer )
        {
            pBuffer = ctx.m_pResizedBuffer;
            bufferSize = ctx.m_bufferSize;
        }

        // Clear Button
        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 0 );

        // Draw clear button
        bool const isInputFocused = ImGui::IsItemFocused() && ImGui::IsItemActive();
        bool const isbufferEmpty = strlen( pBuffer ) == 0;
        if ( !isbufferEmpty )
        {
            ImVec2 const buttonSize = ImVec2( clearButtonWidth, ImGui::GetFrameHeight() );
            ImVec2 const cursorScreenPos = ImGui::GetCursorScreenPos();
            bool const isButtonHovered = ImGui::IsMouseHoveringRect( cursorScreenPos, cursorScreenPos + buttonSize );

            ImGui::PushStyleVar( ImGuiStyleVar_FramePadding, ImVec2( 0, style.FramePadding.y ) );
            ImGui::PushStyleColor( ImGuiCol_Button, Colors::Transparent );
            ImGui::PushStyleColor( ImGuiCol_ButtonActive, Colors::Transparent );
            ImGui::PushStyleColor( ImGuiCol_ButtonHovered, Colors::Transparent );
            ImGui::PushStyleColor( ImGuiCol_Text, isButtonHovered ? Colors::White : Colors::Gray );

            if ( ImGui::Button( EE_ICON_CLOSE"##Clear", buttonSize ) )
            {
                Memory::MemsetZero( pBuffer, bufferSize );
                inputResult = true;
            }

            ImGui::PopStyleColor( 4 );
            ImGui::PopStyleVar();
        }
        else
        {
            ImGui::Dummy( ImVec2( clearButtonWidth, 0 ) );
        }

        // Draw help text
        StringView helpText( pOptionData->m_filterData.GetFilterHelpText() );
        if ( !helpText.empty() && isbufferEmpty && !isInputFocused )
        {
            ImVec2 const textPos = inputRect.GetTL() + ImVec2( style.ItemSpacing.x, style.FramePadding.y );
            pDrawList->AddText( textPos, ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_TextDisabled] ), helpText.data() );
        }

        // Combo Button
        //-------------------------------------------------------------------------

        ImGui::SameLine( 0, 0 );

        float const comboButtonHeight = ImGui::GetFrameHeight();
        {
            ImGuiX::ScopedFont sf( ImGuiX::Font::MediumBold, ImGuiX::Style::s_colorText );
            if ( ImGui::Button( EE_ICON_MENU_DOWN"##comboButton", ImVec2( comboButtonWidth, comboButtonHeight ) ) )
            {
                ImGui::ActivateItemByID( inputTextID );
            }
        }

        ImRect const comboButtonRect( ImGui::GetItemRectMin(), ImGui::GetItemRectMax() );

        // Fill gap between button and drop down button
        //-------------------------------------------------------------------------

        ImVec2 const fillerMin = comboButtonRect.Min + ImVec2( -style.ItemSpacing.x, 0 );
        ImVec2 const fillerMax = comboButtonRect.GetBL() + ImVec2( style.FrameRounding, 0 );
        pDrawList->AddRectFilled( fillerMin, fillerMax, ImGui::ColorConvertFloat4ToU32( style.Colors[ImGuiCol_FrameBg] ) );

        // Combo Popup
        //-------------------------------------------------------------------------

        bool const isComboOpen = ImGui::IsPopupOpen( inputTextID, ImGuiPopupFlags_None );

        if ( ( isInputActive ) && !isComboOpen )
        {
            ImGui::OpenPopup( inputTextID );
        }

        auto CalcPopupHeightFromItemCount = [&] ()
        {
            ImGuiContext& g = *GImGui;

            int32_t const numOptions = isComboOpen ? pOptionData->GetNumFilteredOptions() : pOptionData->GetNumOptions();
            int32_t itemCount = numOptions + pOptionData->m_numShownCategories;
            itemCount = Math::Min( itemCount , 20 );

            if ( itemCount <= 0 )
            {
                itemCount = 1;
            }

            return ( ( g.FontSize + g.Style.ItemSpacing.y ) * itemCount ) + ( g.Style.WindowPadding.y * 2 );
        };

        ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 1.0f );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 8.0f, 8.0f ) );
        ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, ImVec2( 8.0f, 8.0f ) );

        ImGui::SetNextWindowPos( windowStartPos );
        ImGui::SetNextWindowSize( ImVec2( inputWidth + requiredControlsWidth, CalcPopupHeightFromItemCount() ) );

        ImGuiWindowFlags popupWindowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNav;
        if ( ImGui::BeginPopupEx( inputTextID, popupWindowFlags ) )
        {
            bool const isPopupAppearing = ImGui::IsWindowAppearing();
            if ( isPopupAppearing )
            {
                pOptionData->OnOptionsWindowOpened();
            }

            int32_t const numOptions = pOptionData->GetNumFilteredOptions();

            ImGuiWindow* pPopupWindow = g.CurrentWindow;
            ImGui::BringWindowToDisplayFront( pPopupWindow );

            ImGuiID const cursorID = ImGui::GetID( "CursorIdx" );
            int32_t const prevCursorIdx = ImGui::GetStateStorage()->GetInt( cursorID, -1 );

            int32_t cursorIdx = isPopupAppearing ? -1 : prevCursorIdx;
            if ( cursorIdx != -1 && numOptions > 0 )
            {
                cursorIdx = cursorIdx % numOptions;
            }

            // Custom keyboard navigation
            //-------------------------------------------------------------------------

            bool rewriteBuffer = false;
            if ( ImGui::Shortcut( ImGuiKey_DownArrow, ImGuiInputFlags_Repeat, inputTextID ) && ( numOptions > 0 ) )
            {
                cursorIdx = ( cursorIdx + 1 ) % numOptions;
                rewriteBuffer = true;
            }

            if ( ImGui::Shortcut( ImGuiKey_UpArrow, ImGuiInputFlags_Repeat, inputTextID ) && ( numOptions > 0 ) )
            {
                if ( cursorIdx < 0 )
                {
                    cursorIdx = numOptions - 1;
                }
                else
                {
                    cursorIdx = ( cursorIdx - 1 + numOptions ) % numOptions;
                }
                rewriteBuffer = true;
            }

            if ( ImGui::Shortcut( ImGuiKey_PageUp, 0, inputTextID ) ) { /*Do nothing for now*/ }
            if ( ImGui::Shortcut( ImGuiKey_PageDown, 0, inputTextID ) ) { /*Do nothing for now*/ }

            if ( rewriteBuffer )
            {
                OptionData::Option const& option = pOptionData->m_filteredOptions[cursorIdx];
                EE_ASSERT( option.m_text.length() < bufferSize ); // Outer buffer size is too small, ensure the size can accommodate all options
                ImFormatString( pBuffer, bufferSize, "%s", option.m_text.c_str() );
                if ( pInputState != nullptr )
                {
                    pInputState->ReloadUserBufAndSelectAll();
                }
                else
                {
                    ImGui::ActivateItemByID( inputTextID );
                }
            }

            // Draw items
            //-------------------------------------------------------------------------

            for ( int32_t i = 0; i < numOptions; i++ )
            {
                OptionData::Option const& option = pOptionData->m_filteredOptions[i];

                if ( option.IsCategoryStart() && !option.m_category.empty() )
                {
                    ImGui::SeparatorText( option.m_category.c_str() );
                }

                //-------------------------------------------------------------------------

                ImGui::PushID( i );

                char const* const pItemName = option.m_text.c_str();
                ImVec2 const pItemPos = pPopupWindow->DC.CursorPos;

                if ( isPopupAppearing && StringUtils::Stricmp( pBuffer, pItemName ) == 0 )
                {
                    cursorIdx = i;
                    ImGui::ScrollToItem();
                }

                if ( ImGui::Selectable( pItemName, i == cursorIdx ) )
                {
                    ImGui::ClearActiveID();
                    ImFormatString( pBuffer, bufferSize, "%s", pItemName );
                    inputResult = true;
                    ImGui::CloseCurrentPopup();
                }

                if ( i == cursorIdx && cursorIdx != prevCursorIdx )
                {
                    ImGui::ScrollToItem();
                }

                ImGui::PopID();
            }

            //-------------------------------------------------------------------------

            // Close popup on deactivation (unless we are mouse-clicking in our popup)
            if ( ( !isInputActive && !ImGui::IsWindowFocused() ) )
            {
                ImGui::CloseCurrentPopup();
            }

            // Store cursor
            if ( cursorIdx != prevCursorIdx )
            {
                ImGui::GetStateStorage()->SetInt( cursorID, cursorIdx );
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar( 3 );

        ImGui::PopID();

        //-------------------------------------------------------------------------

        return inputResult;
    }

    //-------------------------------------------------------------------------

    bool ComboWithFilter( const char* pComboID, OptionData* pComboData, UUID& selectedOptionID, ImGuiComboFlags flags )
    {
        EE_ASSERT( pComboData != nullptr );

        if ( pComboData->m_optionsNeedRebuild )
        {
            // Getting a selected option ID will rebuild the option list, so if we need to rebuild we expect that the ID is unset
            // If the ID is set, it will no longer be valid when the list rebuilds! This is likely not what the user intended!
            // Please read the warning in the header about caching the selected object IDs
            EE_ASSERT( !selectedOptionID.IsValid() );
            pComboData->RebuildOptionsListFromProvider();
        }

        bool valueUpdated = false;

        // Calculate Width
        //-------------------------------------------------------------------------

        ImGuiContext& g = *GImGui;

        float totalWidgetWidth = 0;
        if ( g.NextItemData.HasFlags & ImGuiNextItemDataFlags_HasWidth )
        {
            totalWidgetWidth = g.NextItemData.Width;
        }

        float comboWidth = 0;
        if ( totalWidgetWidth > 0 )
        {
            comboWidth = Math::Max( totalWidgetWidth, 1.0f );
        }
        else if ( totalWidgetWidth == 0 )
        {
            comboWidth = Math::Max( ImGui::GetContentRegionAvail().x, 1.0f ); // Max allowable size
            comboWidth = Math::Min( comboWidth, 100.0f ); // arbitrary choice
        }
        else if ( totalWidgetWidth < 0 )
        {
            comboWidth = Math::Max( ImGui::GetContentRegionAvail().x, 1.0f );
        }

        // Calculate size of the drop down window
        //-------------------------------------------------------------------------

        float dropDownHeight = 0;

        if ( ( flags & ImGuiComboFlags_HeightLargest ) != 0 )
        {
            dropDownHeight = ImGui::GetFrameHeightWithSpacing() * 20;
        }
        else if ( ( flags & ImGuiComboFlags_HeightLarge ) != 0 )
        {
            dropDownHeight = ImGui::GetFrameHeightWithSpacing() * 12;
        }
        else if ( ( flags & ImGuiComboFlags_HeightSmall ) != 0 )
        {
            dropDownHeight = ImGui::GetFrameHeightWithSpacing() * 4;
        }
        else // Regular or nothing specified
        {
            dropDownHeight = ImGui::GetFrameHeightWithSpacing() * 8;
        }

        //-------------------------------------------------------------------------

        ImGui::PushID( pComboID );

        ImGuiComboFlags comboFlags = flags;
        InlineString previewValue;
        if ( pComboData->m_drawPreviewFunc != nullptr )
        {
            comboFlags |= ImGuiComboFlags_CustomPreview;
        }
        else if ( OptionData::Option const* pSelectedOption = pComboData->TryGetOption( selectedOptionID ) )
        {
            previewValue = pSelectedOption->m_text.c_str();
        }

        // Draw combo
        ImGui::SetNextItemWidth( comboWidth );
        ImGui::SetNextWindowSizeConstraints( ImVec2( comboWidth, 0 ), ImVec2( comboWidth, dropDownHeight ) );

        if ( pComboData->m_preWidgetFunc != nullptr )
        {
            pComboData->m_preWidgetFunc();
        }

        if ( ImGui::BeginCombo( "##Combo", previewValue.empty() ? nullptr : previewValue.c_str(), comboFlags ) )
        {
            bool const isPopupAppearing = ImGui::IsWindowAppearing();
            if ( isPopupAppearing )
            {
                pComboData->OnOptionsWindowOpened();
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::SetNextItemWidth( -1 );
            if ( ImGuiX::InputFilterText( pComboData->m_filterData ) )
            {
                pComboData->OnFilterUpdated();
            }

            //-------------------------------------------------------------------------

            int32_t const numOptions = (int32_t) pComboData->m_filteredOptions.size();
            for ( int32_t i = 0; i < numOptions; i++ )
            {
                OptionData::Option const& option = pComboData->m_filteredOptions[i];

                if ( option.IsCategoryStart() && !option.m_category.empty() )
                {
                    ImGui::SeparatorText( option.m_category.c_str() );
                }

                //-------------------------------------------------------------------------

                bool optionSelected = false;

                ImGui::PushID( i );

                if ( pComboData->m_drawFilteredOptionFunc != nullptr )
                {
                    optionSelected = pComboData->m_drawFilteredOptionFunc( pComboData, i );
                }
                else
                {
                    optionSelected = ImGui::MenuItem( option.m_text.c_str() );
                }

                ImGui::PopID();

                //-------------------------------------------------------------------------

                if ( optionSelected )
                {
                    selectedOptionID = option.m_ID;
                    valueUpdated = true;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndCombo();
        }

        if ( pComboData->m_postWidgetFunc != nullptr )
        {
            pComboData->m_postWidgetFunc();
        }

        // Custom Combo Preview
        //-------------------------------------------------------------------------

        if ( pComboData->m_drawPreviewFunc != nullptr && ( ( flags & ImGuiComboFlags_NoPreview ) == 0 ) )
        {
            if ( ImGui::BeginComboPreview() )
            {
                OptionData::Option const* pSelectedOption = pComboData->TryGetOption( selectedOptionID );
                pComboData->m_drawPreviewFunc( pComboData, pSelectedOption );
                ImGui::EndComboPreview();
            }
        }

        ImGui::PopID();

        //-------------------------------------------------------------------------

        return valueUpdated;
    }

    //-------------------------------------------------------------------------

    void OptionData::Refresh()
    {
        if ( m_optionProvider != nullptr )
        {
            RebuildOptionsListFromProvider();
        }
    }

    void OptionData::OnOptionsWindowOpened()
    {
        if ( m_optionProvider != nullptr )
        {
            RebuildOptionsListFromProvider();
        }
        else
        {
            ClearFilter();
        }
    }

    void OptionData::SetOptions( TVector<Option> const& options )
    {
        ClearOptionProvider();
        m_options = options;
        m_optionsNeedRebuild = false;
        ClearFilter();
    }

    void OptionData::SetOptions( TVector<Option>&& options )
    {
        ClearOptionProvider();
        m_options = eastl::move( options );
        m_optionsNeedRebuild = false;
        ClearFilter();
    }

    void OptionData::SetOptionProvider( TFunction<void( TVector<Option>& )>&& provider )
    {
        m_options.clear();
        m_optionProvider = eastl::move( provider );
        m_optionsNeedRebuild = true;
    }

    void OptionData::ClearOptionProvider()
    {
        if ( m_optionProvider != nullptr )
        {
            m_optionProvider = nullptr;
            m_options.clear();
            m_filteredOptions.clear();
            m_filterData.Clear();
        }
    }

    void OptionData::RebuildOptionsListFromProvider()
    {
        EE_ASSERT( m_optionProvider != nullptr );

        // Clear flag immediately to prevent infinite loops
        m_optionsNeedRebuild = false;

        // Clear options and request fresh ones
        m_options.clear();
        m_optionProvider( m_options );

        // Clear the filter and rebuild filtered list
        ClearFilter();
    }

    void OptionData::SetFilter( char const* pFilter )
    {
        m_filterData.SetFilter( pFilter );
        OnFilterUpdated();
    }

    void OptionData::ClearFilter()
    {
        m_filterData.Clear();
        OnFilterUpdated();
    }

    void OptionData::OnFilterUpdated()
    {
        m_filteredOptions.clear();
        m_numShownCategories = 0;

        if ( m_filterData.HasFilterSet() )
        {
            for ( auto const& opt : m_options )
            {
                if ( m_filterData.MatchesFilter( opt.m_text ) )
                {
                    m_filteredOptions.emplace_back( opt );
                }
            }
        }
        else // No filter so just copy everything
        {
            m_filteredOptions = m_options;
        }

        if ( m_filteredOptions.empty() )
        {
            return;
        }

        // Sort
        //-------------------------------------------------------------------------

        auto SortPredicate = [] ( Option const& lhs, Option const& rhs )
        {
            int32_t result = lhs.m_category.comparei( rhs.m_category );
            if ( result == 0 )
            {
                result = lhs.m_text.comparei( rhs.m_text );
            }

            return result < 0;
        };

        eastl::sort( m_filteredOptions.begin(), m_filteredOptions.end(), SortPredicate );

        // Set category starts
        //-------------------------------------------------------------------------

        String const* pCategoryString = &m_filteredOptions[0].m_category;

        for ( auto& opt : m_filteredOptions )
        {
            if ( pCategoryString->comparei( opt.m_category ) != 0 )
            {
                opt.m_isCategoryStart = true;
                pCategoryString = &opt.m_category;
                m_numShownCategories++;
            }
        }

        // Should we show the first category?
        if ( m_numShownCategories > 0 && !m_filteredOptions[0].m_category.empty() )
        {
            m_filteredOptions[0].m_isCategoryStart = true;
            m_numShownCategories++;
        }
    }

    OptionData::Option const* OptionData::TryGetOption( UUID const& optionID )
    {
        if ( m_optionsNeedRebuild )
        {
            RebuildOptionsListFromProvider();
        }

        //-------------------------------------------------------------------------

        for ( auto const& option : m_options )
        {
            if ( option.m_ID == optionID )
            {
                return &option;
            }
        }

        return nullptr;
    }

    OptionData::Option const* OptionData::TryGetOption( String const& textToMatch )
    {
        if ( m_optionsNeedRebuild )
        {
            RebuildOptionsListFromProvider();
        }

        //-------------------------------------------------------------------------

        for ( auto const& option : m_options )
        {
            if ( option.m_text.comparei( textToMatch ) == 0 )
            {
                return &option;
            }
        }

        return nullptr;
    }

    UUID OptionData::FindItemIDByText( String const& textToMatch )
    {
        if ( m_optionsNeedRebuild )
        {
            RebuildOptionsListFromProvider();
        }

        //-------------------------------------------------------------------------

        UUID foundID;

        if ( textToMatch.empty() )
        {
            return foundID;
        }

        for ( auto const& option : m_options )
        {
            if ( option.m_text.comparei( textToMatch ) == 0 )
            {
                foundID = option.m_ID;
                break;
            }
        }

        return foundID;
    }
}
#endif