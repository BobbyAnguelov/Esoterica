#include "UndoStack.h"

//-------------------------------------------------------------------------

namespace EE
{
    UndoStack::~UndoStack()
    {
        EE_ASSERT( m_pCompoundStackAction == nullptr );
        ClearRedoStack();
        ClearUndoStack();
    }

    void UndoStack::Reset()
    {
        EE_ASSERT( m_pCompoundStackAction == nullptr );
        ClearRedoStack();
        ClearUndoStack();
    }

    //-------------------------------------------------------------------------

    void UndoStack::ExecuteUndo( IUndoableAction* pAction ) const
    {
        // Notify listeners
        m_preUndoRedoEvent.Execute( Operation::Redo, pAction );

        // Undo the action and place it on the undone stack
        pAction->Undo();

        // Notify listeners
        m_postUndoRedoEvent.Execute( Operation::Undo, pAction );
        m_actionPerformed.Execute();
    }

    IUndoableAction const* UndoStack::Undo()
    {
        EE_ASSERT( CanUndo() );
        EE_ASSERT( m_pCompoundStackAction == nullptr );

        // Pop action off the recorded stack
        IUndoableAction* pAction = m_recordedActions.back();
        m_recordedActions.pop_back();

        if ( auto pCompoundAction = TryCast<CompoundStackAction>( pAction ) )
        {
            // Execute undo actions in reverse
            for ( int32_t i = (int32_t) pCompoundAction->m_actions.size() - 1; i >= 0; i-- )
            {
                ExecuteUndo( pCompoundAction->m_actions[i] );
            }
        }
        else
        {
            ExecuteUndo( pAction );
        }

        m_undoneActions.emplace_back( pAction );
        return pAction;
    }

    void UndoStack::ExecuteRedo( IUndoableAction* pAction ) const
    {
        // Notify listeners
        m_preUndoRedoEvent.Execute( Operation::Redo, pAction );

        // Undo the action and place it on the undone stack
        pAction->Redo();

        // Notify listeners
        m_postUndoRedoEvent.Execute( Operation::Redo, pAction );
        m_actionPerformed.Execute();
    }

    IUndoableAction const* UndoStack::Redo()
    {
        EE_ASSERT( CanRedo() );
        EE_ASSERT( m_pCompoundStackAction == nullptr );

        // Pop action off the recorded stack
        IUndoableAction* pAction = m_undoneActions.back();
        m_undoneActions.pop_back();

        if ( auto pCompoundAction = TryCast<CompoundStackAction>( pAction ) )
        {
            for ( auto pSubAction : pCompoundAction->m_actions )
            {
                ExecuteRedo( pSubAction );
            }
        }
        else
        {
            ExecuteRedo( pAction );
        }

        m_recordedActions.emplace_back( pAction );
        return pAction;
    }

    //-------------------------------------------------------------------------

    void UndoStack::RegisterAction( IUndoableAction* pAction )
    {
        EE_ASSERT( pAction != nullptr );

        if ( m_pCompoundStackAction != nullptr )
        {
            m_pCompoundStackAction->AddToStack( pAction );
        }
        else
        {
            m_recordedActions.emplace_back( pAction );
            ClearRedoStack();
            m_actionPerformed.Execute();
        }
    }

    void UndoStack::BeginCompoundAction()
    {
        EE_ASSERT( m_pCompoundStackAction == nullptr );
        m_pCompoundStackAction = EE::New<CompoundStackAction>();
    }

    void UndoStack::EndCompoundAction()
    {
        EE_ASSERT( m_pCompoundStackAction != nullptr );
        m_recordedActions.emplace_back( m_pCompoundStackAction );
        m_pCompoundStackAction = nullptr;

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