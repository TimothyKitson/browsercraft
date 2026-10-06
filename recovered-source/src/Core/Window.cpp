#include "Window.h"
#include "GLFunctions.h"
#include <stdexcept>
#include <cstdio>

Window::Window(const std::string& title, int width, int height)
    : m_width(width), m_height(height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

#ifdef __EMSCRIPTEN__
    // WebGL2 is OpenGL ES 3.0. Asking for a desktop core profile here makes
    // SDL_GL_CreateContext fail with "context attributes are not supported".
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    // Request an OpenGL 3.3 core-profile context.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!m_window)
    {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext)
    {
        throw std::runtime_error(std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
    }

    SDL_GL_SetSwapInterval(1); // vsync on

    if (!LoadGLFunctions())
    {
        throw std::runtime_error("Failed to load one or more required OpenGL functions.");
    }

    std::printf("OpenGL vendor:   %s\n", glGetString(GL_VENDOR));
    std::printf("OpenGL renderer: %s\n", glGetString(GL_RENDERER));
    std::printf("OpenGL version:  %s\n", glGetString(GL_VERSION));
}

Window::~Window()
{
    if (m_glContext) SDL_GL_DeleteContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Window::swapBuffers()
{
    SDL_GL_SwapWindow(m_window);
}

void Window::setRelativeMouseMode(bool enabled)
{
    SDL_SetRelativeMouseMode(enabled ? SDL_TRUE : SDL_FALSE);
}
