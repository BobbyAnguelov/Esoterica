#include "GameInput.h"

//-------------------------------------------------------------------------

namespace EE::Input
{
    void GameInputMap::UnregisterInputs()
    {
        m_axes.clear();
        m_values.clear();
        m_signals.clear();
    }
}