#pragma once
#include "EngineTools/_Module/API.h"
#include "System/TypeSystem/RegisteredType.h"
#include "System/Types/Arrays.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EE_ENGINETOOLS_API IUndoableAction : public IRegisteredType
    {
        friend class UndoStack;

        EE_REGISTER_TYPE( IUndoableAction );

    public:

        virtual ~IUndoableAction() = default;

    protected:

        virtual void Undo() = 0;
        virtual void Redo() = 0;
    };

    //-------------------------------------------------------------------------

    class EE_ENGINETOOLS_API UndoStack
    {
    public:

        enum class Operation : uint8_t
        {
            Undo,
            Redo
        };

    public:

        ~UndoStack();

        // Do we have an action to undo
        inline bool CanUndo() { return !m_recordedActions.empty(); }

        // Undo the last action - returns the action that we undid
        IUndoableAction const* Undo();

        // Do we have an action we can redo
        inline bool CanRedo() { return !m_undoneActions.empty(); }

        // Redoes the last action - returns the action that we redid
        IUndoableAction const* Redo();

        //-------------------------------------------------------------------------

        // Register a new action, this transfers ownership of the action memory to the stack
        void RegisterAction( IUndoableAction* pAction );

    private:

        void ClearUndoStack();
        void ClearRedoStack();

    private:

        TVector<IUndoableAction*>    m_recordedActions;
        TVector<IUndoableAction*>    m_undoneActions;
    };
}