#pragma once
#include <SDL.h>
#include <unordered_map>

// Keyboard + mouse state for one frame.
//   input.beginFrame();
//   while (SDL_PollEvent(&e)) input.processEvent(e);
class Input
{
public:
    void beginFrame();
    void processEvent(const SDL_Event& e);

    bool isKeyDown(SDL_Scancode key) const;
    bool wasKeyPressed(SDL_Scancode key) const;

    bool isMouseButtonDown(Uint8 button) const;
    bool wasMouseButtonPressed(Uint8 button) const;

    float mouseDeltaX() const { return m_mouseDeltaX; }
    float mouseDeltaY() const { return m_mouseDeltaY; }
    int mouseX() const { return m_mouseX; }
    int mouseY() const { return m_mouseY; }
    int wheelDelta() const { return m_wheelDelta; }

    bool quitRequested() const { return m_quitRequested; }
    void requestQuit() { m_quitRequested = true; }

    bool resized() const { return m_resized; }
    int newWidth() const { return m_newWidth; }
    int newHeight() const { return m_newHeight; }

private:
    std::unordered_map<int, bool> m_keysDown;
    std::unordered_map<int, bool> m_keysPressedThisFrame;
    std::unordered_map<int, bool> m_mouseDown;
    std::unordered_map<int, bool> m_mousePressedThisFrame;

    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
    int m_mouseX = 0;
    int m_mouseY = 0;
    int m_wheelDelta = 0;

    bool m_quitRequested = false;
    bool m_resized = false;
    int m_newWidth = 0;
    int m_newHeight = 0;
};
