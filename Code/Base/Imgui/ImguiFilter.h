#pragma once
#include "ImguiX.h"
#include "ImguiTextBuffer.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    EE_BASE_API bool InputFilterText( class FilterData& filterData, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback pInputTextCallback = nullptr, void* pUserData = nullptr );

    //-------------------------------------------------------------------------

    // Helper that maintains all the state needed for filtering
    class EE_BASE_API FilterData
    {
        friend EE_BASE_API bool InputFilterText( FilterData&, ImGuiInputTextFlags, ImGuiInputTextCallback, void* );

    public:

        // Manually set the filter buffer
        void SetFilter( char const* pFilterText );

        // Manually set the filter buffer
        void SetFilter( String const& filterText );

        // Set the help text shown when we dont have focus and the filter is empty
        void SetFilterHelpText( String const& helpText ) { m_filterHelpText = helpText; }

        // Get the filter help text
        char const* GetFilterHelpText() { return m_filterHelpText.c_str(); }

        // Clear the filter
        inline void Clear();

        // Do we have a filter set?
        inline bool HasFilterSet() const { return !m_tokens.empty(); }

        // Get the split filter text token
        inline TVector<String> const& GetFilterTokens() const { return m_tokens; }

        // Does a provided string match the current filter - the string copy is intentional!
        bool MatchesFilter( String string );

        // Does a provided string match the current filter - the string copy is intentional!
        bool MatchesFilter( InlineString string );

        // Does a provided string match the current filter
        bool MatchesFilter( char const* pString ) { return MatchesFilter( InlineString( pString ) ); }

    private:

        void GenerateTokens();

    private:

        ImGuiX::TextBuffer  m_buffer;
        TVector<String>     m_tokens;
        String              m_filterHelpText = "Filter...";
    };
}
#endif