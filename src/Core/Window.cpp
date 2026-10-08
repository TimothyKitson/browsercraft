#include "Window.h"
#include "GLFunctions.h"
#include <stdexcept>
#include <cstdio>

Window::Window(const std::string& title, int width, int height)
    : m_width(width), m_height(height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());

#ifdef __EMSCRIPTEN__
    // WebGL2 is OpenGL ES 3.0. Asking for a desktop 3.3 core profile here
    // fails outright with "context attributes are not supported".
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    m_window = SDL_CreateWindow(title.c_str(),
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                width, height,
                                SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!m_window)
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext)
        throw std::runtime_error(std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());

    SDL_GL_SetSwapInterval(1); // vsync

    if (!LoadGLFunctions())
        throw std::runtime_error("Failed to load required OpenGL 3.3 functions.");

    std::printf("GPU:      %s\n", glGetString(GL_RENDERER));
    std::printf("OpenGL:   %s\n", glGetString(GL_VERSION));
}

Window::~Window()
{
    if (m_glContext) SDL_GL_DeleteContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Window::swapBuffers() { SDL_GL_SwapWindow(m_window); }

void Window::setRelativeMouseMode(bool enabled)
{
    SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
    // Explicit, because leaving relative mode does not reliably bring the
    // pointer back. Menus, the inventory and every other screen are
    // mouse-driven and were being used with an invisible cursor.
    SDL_ShowCursor(enabled ? SDL_DISABLE : SDL_ENABLE);
}

void Window::toggleFullscreen()
{
#ifdef __EMSCRIPTEN__
    // The browser already owns F11, and the per-frame canvas resize
    // would fight anything SDL did here. Leave it to the page.
    return;
#else
    m_fullscreen = !m_fullscreen;
    // Borderless desktop rather than a true mode switch: it is instant,
    // and it never leaves the player staring at a black screen while the
    // monitor re-syncs. The browser build ignores this entirely -- the
    // page is in charge of its own size there.
    SDL_SetWindowFullscreen(m_window, m_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

    int w = 0, h = 0;
    SDL_GetWindowSize(m_window, &w, &h);
    if (w > 0 && h > 0) setSize(w, h);
#endif
}

void Window::setTitle(const std::string& title)
{
    SDL_SetWindowTitle(m_window, title.c_str());
}
