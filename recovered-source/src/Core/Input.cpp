#include "Input.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include <cstdio>

void Input::beginFrame()
{
    m_keysPressedThisFrame.clear();
    m_mousePressedThisFrame.clear();
    m_anyKey = SDL_SCANCODE_UNKNOWN;
    m_anyMouseButton = 0;
    m_typedText.clear();
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;
    m_wheelDelta = 0;
    m_resized = false;
}

void Input::startTextEntry() { SDL_StartTextInput(); }
void Input::stopTextEntry() { SDL_StopTextInput(); }

void Input::processEvent(const SDL_Event& e)
{
    if (e.type == SDL_KEYDOWN && e.key.repeat == 0)
        m_anyKey = e.key.keysym.scancode;
    else if (e.type == SDL_MOUSEBUTTONDOWN)
        m_anyMouseButton = e.button.button;

    switch (e.type)
    {
        case SDL_QUIT:
            m_quitRequested = true;
            break;

        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_RESIZED ||
                e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                m_resized = true;
                m_newWidth = e.window.data1;
                m_newHeight = e.window.data2;
            }
            break;

        case SDL_KEYDOWN:
            if (e.key.repeat == 0)
            {
                m_keysDown[e.key.keysym.scancode] = true;
                m_keysPressedThisFrame[e.key.keysym.scancode] = true;
            }
            break;

        case SDL_KEYUP:
            m_keysDown[e.key.keysym.scancode] = false;
            break;

        case SDL_TEXTINPUT:
            m_typedText += e.text.text;
            break;

        case SDL_MOUSEMOTION:
            m_mouseDeltaX += static_cast<float>(e.motion.xrel);
            m_mouseDeltaY += static_cast<float>(e.motion.yrel);
            m_mouseX = e.motion.x;
            m_mouseY = e.motion.y;
            break;

        case SDL_MOUSEBUTTONDOWN:
            m_mouseDown[e.button.button] = true;
            m_mousePressedThisFrame[e.button.button] = true;
            m_mouseX = e.button.x;
            m_mouseY = e.button.y;
            break;

        case SDL_MOUSEBUTTONUP:
            m_mouseDown[e.button.button] = false;
            m_mouseX = e.button.x;
            m_mouseY = e.button.y;
            break;

        case SDL_MOUSEWHEEL:
            m_wheelDelta += e.wheel.y;
            break;

        default:
            break;
    }
}

static bool lookup(const std::unordered_map<int, bool>& map, int key)
{
    auto it = map.find(key);
    return it != map.end() && it->second;
}

bool Input::isKeyDown(SDL_Scancode key) const { return lookup(m_keysDown, key); }
bool Input::wasKeyPressed(SDL_Scancode key) const { return lookup(m_keysPressedThisFrame, key); }
bool Input::isMouseButtonDown(Uint8 button) const { return lookup(m_mouseDown, button); }
bool Input::wasMouseButtonPressed(Uint8 button) const { return lookup(m_mousePressedThisFrame, button); }


#ifdef __EMSCRIPTEN__

// SDL2's Emscripten backend reports an absolute mouse position it builds up
// from relative motion starting at (0,0), so it is wrong by however far the
// pointer was from the origin when the first event arrived. The browser knows
// the real position, so ask it, scaling for any gap between the canvas's CSS
// size and its drawing buffer.
namespace
{
    void hookBrowserPointer()
    {
        static bool hooked = false;
        if (hooked) return;
        hooked = true;

        EM_ASM({
            if (window.__bcPointerHooked) return;
            window.__bcPointerHooked = 1;
            window.__bcMouseX = 0;
            window.__bcMouseY = 0;
            var read = function (e) {
                var c = (typeof Module !== 'undefined' && Module.canvas)
                        ? Module.canvas : document.querySelector('canvas');
                if (!c) return;
                var r = c.getBoundingClientRect();
                if (!r.width || !r.height) return;
                window.__bcMouseX = (e.clientX - r.left) * (c.width / r.width);
                window.__bcMouseY = (e.clientY - r.top) * (c.height / r.height);
            };
            window.addEventListener('mousemove', read, true);
            window.addEventListener('mousedown', read, true);
        });
    }
}

int Input::mouseX() const
{
    hookBrowserPointer();
    return EM_ASM_INT({ return window.__bcMouseX | 0; });
}

int Input::mouseY() const
{
    hookBrowserPointer();
    return EM_ASM_INT({ return window.__bcMouseY | 0; });
}

#else

int Input::mouseX() const { return m_mouseX; }
int Input::mouseY() const { return m_mouseY; }

#endif
