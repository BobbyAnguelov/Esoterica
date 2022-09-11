#include "UndoStack.h"

//-------------------------------------------------------------------------

namespace EE
{
    UndoStack::~UndoStack()
    {
        ClearRedoStack();
        ClearUndoStack();
    }

    void UndoStack::Reset()
    {
        ClearRedoStack();
        ClearUndoStack();
    }

    //-------------------------------------------------------------------------

    IUndoableAction const* UndoStack::Undo()
    {
        EE_ASSERT( CanUndo() );

        // Pop action off the recorded stack
        IUndoableAction* pAction = m_recordedActions.back();
        m_recordedActions.pop_back();

        // Notify listeners
        m_preUndoRedoEvent.Execute( Operation::Redo, pAction );

        // Undo the action and place it on the undone stack
        pAction->Undo();
        m_undoneActions.emplace_back( pAction );

        // Notify listeners
        m_postUndoRedoEvent.Execute( Operation::Undo, pAction );
        m_actionPerformed.Execute();

        return pAction;
    }

    IUndoableAction const* UndoStack::Redo()
    {
        EE_ASSERT( CanRedo() );

        // Pop action off the recorded stack
        IUndoableAction* pAction = m_undoneActions.back();
        m_undoneActions.pop_back();

        // Notify listeners
        m_preUndoRedoEvent.Execute( Operation::Redo, pAction );

        // Undo the action and place it on the undone stack
        pAction->Redo();
        m_recordedActions.emplace_back( pAction );

        // Notify listeners
        m_postUndoRedoEvent.Execute( Operation::Redo, pAction );
        m_actionPerformed.Execute();

        return pAction;
    }

    //-------------------------------------------------------------------------

    void UndoStack::RegisterAction( IUndoableAction* pAction )
    {
        EE_ASSERT( pAction != nullptr );
        m_recordedActions.emplace_back( pAction );
        ClearRedoStack();

        m_actionPerformed.Execute();
    }

    void UndoStack::ClearUndoStack()
    {
        for ( auto& pAction : m_recordedActions )
        {
            EE::Delete( pAction );
        }

        m_recordedActions.clear();
    }

    void UndoStack::ClearRedoStack()
    {
        for ( auto& pAction : m_undoneActions )
        {
            EE::Delete( pAction );
        }

        m_undoneActions.clear();
    }

}