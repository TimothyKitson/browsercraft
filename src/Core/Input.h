#pragma once
#include <SDL.h>
#include <string>
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
    // Defined out of line: the browser build answers these from the DOM
    // rather than from SDL, whose web backend drifts.
    int mouseX() const;
    int mouseY() const;
    int wheelDelta() const { return m_wheelDelta; }

    // Text typed this frame, and how many times backspace fired. Key repeat
    // is deliberately counted here even though wasKeyPressed() ignores it:
    // holding backspace should keep deleting.
    const std::string& typedText() const { return m_typedText; }
    int backspaces() const { return m_backspaces; }

    // Whatever was pressed this frame, for the rebinding screen. 0 when
    // nothing was. Scanning every scancode from the caller would mean
    // five hundred lookups a frame to find at most one answer.
    int firstKeyPressed() const;
    int firstMouseButtonPressed() const;

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

    std::string m_typedText;
    int m_backspaces = 0;

    bool m_quitRequested = false;
    bool m_resized = false;
    int m_newWidth = 0;
    int m_newHeight = 0;
};
