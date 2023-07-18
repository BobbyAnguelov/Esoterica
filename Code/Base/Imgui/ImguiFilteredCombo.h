#pragma once

#include "ImguiX.h"

//-------------------------------------------------------------------------

namespace EE::ImGuiX
{
    template<typename T>
    class ComboWithFilterWidget
    {
    public:

        constexpr static float const s_defaultComboDropDownWidth = 450;

        enum Flags : uint8_t
        {
            HidePreview = 0
        };

        struct Option
        {
            String  m_label;
            String  m_filterComparator; // lowercase version of the label
            T       m_value;
        };

    public:

        ComboWithFilterWidget( TBitFlags<Flags> flags = TBitFlags<Flags>() ) : m_flags( flags ) {}
        virtual ~ComboWithFilterWidget() = default;

        // Draws the filter. Returns true if the filter has been updated
        bool DrawAndUpdate( float width = -1, float minDropDownWidth = -1, float dropDownHeightInItems = 20 );

        // Do we have something selected?
        inline bool HasValidSelection() const { return m_selectedIdx != InvalidIndex; }

        // Get the currently selected option
        Option const* GetSelectedOption() const { return ( m_selectedIdx == InvalidIndex ) ? nullptr : &m_filteredOptions[m_selectedIdx]; }

        // User provided set function
        virtual void SetSelectedOption( T const& selectedItem );

    protected:

        // Create the displayable options for the combo
        inline void GenerateOptions()
        {
            m_options.clear();
            m_filteredOptions.clear();
            PopulateOptionsList();
            ApplyFilterToOptions();
        };

        // User overridable draw function for the preview of the combo
        virtual void DrawPreview() const;

        // User overridable draw function for the options in the combo list - returns true if selected
        virtual bool DrawOption( Option const& option ) const;

        // User overridable draw function to fill the options list
        virtual void PopulateOptionsList() {};

    private:

        // Apply the filter to the options list
        void ApplyFilterToOptions();

    protected:

        FilterWidget        m_filterWidget;
        int32_t             m_selectedIdx = InvalidIndex;
        TVector<Option>     m_options;
        TVector<Option>     m_filteredOptions;
        TBitFlags<Flags>    m_flags = TBitFlags<Flags>();
        bool                m_isComboOpen = false;
    };

    //-------------------------------------------------------------------------

    template<typename T>
    bool ComboWithFilterWidget<T>::DrawAndUpdate( float width, float minDropDownWidth, float dropDownHeightInItems )
    {
        bool valueUpdated = false;

        //-------------------------------------------------------------------------

        ImGui::PushID( this );
        ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2( 0, 0 ) );
        if ( ImGui::BeginChild( "FilterLayout", ImVec2( width, ImGui::GetFrameHeight() ), false, ImGuiWindowFlags_AlwaysUseWindowPadding | ImGuiWindowFlags_NoScrollbar ) )
        {
            bool const hidePreviewSection = m_flags.IsFlagSet( Flags::HidePreview );
            uint32_t const comboFlags = ImGuiComboFlags_HeightLarge | ImGuiComboFlags_PopupAlignLeft | ( hidePreviewSection ? ImGuiComboFlags_NoPreview : ImGuiComboFlags_CustomPreview );

            // Calculate size of the drop down window
            ImVec2 comboDropDownSize( width, ImGui::GetFrameHeightWithSpacing() * dropDownHeightInItems );
            if ( hidePreviewSection )
            {
                comboDropDownSize.x = Math::Max( width, s_defaultComboDropDownWidth );
            }
            else
            {
                if ( comboDropDownSize.x <= 0 )
                {
                    comboDropDownSize.x = ImGui::GetContentRegionAvail().x;
                }
                else
                {
                    comboDropDownSize.x = Math::Max( width, s_defaultComboDropDownWidth );
                }
            }

            // Draw combo
            ImGui::SetNextItemWidth( -1 );
            ImGui::SetNextWindowSizeConstraints( ImVec2( comboDropDownSize.x, 0 ), comboDropDownSize );

            bool const wasComboOpen = m_isComboOpen;
            m_isComboOpen = ImGui::BeginCombo( "##Combo", "", comboFlags );

            // Regenerate options whenever we open the combo
            if ( !wasComboOpen && m_isComboOpen )
            {
                GenerateOptions();
            }

            // Draw combo if open
            if ( m_isComboOpen )
            {
                if ( m_filterWidget.DrawAndUpdate( -1, ImGuiX::FilterWidget::Flags::TakeInitialFocus ) )
                {
                    ApplyFilterToOptions();
                }

                //-------------------------------------------------------------------------

                for ( int32_t i = 0; i < m_filteredOptions.size(); i++ )
                {
                    if ( DrawOption( m_filteredOptions[i] ) )
                    {
                        m_selectedIdx = i;
                        valueUpdated = true;
                        ImGui::CloseCurrentPopup();
                    }
                }

                ImGui::EndCombo();
            }

            // Combo Preview
            if ( !m_flags.IsFlagSet( Flags::HidePreview ) )
            {
                if ( ImGui::BeginComboPreview() )
                {
                    DrawPreview();
                    ImGui::EndComboPreview();
                }
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar( 1 );
        ImGui::PopID();

        //-------------------------------------------------------------------------

        return valueUpdated;
    }

    template<typename T>
    void EE::ImGuiX::ComboWithFilterWidget<T>::DrawPreview() const
    {
        ImGui::Text( ( m_selectedIdx == InvalidIndex ) ? "" : m_filteredOptions[m_selectedIdx].m_label.c_str() );
    }

    template<typename T>
    bool EE::ImGuiX::ComboWithFilterWidget<T>::DrawOption( Option const& option ) const
    {
        return ImGui::MenuItem( option.m_label.c_str() );
    }

    template<typename T>
    void ComboWithFilterWidget<T>::ApplyFilterToOptions()
    {
        if ( m_filterWidget.HasFilterSet() )
        {
            m_filteredOptions.clear();

            for ( auto const& option : m_options )
            {
                if ( m_filterWidget.MatchesFilter( option.m_filterComparator ) )
                {
                    m_filteredOptions.emplace_back( option );
                }
            }
        }
        else
        {
            m_filteredOptions = m_options;
        }
    }

    template<typename T>
    void EE::ImGuiX::ComboWithFilterWidget<T>::SetSelectedOption( T const& selectedItem )
    {
        m_selectedIdx = InvalidIndex;
        GenerateOptions();

        if ( selectedItem.IsValid() )
        {
            for ( auto i = 0u; i < m_filteredOptions.size(); i++ )
            {
                if ( m_filteredOptions[i].m_value == selectedItem )
                {
                    m_selectedIdx = i;
                    break;
                }
            }
        }
    }
}