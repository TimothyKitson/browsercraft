#include "Input.h"

void Input::beginFrame()
{
    m_keysPressedThisFrame.clear();
    m_mousePressedThisFrame.clear();
    m_anyKey = SDL_SCANCODE_UNKNOWN;
    m_anyMouseButton = 0;
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;
    m_wheelDelta = 0;
    m_resized = false;
}

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
