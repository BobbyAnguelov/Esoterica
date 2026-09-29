#include "ImguiFilter.h"
#include "ImguiInputs.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    void FilterData::Clear()
    {
        m_buffer.Clear();
        m_tokens.clear();
    }

    bool FilterData::MatchesFilter( String string )
    {
        if ( string.empty() )
        {
            return false;
        }

        if ( m_tokens.empty() )
        {
            return true;
        }

        //-------------------------------------------------------------------------

        string.make_lower();
        for ( auto const& token : m_tokens )
        {
            if ( string.find( token ) == String::npos )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    bool FilterData::MatchesFilter( InlineString string )
    {
        if ( string.empty() )
        {
            return false;
        }

        if ( m_tokens.empty() )
        {
            return true;
        }

        //-------------------------------------------------------------------------

        string.make_lower();
        for ( auto const& token : m_tokens )
        {
            if ( string.find( token.c_str() ) == InlineString::npos )
            {
                return false;
            }
        }

        //-------------------------------------------------------------------------

        return true;
    }

    void FilterData::SetFilter( char const* pFilterText )
    {
        if ( pFilterText == nullptr )
        {
            Clear();
        }
        else
        {
            m_buffer.Fill( pFilterText );
            GenerateTokens();
        }
    }

    void FilterData::SetFilter( String const& filterText )
    {
        m_buffer.Fill( filterText );
        GenerateTokens();
    }

    void FilterData::GenerateTokens()
    {
        StringUtils::Split( m_buffer.GetString(), m_tokens );

        for ( auto& token : m_tokens )
        {
            token.make_lower();
        }
    }

    //-------------------------------------------------------------------------

    bool InputFilterText( FilterData& filterData, ImGuiInputTextFlags flags, ImGuiInputTextCallback pInputTextCallback, void* pUserData )
    {
        ImGui::PushID( &filterData );
        bool const filterUpdated = InputTextWithClearButton( "##Filter", filterData.GetFilterHelpText(), filterData.m_buffer.Data(), filterData.m_buffer.Size(), flags, pInputTextCallback, pUserData );
        ImGui::PopID();

        if( filterUpdated )
        {
            filterData.GenerateTokens();
        }

        return filterUpdated;
    }
}
#endif