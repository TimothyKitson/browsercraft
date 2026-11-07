namespace
{
    constexpr float BOOK_TEXT_WIDTH = 114.0f;
    constexpr int BOOK_LINES = 14;
    constexpr size_t BOOK_PAGE_CHARS = 1024;
    constexpr int BOOK_MAX_PAGES = 100;
    constexpr size_t BOOK_TITLE_CHARS = 15;

    std::vector<std::string> wrapBookPage(const std::string& text, float width, float scale)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t i = 0;
        while (i <= text.size())
        {
            if (i == text.size() || text[i] == '\n')
            {
                lines.push_back(line);
                line.clear();
                ++i;
                continue;
            }
            size_t end = i;
            while (end < text.size() && text[end] != ' ' && text[end] != '\n') ++end;
            std::string word = text.substr(i, end - i);
            const bool space = end < text.size() && text[end] == ' ';
            std::string candidate = line + word;
            if (UIRenderer::textWidth(candidate, scale) <= width)
                line = candidate;
            else
            {
                if (!line.empty())
                {
                    while (!line.empty() && line.back() == ' ') line.pop_back();
                    lines.push_back(line);
                    line.clear();
                }
                while (UIRenderer::textWidth(word, scale) > width && word.size() > 1)
                {
                    size_t fit = 1;
                    while (fit < word.size() && UIRenderer::textWidth(word.substr(0, fit + 1), scale) <= width) ++fit;
                    lines.push_back(word.substr(0, fit));
                    word.erase(0, fit);
                }
                line = word;
            }
            if (space) line += ' ';
            i = space ? end + 1 : end;
            if (end == text.size()) { lines.push_back(line); break; }
        }
        if (lines.empty()) lines.push_back("");
        return lines;
    }

    bool bookPageFits(const std::string& text)
    {
        return text.size() < BOOK_PAGE_CHARS && static_cast<int>(wrapBookPage(text, BOOK_TEXT_WIDTH, 1.0f).size()) <= BOOK_LINES;
    }

    int bookGeneration(const ItemStack& stack) { return static_cast<int>((stack.enchantments >> 24) & 0x3u); }
}

const Application::BookText* Application::bookOf(const ItemStack& stack) const
{
    if (stack.id != Items::WritableBook && stack.id != Items::WrittenBook) return nullptr;
    if (stack.damage <= 0 || stack.damage > static_cast<int>(m_bookTexts.size())) return nullptr;
    return &m_bookTexts[static_cast<size_t>(stack.damage - 1)];
}

ItemStack* Application::lecternBookAt(const glm::ivec3& at)
{
    auto found = m_lecternBooks.find({ static_cast<int>(m_dimension), at.x, at.y, at.z });
    return found == m_lecternBooks.end() ? nullptr : &found->second;
}

void Application::openBook(const ItemStack& stack, bool fromLectern, const glm::ivec3& lectern)
{
    m_bookOpen = true;
    m_bookEditing = stack.id == Items::WritableBook && !fromLectern;
    m_bookSigning = false;
    m_bookFromLectern = fromLectern;
    m_bookLectern = lectern;
    m_bookPage = 0;
    m_bookSlot = m_inventory.selectedSlot();
    const BookText* text = bookOf(stack);
    m_bookShown = text ? *text : BookText{};
    m_bookDraft = m_bookShown.pages;
    if (m_bookDraft.empty()) m_bookDraft.push_back("");
    if (m_bookShown.pages.empty()) m_bookShown.pages.push_back("");
    m_bookTitle.clear();
    setMouseCaptured(false);
    m_audio.play(Sound::Click, 0.4f, 1.3f);
}

void Application::saveBookDraft(bool sign)
{
    ItemStack& held = m_inventory.slot(m_bookSlot);
    if (held.id != Items::WritableBook) return;
    while (m_bookDraft.size() > 1 && m_bookDraft.back().empty()) m_bookDraft.pop_back();
    const bool blank = m_bookDraft.size() == 1 && m_bookDraft[0].empty();
    if (held.damage <= 0 || held.damage > static_cast<int>(m_bookTexts.size()))
    {
        if (blank && !sign) return;
        if (m_bookTexts.size() >= 60000u) return;
        m_bookTexts.emplace_back();
        held.damage = static_cast<int>(m_bookTexts.size());
    }
    BookText& book = m_bookTexts[static_cast<size_t>(held.damage - 1)];
    book.pages = m_bookDraft;
    if (sign)
    {
        book.title = m_bookTitle;
        book.author = m_playerName.empty() ? "Player" : m_playerName;
        held.id = Items::WrittenBook;
        held.enchantments = 0;
    }
}

void Application::closeBook(bool keep)
{
    if (!m_bookOpen) return;
    m_bookOpen = false;
    if (keep && m_bookEditing) saveBookDraft(false);
    m_bookSigning = false;
    if (!m_inventoryOpen) setMouseCaptured(true);
}

