#pragma once
#include <glm/glm.hpp>

// Which way a thing hung on a wall looks, as a vector out of its
// face. Shared by the signs and the item frames, because the two hang
// the same way and the words and the item have to stand off the board
// by the same amount or one of them ends up inside the wall.
//
// It is here rather than beside the block ids because Block.h keeps
// glm out on purpose: it is included almost everywhere, the audio
// engine among them, and that file pulls the vendored stb_vorbis in
// ahead of it. The two cannot see each other without the build coming
// apart, which is what the note in Block.h has been warning about
// since long before this.
inline glm::vec3 wallOut(int facing)
{
    switch (facing)
    {
        case 0:  return glm::vec3(0.0f, 0.0f, 1.0f);
        case 1:  return glm::vec3(-1.0f, 0.0f, 0.0f);
        case 2:  return glm::vec3(0.0f, 0.0f, -1.0f);
        default: return glm::vec3(1.0f, 0.0f, 0.0f);
    }
}
