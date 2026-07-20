#include "GameInput.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    void GameInputState::UnregisterInputs()
    {
        m_axes.clear();
        m_values.clear();
        m_signals.clear();
    }
}