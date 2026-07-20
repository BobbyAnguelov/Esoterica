#pragma once
#include "Base/Types/Function.h"
#include "Base/Types/Arrays.h"

//-------------------------------------------------------------------------

#if EE_DEVELOPMENT_TOOLS
namespace EE::ImGuiX
{
    class CommandStack
    {
    public:

        inline bool HasCommands() const
        {
            return !m_commands.empty();
        }

        void PushCommand( TFunction<void()>&& command )
        {
            m_commands.emplace_back( eastl::move( command ) );
        }

        void PushCommand( TFunction<void()> const& command )
        {
            m_commands.emplace_back( command );
        }

        void ExecuteCommands()
        {
            for ( auto& cmd : m_commands )
            {
                cmd();
            }
            m_commands.clear();
        }

        void Clear()
        {
            m_commands.clear();
        }

    private:

        TVector<TFunction<void()>> m_commands;
    };

    //-------------------------------------------------------------------------

    class [[nodiscard]] ScopedCommandStack : public CommandStack
    {
    public:

        ScopedCommandStack() = default;

        ~ScopedCommandStack()
        {
            ExecuteCommands();
        }
    };
}
#endif