void Application::updateBookInput()
{
    if (!m_bookOpen) return;
    if (m_bookFromLectern && m_world->getBlock(m_bookLectern.x, m_bookLectern.y, m_bookLectern.z) != Blocks::LecternBook)
    {
        closeBook(false);
        return;
    }
    if (m_bookEditing && m_inventory.slot(m_bookSlot).id != Items::WritableBook)
    {
        m_bookOpen = false;
        if (!m_inventoryOpen) setMouseCaptured(true);
        return;
    }

    const std::vector<std::string>& pages = m_bookEditing ? m_bookDraft : m_bookShown.pages;
    const int count = static_cast<int>(pages.size());
    if (m_input.wasKeyPressed(SDL_SCANCODE_PAGEUP) && m_bookPage > 0) --m_bookPage;
    if (m_input.wasKeyPressed(SDL_SCANCODE_PAGEDOWN) && m_bookPage + 1 < count) ++m_bookPage;
    if (!m_bookEditing) return;

    if (m_bookSigning)
    {
        if (m_input.wasKeyPressed(SDL_SCANCODE_BACKSPACE) && !m_bookTitle.empty()) m_bookTitle.pop_back();
        for (char c : m_input.typedText())
            if (c >= 32 && m_bookTitle.size() < BOOK_TITLE_CHARS) m_bookTitle.push_back(c);
        if ((m_input.wasKeyPressed(SDL_SCANCODE_RETURN) || m_input.wasKeyPressed(SDL_SCANCODE_KP_ENTER)) && !m_bookTitle.empty())
        {
            saveBookDraft(true);
            m_bookOpen = false;
            m_bookSigning = false;
            if (!m_inventoryOpen) setMouseCaptured(true);
        }
        return;
    }

    m_bookPage = std::clamp(m_bookPage, 0, static_cast<int>(m_bookDraft.size()) - 1);
    std::string& page = m_bookDraft[static_cast<size_t>(m_bookPage)];
    if (m_input.wasKeyPressed(SDL_SCANCODE_BACKSPACE) && !page.empty()) page.pop_back();
    for (char c : m_input.typedText())
    {
        if (c < 32) continue;
        std::string next = page + c;
        if (bookPageFits(next)) page = next;
    }
    if (m_input.wasKeyPressed(SDL_SCANCODE_RETURN) || m_input.wasKeyPressed(SDL_SCANCODE_KP_ENTER))
    {
        std::string next = page + '\n';
        if (bookPageFits(next)) page = next;
    }
}

