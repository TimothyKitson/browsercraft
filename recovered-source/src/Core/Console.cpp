#include "Console.h"
#include "Input.h"
#include <SDL.h>
#include <algorithm>

void Console::openInput(bool slash)
{
    m_open = true;
    m_input = slash ? "/" : "";
    m_historyIndex = -1;
    Input::startTextEntry();
}

void Console::close()
{
    m_open = false;
    m_input.clear();
    m_historyIndex = -1;
    Input::stopTextEntry();
}

void Console::update(float deltaTime)
{
    for (Line& line : m_lines) line.age += deltaTime;
}

void Console::print(const std::string& text, bool error)
{
    // Split on newlines so a multi-line reply scrolls like separate messages.
    size_t start = 0;
    while (start <= text.size())
    {
        const size_t end = text.find('\n', start);
        const std::string piece = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (!piece.empty() || end != std::string::npos)
            m_lines.push_back(Line{ piece, 0.0f, error });
        if (end == std::string::npos) break;
        start = end + 1;
    }

    while (m_lines.size() > MAX_LINES) m_lines.pop_front();
}

std::vector<const Console::Line*> Console::recentLines() const
{
    std::vector<const Line*> out;
    for (auto it = m_lines.rbegin(); it != m_lines.rend() && out.size() < VISIBLE_LINES; ++it)
    {
        if (!m_open && it->age > LINE_FADE_SECONDS) break;
        out.push_back(&*it);
    }
    std::reverse(out.begin(), out.end());
    return out;
}

void Console::handleText(const std::string& text)
{
    if (!m_open) return;
    for (char c : text)
        if (c >= 0x20 && c != 0x7F) m_input += c;
}

bool Console::handleKey(int scancode, bool ctrlDown)
{
    if (!m_open) return false;

    switch (scancode)
    {
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
            submit();
            return true;

        case SDL_SCANCODE_ESCAPE:
            close();
            return true;

        case SDL_SCANCODE_BACKSPACE:
            if (!m_input.empty())
            {
                if (ctrlDown)
                {
                    // Rub out the last word, trailing spaces and all.
                    while (!m_input.empty() && m_input.back() == ' ') m_input.pop_back();
                    while (!m_input.empty() && m_input.back() != ' ') m_input.pop_back();
                }
                else
                {
                    m_input.pop_back();
                }
            }
            return true;

        case SDL_SCANCODE_UP:
            if (!m_history.empty())
            {
                if (m_historyIndex < 0) m_historyIndex = static_cast<int>(m_history.size());
                if (m_historyIndex > 0) --m_historyIndex;
                m_input = m_history[static_cast<size_t>(m_historyIndex)];
            }
            return true;

        case SDL_SCANCODE_DOWN:
            if (m_historyIndex >= 0)
            {
                ++m_historyIndex;
                if (m_historyIndex >= static_cast<int>(m_history.size()))
                {
                    m_historyIndex = -1;
                    m_input.clear();
                }
                else
                {
                    m_input = m_history[static_cast<size_t>(m_historyIndex)];
                }
            }
            return true;

        default:
            // Everything else while the console is open belongs to the console,
            // so gameplay never acts on a keystroke meant for the text line.
            return true;
    }
}

void Console::submit()
{
    std::string entry = m_input;

    // Trim both ends.
    const size_t first = entry.find_first_not_of(" \t");
    const size_t last = entry.find_last_not_of(" \t");
    entry = (first == std::string::npos) ? "" : entry.substr(first, last - first + 1);

    if (entry.empty())
    {
        close();
        return;
    }

    if (m_history.empty() || m_history.back() != entry) m_history.push_back(entry);

    if (entry[0] == '/')
    {
        const std::string command = entry.substr(1);
        print(entry);
        if (m_handler)
        {
            const std::string reply = m_handler(command);
            if (!reply.empty())
            {
                // A reply starting with "!" marks an error, which the renderer
                // draws in red. The marker itself is not shown.
                const bool error = reply[0] == '!';
                print(error ? reply.substr(1) : reply, error);
            }
        }
    }
    else
    {
        // No multiplayer yet, so this only ever talks to you.
        print("<you> " + entry);
    }

    close();
}
