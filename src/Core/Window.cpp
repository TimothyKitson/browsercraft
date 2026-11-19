#include "Window.h"
#include "GLFunctions.h"
#include <stdexcept>
#include <cstdio>
#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

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
    glVertexAttrib1f(6, 1056832.0f);

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

bool Window::fullscreen() const
{
#ifdef __EMSCRIPTEN__
    EmscriptenFullscreenChangeEvent state{};
    return emscripten_get_fullscreen_status(&state) == EMSCRIPTEN_RESULT_SUCCESS && state.isFullscreen;
#else
    return m_fullscreen;
#endif
}

void Window::toggleFullscreen()
{
#ifdef __EMSCRIPTEN__
    // Web fullscreen must be requested on the page's canvas. SDL's
    // window fullscreen API is a no-op in the browser build.
    if (fullscreen())
        emscripten_exit_fullscreen();
    else
        emscripten_request_fullscreen("#canvas", EM_TRUE);
#else
    const bool wantFullscreen = (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) == 0;
    if (SDL_SetWindowFullscreen(m_window, wantFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0)
    {
        std::printf("Fullscreen toggle failed: %s\n", SDL_GetError());
        return;
    }
    m_fullscreen = (SDL_GetWindowFlags(m_window) & SDL_WINDOW_FULLSCREEN) != 0;

    int w = 0, h = 0;
    SDL_GetWindowSize(m_window, &w, &h);
    if (w > 0 && h > 0) setSize(w, h);
#endif
}

void Window::setTitle(const std::string& title)
{
    SDL_SetWindowTitle(m_window, title.c_str());
}
