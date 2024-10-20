# Coding Guidelines

**Rule of thumb:** When in doubt about the style make the code look like all the surrounding code!

## Filenames and Folder Structure

Filenames should all be named using CamelCase (https://en.wikipedia.org/wiki/Camel_case).

Filenames should be descriptive and ideally contain the domain/namespace that the file is part of:

```
InputState.cpp
AnimationSkeleton.cpp
RenderMesh.cpp
etc...
```

Please make use of folders/directories to separate out your files.

It is preferred to use an underscore (_) to create categories (still using CamelCase) when you have a lot of files that for a specific feature/system

```
GraphNode_Blend2D.cpp
GraphNode_RootMotionWarp.cpp
...
ParticleEmitter_Cluster.cpp
ParticleEmitter_Bounce.cpp
...
ResourceEditor_Skeleton.cpp
ResourceEditor_AnimationClip.cpp
```

## Braces, indentation and spacing

### Braces
Esoterica uses Allman style braces (https://en.wikipedia.org/wiki/Indentation_style#Allman_style)

The only exception for this is for empty functions or one line functions. These should be written as follows:

```cpp
Foo::SomeFunction()
{}

void DoSomething() {}
```

You must use braces for all conditional statements.

```cpp
if( x )
{
    continue;
}
```

**DO NOT** omit these braces like in the below example:

```cpp
// THIS IS WRONG
if( x )
    continue;
```

Pointers are aligned with the type and not the variable name

```cpp
int* pInt; // Correct

int *pInt; // Wrong!
int * pInt; // Wrong!
```

In the case of a const type, you will still align the \* with the type

```cpp
int const* pInt; // Correct

int const *pInt; // Wrong
int const * pInt; // Wrong
```

### Tabs/Spacing

Esoterica using spaces instead of tabs. This is so that code stays consistent no matter your editors tab length.

Class/Struct members must be aligned with one another:

```cpp
struct Foo
{
    int             m_a = 0.0f;
    TVector<bool>   m_b;
    float           m_c;
};
```

* There are no spaces between indices for arrays
```cpp
int f = array[3];
```

* There are spaces between braces:
```cpp
if( x ) ...

void Foo( args... )

int foo = ( bar + 5 ) - ( ( 6 * foobar ) + barfoo );
```

* There are no spaces for template specialization args

```cpp
TVector<float>
TBitFlags<BlendOptions>
```

* There are no spaces for c-style casts
```cpp
(Type*) pPtr;
```

* There are always spaces between operators

```cpp
int f = a + b; // Correct

int f = a+b; // Wrong
```

## Switch statements

Switch statement must have the case block indented.
Switch statement case blocks must use braces!
The break must be aligned to the closing brace.
There must not be a space between the case value and the colon

```cpp
switch( foo )
{
    case 0:
    {
        ...
    }
    break;

    case 1:
    {
        ...
    }
    break;

    deafult:
    {
        ...
    }
    break;
}
```

### Preprocessor Directives

All preprocessor directives need to be indented to match the surrounding code

```cpp
namespace EE::Foo
{
    void DoSomething
    {
        float var = 0.0f;

        #if SOME_PP_DIRECTIVE
        var += 5.0f;
        #endif

        var += 1.0f;
    }
}
```

**Exception**: Code within parts of EE::Render is allowed to not match this convention, until ClangFormat is fixed!

## Separators

Please use this separator string to break up logical code blocks (structures, namespaces, etc..)

```cpp
//-------------------------------------------------------------------------
```

Look at existing usage of this separator and following the same conventions

## Namespaces

Esoterica uses namespaces (EE)

Avoid putting things in the global EE namespace and prefer domains specific namespaces
* EE::Render
* EE::AI
* EE::Math
* etc...

Prefer using C++17 namespace syntax over the old syntax, so do this:

```cpp
// This is preferred
namespace EE::Render
{
    ...
}

// This is wrong
namespace EE
{
    namespace Render
    {
    }
}
```

It's okay to use the old format if you have multiple namespace in a single file:

```cpp
namespace EE
{
    namespace Render
    {
    }

    namespace Math
    {
    }
}
```

Prefer short and clear names for your types, this will make writing code within the namespace much simpler

Namespaces also allow us to have multiple things with the same name:
* EE\::Reflection::TypeInfo
* EE\::TypeSystem::TypeInfo

**AVOID** "using namespace" directives

## Naming Conventions

### Variables
Variables must use Camel Case (https://en.wikipedia.org/wiki/Camel_case).

```cpp
int someVariableName;
```

The following variable prefixes MUST be used:

* m_ for class/struct members
* s_ for class/struct static members
* g_ any compile unit global variables
* p for any *actual* pointer (do NOT use this prefix for smart ptrs e.g., uniquePtr or ResourcePtr )
* pp for pointers to pointers

**Exception:** Some math types avoid the 's_' for some special statically defined implementations and instead use the enum standard e.g. "Float3::Zero, Quaternion::Identity, TypeID::Invalid". This is generally only done in the core libs but it's an okay pattern for commonly referenced static members.

### Structure/Types

Structure/Types names must use Camel Caps (camel case but starting with a capital letter).

```cpp
class SomeClassName {};
struct SomeStructName {};
enum SomeEnumName {};
```

If a type is a templated type, it must start with a 'T' prefix

```cpp
template<typename T>
class TSomeClass {};
```

Structures must be laid out as follows:

* All functions must be before all members
* Static functions/members must be before all non-static functions and members
* Public functions/members must be first, followed by protected and finally private!
* **DO NOT** use multiple public/protected/private blocks per section!

```cpp
struct/class Foo
{
    public:

    // All public static functions

    protected:

    // All protected static functions

    private:

    // All private static functions

    //-------------------------------------------------------------------------

    public:

    // All public static members

    protected:

    // All protected static members

    private:

    // All private static members

    //-------------------------------------------------------------------------

    public:

    // All public functions

    protected:

    // All protected functions

    private:

    // All private functions

    //-------------------------------------------------------------------------

    public:

    // All public members

    protected:

    // All protected members

    private:

    // All private members
}
```

This is done so that we can immediately see all the data that a type contains and the order thereof. If you use multiple access specifier blocks for the data, the compiler is free to reorder those blocks as it wishes!

### Enums

Enum names must use Camel Caps (camel case but starting with a capital letter).

Enum members/items must be declared using Camel Caps. **DO NOT USE** all caps or snake case!

**Exception**: If you have a **REALLY** large enum (50+ items), then its OK to use an underscore (_) to separate enum values into categories (if it makes sense)

e.g.,

```cpp
enum class AIStates
{
    Ambient_Idle,
    Ambient_UseSmartObject,
    ...
    Combat_Idle,
    Combat_MoveToCover,
    ...
}
```

### Macros

Macros must be decalared in ALL CAPS and Snake Case (https://simple.wikipedia.org/wiki/Snake_case)

Avoid macros as much as possible, prefer const-expr/const-eval wherever possible!

## Enums

Prefer strongly type enums ( enum class Foo or enum struct Bar ).

Try to avoid raw flag enums. We have TBitFlags and BitFlags as utility classes when you need a set of bit flags.

## Const Correctness

Esoterica uses east const:

```cpp
int const var = 2.0f;
```

This is done so as to be consistent when we have const ptr/refs to const types:

```cpp
int const * const pPtr = &foo;
```

as opposed to

```cpp
const int * const pPtr = &foo;
```

Use const as much as possible, Esoterica is anally const correct.

To this point, the 'mutable' keyword is okay to use to explicitly declare intent.

## Comments
Comments must always start with a capital letter.

All code should be well commented. Core classes should have a comment per function explaining what it does.

```cpp
class Entity
{
    ...

    // Does this map contain this entity?
    // If you are working with an entity and a map, prefer to check the map ID vs the entity's map ID!
    inline bool ContainsEntity( EntityID entityID ) const { return FindEntity( entityID ) != nullptr; }

    // Adds a set of entities to this map - Transfers ownership of the entities to the map
    // Additionally allows you to offset all the entities via the supplied offset transform
    // Takes 1 frame to be fully added
    void AddEntities( TVector<Entity*> const& entities, Transform const& offsetTransform = Transform::Identity );

    //-------------------------------------------------------------------------
    // Entity API
    //-------------------------------------------------------------------------

    // Get the number of entities in this map
    inline int32_t GetNumEntities() const { return (int32_t) m_entities.size(); }

    // Get all entities in the map. This includes all entities that are still loading/unloading, etc...
    inline TVector<Entity*> const& GetEntities() const { return m_entities; }

    // Finds an entity via its ID. This uses a lookup map so is relatively performant
    inline Entity* FindEntity( EntityID entityID ) const;

    ...
};
```

## Reflection

The reflection macro (EE_REFLECT_XXX) should be the first thing in the class/struct/enum.

Note: For type reflection (EE_REFLECT_TYPE), this macro will declare a public access block, so make sure to specify the access immediately after this macro

```cpp
struct ReflectMe : public IReflectedType
{
    EE_REFLECT_TYPE( ReflectMe );

public:

    ReflectMe() = default;
    ...
};

enum ReflectedEnum
{
    EE_REFLECT_ENUM

    A,
    B,
    C,
    ...
};
```

For property/variable reflection, make sure the EE_REFLECT( ... ) macro is either on the same line if you have no options or on the line above the variable.

If the macro is placed on the line above the variable, make sure that there is a trailing semi-colon (;)

```cpp
struct ReflectMe : public IReflectedType
{
    EE_REFLECT_TYPE( ReflectMe );

private:

    EE_REFLECT() bool       m_tickMe = false;

    EE_REFLECT( ReadOnly );
    float                   m_toolsOnlyFloat = 0.0f;
};
```

## General guidelines

* Prefer 'using' over typedef
* Use constexpr/consteval whereever possible
* 'auto' is okay (use common sense for this)
* **DO NOT** creat global state and singletons whenever possible! Esoterica is heavily threaded, global state is looking for trouble.
* **DO NOT** create any circular dependencies in code **UNLESS ABSOLUTELY FUCKING NECESSARY**!
* Prefer using early outs vs heavily nested code

```cpp
// This is preferred
void Foo()
{
    ...

    if( x )
    {
        return;
    }

    ...

    if( y )
    {
        return;
    }
}

// This is wrong.
void Foo()
{
    ...

    if( !x )
    {
        ...

        if( !y )
        {
            ...
        }
    }
}
```

* Prefer c++ casts (reinterpret, static, const) and generally avoid c-style casts! For static casting, this adds a level of typesafety that you dont have with c-style casts.

```cpp
Foo* pFoo = static_cast<Foo*>( pBar );
Foo* pFoo = reinterpret_cast<Foo*>( pBar );
```

* Try to avoid typedef'ing/using types with new names for convienience unless it's really a big positive for the API or you foresee the type potentially changing over time:

```
// For example this is probably justifiable since the allocated storage might need to be changed and you dont want to change all the code for each change 
using QueryResults = TInlineVector<SomeType, 100>;

// This on the other hand is just laziness
using QueryResults = TVector<SomeType>;
```