#pragma once

#include "ImguiTextBuffer.h"
#include "ImguiFilter.h"
#include "Base/ThirdParty/imgui/imgui.h"
#include "Base/Types/Function.h"
#include "Base/Types/UUID.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    // Draw an input text field that show some help-text when empty and that has a clear button
    // Help text is drawn when the input buffer is empty
    EE_BASE_API bool InputTextWithClearButton( const char* pInputTextID, char const* pHelpText, char* pBuffer, size_t bufferSize, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback pInputTextCallback = nullptr, void* pUserData = nullptr );

    //-------------------------------------------------------------------------

    // Combo with a preset set of options.
    // The currently selected option is identified by the selected option ID passed into the function
    // If the user selects a new value from the combo, it will return true and the selected option ID will be updated
    // WARNING: the OptionID is not stable so you cannot rely on it between updates! Do not cache this and rely on it
    EE_BASE_API bool ComboWithFilter( const char* pComboID, class OptionData* pOptionData, UUID& selectedOptionID, ImGuiComboFlags flags = 0 );

    //-------------------------------------------------------------------------

    EE_BASE_API bool InputTextWithOptions( const char* pInputTextID, char* pBuffer, size_t bufferSize, class OptionData* pOptionData, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback pInputTextCallback = nullptr, void* pUserData = nullptr );

    //-------------------------------------------------------------------------

    class EE_BASE_API OptionData
    {
        friend EE_BASE_API bool ComboWithFilter( const char*, OptionData*, UUID& selectedItemID, ImGuiComboFlags );
        friend EE_BASE_API bool InputTextWithOptions( const char*, char*, size_t, OptionData*, ImGuiInputTextFlags, ImGuiInputTextCallback, void* );

    public:

        struct Option
        {
            friend OptionData;

        public:

            Option() = default;
            Option( String const& text, String const& category = String(), uint64_t userData = 0 ) : m_ID( UUID::GenerateID() ), m_text( text ), m_category( category ), m_userData( userData ) {}
            Option( char const* pText, char const* pCategory = nullptr, uint64_t userData = 0 ) : m_ID( UUID::GenerateID() ), m_text( pText ? pText : "" ), m_category( pCategory ? pCategory : "" ), m_userData( userData ) {}
            Option( String const& text, uint64_t userData ) : m_ID( UUID::GenerateID() ), m_text( text ), m_userData( userData ) {}
            Option( char const* pText, uint64_t userData ) : m_ID( UUID::GenerateID() ), m_text( pText ? pText : "" ), m_userData( userData ) {}

            inline bool IsValid() const { return m_ID.IsValid(); }
            inline bool IsCategoryStart() const { return m_isCategoryStart; }

            inline bool operator<( Option const& rhs ) const { return m_text.comparei( rhs.m_text ) < 0; }

        public:

            UUID            m_ID = UUID::GenerateID();
            String          m_text;
            String          m_category;
            uint64_t        m_userData = 0;             // Can be used to store an index into some other payload storage or a ptr

        private:

            bool            m_isCategoryStart = false;  // Is this the first option in a category
        };

    public:

        OptionData() = default;

        // Options
        //-------------------------------------------------------------------------

        // Refresh all options, only really makes sense if you have an option provider
        void Refresh();

        // Set explicit set of options, will clear any set option provider
        void SetOptions( TVector<Option> const& options );

        // Set explicit set of options, will clear any set option provider
        void SetOptions( TVector<Option>&& options );

        // Set an option provider, will clear any set options
        void SetOptionProvider( TFunction<void( TVector<Option>& )>&& provider );

        // Clear the option provider
        void ClearOptionProvider();

        // Get the number of options
        EE_FORCE_INLINE int32_t GetNumOptions() const { return (int32_t) m_options.size(); }

        // Get the number of filtered options
        EE_FORCE_INLINE int32_t GetNumFilteredOptions() const { return (int32_t) m_filteredOptions.size(); }

        // Try to find an item via UUID
        Option const* TryGetOption( UUID const& optionID );

        // Try to find an item via UUID
        Option const* TryGetOption( String const& textToMatch );

        // Try to find an item via text
        UUID FindItemIDByText( String const& textToMatch );

        // Filtering
        //-------------------------------------------------------------------------

        void SetFilter( char const* pFilter );

        void ClearFilter();

    protected:

        void OnOptionsWindowOpened();

        void RebuildOptionsListFromProvider();

        void OnFilterUpdated();

    public:

        FilterData                                              m_filterData;
        TVector<Option>                                         m_options;
        TVector<Option>                                         m_filteredOptions;
        TFunction<void()>                                       m_preWidgetFunc;            // Called immediately before the Imgui::Input/Combo function call so you can set additional styles/etc...
        TFunction<void()>                                       m_postWidgetFunc;           // Called immediately after the Imgui::Input/Combo function so you can query item state, etc...
        TFunction<bool( OptionData const*, int )>               m_drawFilteredOptionFunc;   // Custom draw function for options, returns true if an option is picked, the index passed in is for the filtered list!
        TFunction<void( OptionData const*, Option const* )>     m_drawPreviewFunc;          // Custom draw function for a combo option preview. Args = ( OptionData const* pOptionList, Option const* pSelectedOption )

    protected:

        TFunction<void( TVector<Option>& )>                     m_optionProvider;           // This (OPTIONAL) function will be used to fill the options list each time the combo is opened
        bool                                                    m_optionsNeedRebuild = false;
        int32_t                                                 m_numShownCategories = 0;
    };
}
#endif