void Application::renderBookScreen()
{
    m_ui.setScale(static_cast<float>(guiScale()) * MENU_UNIT);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    const float u = 1.0f / MENU_UNIT;
    m_ui.quad(0, 0, w, h, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));

    const float left = std::floor((w - 192.0f * u) * 0.5f);
    const float top = 2.0f * u;
    if (const GuiImage* image = m_gui.image("book"))
        m_ui.texturedQuad(image->texture, left, top, 192.0f * u, 192.0f * u, 0.0f, 0.0f, 0.75f, 0.75f, glm::vec4(1.0f));
    else
    {
        m_ui.quad(left + 20.0f * u, top + 2.0f * u, 150.0f * u, 180.0f * u, glm::vec4(0.36f, 0.22f, 0.12f, 1.0f));
        m_ui.quad(left + 24.0f * u, top + 6.0f * u, 142.0f * u, 172.0f * u, glm::vec4(0.95f, 0.92f, 0.82f, 1.0f));
    }

    const glm::vec4 ink(0.0f, 0.0f, 0.0f, 1.0f);
    const glm::vec4 grey(0.33f, 0.33f, 0.33f, 1.0f);
    const float textScale = PANEL_TEXT;
    const float textLeft = left + 36.0f * u;
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
    const bool clicked = !m_automated && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT);
    const bool blink = static_cast<int>(m_elapsedSeconds * 3.0f) % 2 == 0;

    auto centred = [&](const std::string& text, float y, const glm::vec4& colour) {
        const float width = UIRenderer::textWidth(text, textScale);
        m_ui.text(text, textLeft + (BOOK_TEXT_WIDTH * u - width) * 0.5f, y, textScale, colour);
    };

    const float buttonY = 196.0f * u;
    const float buttonW = 98.0f * u;
    const float buttonH = 20.0f * u;
    const float middle = w * 0.5f;

    if (m_bookEditing && m_bookSigning)
    {
        centred("Enter Book Title:", top + 32.0f * u, ink);
        centred(m_bookTitle + (blink ? "_" : " "), top + 48.0f * u, ink);
        centred("by " + (m_playerName.empty() ? std::string("Player") : m_playerName), top + 58.0f * u, grey);
        const std::vector<std::string> warning =
            wrapBookPage("Note! When you sign the book, it will no longer be editable.", BOOK_TEXT_WIDTH * u, textScale);
        for (size_t i = 0; i < warning.size(); ++i)
            m_ui.text(warning[i], textLeft, top + (80.0f + 9.0f * static_cast<float>(i)) * u, textScale, ink);
        if (menuButton({ middle - 100.0f * u, buttonY, buttonW, buttonH }, "Sign and Close", !m_bookTitle.empty()))
        {
            saveBookDraft(true);
            m_bookOpen = false;
            m_bookSigning = false;
            if (!m_inventoryOpen) setMouseCaptured(true);
        }
        else if (menuButton({ middle + 2.0f * u, buttonY, buttonW, buttonH }, "Cancel"))
            m_bookSigning = false;
        m_ui.setScale(1.0f);
        return;
    }

    const std::vector<std::string>& pages = m_bookEditing ? m_bookDraft : m_bookShown.pages;
    const int count = std::max(1, static_cast<int>(pages.size()));
    m_bookPage = std::clamp(m_bookPage, 0, count - 1);
    const std::string indicator = "Page " + std::to_string(m_bookPage + 1) + " of " + std::to_string(count);
    m_ui.text(indicator, left + (192.0f - 44.0f) * u - UIRenderer::textWidth(indicator, textScale), top + 16.0f * u, textScale, ink);

    std::string body = pages.empty() ? std::string() : pages[static_cast<size_t>(m_bookPage)];
    const std::vector<std::string> lines = wrapBookPage(body, BOOK_TEXT_WIDTH * u, textScale);
    for (size_t i = 0; i < lines.size() && i < static_cast<size_t>(BOOK_LINES); ++i)
        m_ui.text(lines[i], textLeft, top + (30.0f + 9.0f * static_cast<float>(i)) * u, textScale, ink);
    if (m_bookEditing && blink)
    {
        const size_t last = std::min(lines.size(), static_cast<size_t>(BOOK_LINES)) - 1;
        const float caretX = textLeft + UIRenderer::textWidth(lines[last], textScale);
        m_ui.text("_", caretX, top + (30.0f + 9.0f * static_cast<float>(last)) * u, textScale, ink);
    }

    auto turn = [&](bool forward, float x) -> bool {
        const float bw = 23.0f * u, bh = 13.0f * u, y = top + 157.0f * u;
        const bool hover = mouseX >= x && mouseX <= x + bw && mouseY >= y && mouseY <= y + bh;
        const std::string sprite = std::string("sprites/widget/page_") + (forward ? "forward" : "backward") + (hover ? "_highlighted" : "");
        if (!guiSprite(sprite.c_str(), x, y, bw, bh))
        {
            const glm::vec4 arrow = hover ? glm::vec4(0.8f, 0.2f, 0.2f, 1.0f) : glm::vec4(0.45f, 0.3f, 0.2f, 1.0f);
            for (int k = 0; k < 6; ++k)
                m_ui.quad(forward ? x + (6.0f + k * 2.0f) * u : x + (17.0f - k * 2.0f) * u, y + (6.5f - (5 - k)) * u,
                          2.0f * u, (2.0f * (5 - k) + 1.0f) * u, arrow);
        }
        return hover && clicked;
    };
    const bool canGrow = m_bookEditing && count < BOOK_MAX_PAGES;
    if ((m_bookPage + 1 < count || canGrow) && turn(true, left + 116.0f * u))
    {
        if (m_bookPage + 1 >= count && canGrow) m_bookDraft.push_back("");
        ++m_bookPage;
        m_audio.play(Sound::Click, 0.3f, 1.4f);
    }
    if (m_bookPage > 0 && turn(false, left + 43.0f * u))
    {
        --m_bookPage;
        m_audio.play(Sound::Click, 0.3f, 1.4f);
    }

    if (m_bookEditing)
    {
        if (menuButton({ middle - 100.0f * u, buttonY, buttonW, buttonH }, "Sign")) m_bookSigning = true;
        else if (menuButton({ middle + 2.0f * u, buttonY, buttonW, buttonH }, "Done")) closeBook(true);
    }
    else if (m_bookFromLectern)
    {
        if (menuButton({ middle - 100.0f * u, buttonY, buttonW, buttonH }, "Done")) closeBook(false);
        else if (menuButton({ middle + 2.0f * u, buttonY, buttonW, buttonH }, "Take Book"))
        {
            const glm::ivec3 at = m_bookLectern;
            closeBook(false);
            useLectern(at, Blocks::LecternBook);
        }
    }
    else if (menuButton({ middle - 100.0f * u, buttonY, 200.0f * u, buttonH }, "Done"))
        closeBook(false);
    m_ui.setScale(1.0f);
}
