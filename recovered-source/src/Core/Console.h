#pragma once
#include <deque>
#include <functional>
#include <string>
#include <vector>

// The chat line and command console.
//
// There is no multiplayer, so anything typed that is not a command is just
// echoed back locally. Commands are dispatched through a callback the owner
// installs, which keeps every command's implementation in Application where
// the world, the player and the inventory actually live.
class Console
{
public:
    static constexpr size_t MAX_LINES = 60;    // scrollback kept
    static constexpr size_t VISIBLE_LINES = 10; // shown while open
    static constexpr float LINE_FADE_SECONDS = 9.0f;

    struct Line
    {
        std::string text;
        float age = 0.0f;
        bool error = false;
    };

    // Runs one command (without the leading slash) and returns what to print.
    using Handler = std::function<std::string(const std::string&)>;

    void setHandler(Handler handler) { m_handler = std::move(handler); }

    bool open() const { return m_open; }

    // `slash` pre-fills the input with "/" so a single key opens straight into
    // a command, the way Minecraft's does.
    void openInput(bool slash);
    void close();

    void update(float deltaTime);

    // Returns true when the key was consumed, so gameplay never sees it.
    bool handleKey(int scancode, bool ctrlDown);
    void handleText(const std::string& text);

    void print(const std::string& text, bool error = false);

    const std::string& input() const { return m_input; }
    const std::deque<Line>& lines() const { return m_lines; }

    // Lines still worth drawing when the console is shut: recent ones only.
    std::vector<const Line*> recentLines() const;

private:
    void submit();

    bool m_open = false;
    std::string m_input;
    std::deque<Line> m_lines;

    // Previously submitted entries, newest last, walked with up and down.
    std::vector<std::string> m_history;
    int m_historyIndex = -1;

    Handler m_handler;
};
