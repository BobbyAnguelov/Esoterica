#pragma once

//-------------------------------------------------------------------------

namespace EE
{
    template<typename T>
    class [[nodiscard]] TScopedGuardValue
    {
    public:

        TScopedGuardValue( T& variable, T const& newValue )
            : m_variable( variable )
            , m_originalValue( variable )

        {
            variable = newValue;
        }

        ~TScopedGuardValue()
        {
            m_variable = m_originalValue;
        }

    private:

        TScopedGuardValue( TScopedGuardValue const& ) = delete;
        TScopedGuardValue( TScopedGuardValue const&& ) = delete;
        TScopedGuardValue& operator=( TScopedGuardValue const& ) = delete;
        TScopedGuardValue& operator=( TScopedGuardValue const&& ) = delete;

    private:

        T&  m_variable;
        T   m_originalValue;
    };
}