#pragma once
#include <SDL.h>
#include <string>

// Owns the SDL window + OpenGL context.
class Window
{
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void swapBuffers();
    void setRelativeMouseMode(bool enabled);
    void setTitle(const std::string& title);
    void toggleFullscreen();

    int width() const { return m_width; }
    int height() const { return m_height; }
    float aspect() const { return static_cast<float>(m_width) / static_cast<float>(m_height ? m_height : 1); }
    void setSize(int w, int h) { m_width = w; m_height = h; }

    SDL_Window* sdlWindow() const { return m_window; }

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    int m_width;
    int m_height;
    bool m_fullscreen = false;
};
