#pragma once

//-------------------------------------------------------------------------

namespace EE
{
    enum class ResizeHandle
    {
        None,
        N,
        NW,
        W,
        SW,
        S,
        SE,
        E,
        NE
    };

    struct DragAndDrop
    {
        constexpr static char const* const s_filePayloadID = "ResourceFile";
    };
}