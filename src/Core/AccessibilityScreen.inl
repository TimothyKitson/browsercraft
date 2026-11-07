bool Application::guiSlider(int id, float x, float y, float w, const std::string& label, float& value)
{
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
    const bool inside = mouseX >= x && mouseX < x + w && mouseY >= y && mouseY < y + 20.0f;
    if (!m_automated && inside && m_input.wasMouseButtonPressed(SDL_BUTTON_LEFT)) m_activeSlider = id;
    if (!m_input.isMouseButtonDown(SDL_BUTTON_LEFT) && m_activeSlider == id)
    {
        m_activeSlider = -1;
        m_audio.play(Sound::Click, 0.45f);
    }
    bool changed = false;
    if (m_activeSlider == id && !m_automated)
    {
        const float next = std::clamp((mouseX - (x + 4.0f)) / (w - 8.0f), 0.0f, 1.0f);
        changed = next != value;
        value = next;
    }
    const bool lit = inside || m_activeSlider == id;
    if (!guiNineSlice(lit ? "sprites/widget/slider_highlighted" : "sprites/widget/slider", x, y, w, 20.0f, 1.0f))
    {
        m_ui.quad(x, y, w, 20.0f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        m_ui.quad(x + 1.0f, y + 1.0f, w - 2.0f, 18.0f, lit ? glm::vec4(0.3f, 0.3f, 0.3f, 1.0f) : glm::vec4(0.2f, 0.2f, 0.2f, 1.0f));
    }
    const float handleX = std::floor(x + value * (w - 8.0f));
    if (!guiNineSlice(lit ? "sprites/widget/slider_handle_highlighted" : "sprites/widget/slider_handle", handleX, y, 8.0f, 20.0f, 1.0f))
    {
        m_ui.quad(handleX, y, 8.0f, 20.0f, lit ? glm::vec4(1.0f) : glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        m_ui.quad(handleX + 1.0f, y + 1.0f, 6.0f, 18.0f, glm::vec4(0.6f, 0.6f, 0.6f, 1.0f));
    }
    m_ui.textWithShadow(label, std::floor(x + (w - UIRenderer::textWidth(label, 1.0f)) / 2.0f), y + 6.0f, 1.0f, glm::vec4(1.0f));
    return changed;
}

void Application::renderAccessibility()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    const float mouseX = static_cast<float>(m_input.mouseX()) / unit;
    const float mouseY = static_cast<float>(m_input.mouseY()) / unit;
    menuScreenBackdrop(w, h);
    const std::string title = "Accessibility Settings";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 12.0f, 1.0f, glm::vec4(1.0f));
    auto percent = [](float value) { return std::to_string(static_cast<int>(std::lround(value * 100.0f))) + "%"; };
    auto onOff = [](bool value) { return std::string(value ? "ON" : "OFF"); };
    std::string tip;
    bool changed = false;
    const int rows = 6;
    m_accessList.selected = -1;
    guiList(m_accessList, 33.0f, std::max(0.0f, h - 66.0f), 25.0f, 310.0f, rows, 0.0f, [&](int row, float x, float y, bool) {
        const float left = x - 2.0f;
        const float right = left + 160.0f;
        const float top = y;
        auto cycle = [&](float bx, const std::string& label, bool enabled) { return guiButton(bx, top, 150.0f, 20.0f, label, enabled); };
        auto hover = [&](float bx) { return mouseX >= bx && mouseX < bx + 150.0f && mouseY >= top && mouseY < top + 20.0f; };
        switch (row)
        {
            case 0:
                cycle(left, "Narrator: Not Available", false);
                cycle(right, "High Contrast: OFF", false);
                if (hover(right)) tip = "High Contrast resource pack is not available.";
                break;
            case 1:
                changed |= guiSlider(1, left, top, 150.0f, "Text Background Opacity: " + percent(m_textBackground), m_textBackground);
                if (guiSlider(2, right, top, 150.0f, "Chat Text Opacity: " + percent(m_chatOpacity), m_chatOpacity)) changed = true;
                m_chatOpacity = std::max(0.1f, m_chatOpacity);
                break;
            case 2:
                if (cycle(left, std::string("Sneak: ") + (m_toggleSneak ? "Toggle" : "Hold"), true)) { m_toggleSneak = !m_toggleSneak; changed = true; }
                if (cycle(right, std::string("Sprint: ") + (m_toggleSprint ? "Toggle" : "Hold"), true)) { m_toggleSprint = !m_toggleSprint; changed = true; }
                break;
            case 3:
            {
                float distortion = static_cast<float>(m_distortion) / 100.0f;
                if (guiSlider(3, left, top, 150.0f, "Distortion Effects: " + percent(distortion), distortion))
                {
                    m_distortion = static_cast<int>(std::lround(distortion * 100.0f));
                    changed = true;
                }
                if (hover(left)) tip = "How intense screen distortion effects are, such as nausea and Nether portals.";
                changed |= guiSlider(4, right, top, 150.0f, "FOV Effects: " + percent(m_fovEffects), m_fovEffects);
                if (hover(right)) tip = "How much the field of view changes with gameplay effects.";
                break;
            }
            case 4:
                changed |= guiSlider(5, left, top, 150.0f, "Darkness Pulsing: " + percent(m_darknessPulsing), m_darknessPulsing);
                if (hover(left)) tip = "How much the Darkness effect pulses when a Warden or Sculk Shrieker gives it to you.";
                if (cycle(right, "Hide Lightning Flashes: " + onOff(m_hideLightning), true)) { m_hideLightning = !m_hideLightning; changed = true; }
                if (hover(right)) tip = "Prevents lightning bolts from making the sky flash. The bolts themselves will still be visible.";
                break;
            default:
                changed |= guiSlider(6, left, top, 150.0f, "Panorama Scroll Speed: " + percent(m_panoramaSpeed), m_panoramaSpeed);
                if (cycle(right, "Hide Splash Texts: " + onOff(m_hideSplash), true)) { m_hideSplash = !m_hideSplash; changed = true; }
                if (hover(right)) tip = "Hides the yellow splash text in the main menu.";
                break;
        }
    });
    if (changed && m_activeSlider < 0) saveOptions();
    if (guiButton(std::floor((w - 200.0f) / 2.0f), h - 27.0f, 200.0f, 20.0f, "Done"))
    {
        saveOptions();
        m_screen = m_accessReturn;
    }
    if (!tip.empty()) wrappedTooltip(tip, mouseX, mouseY, 200.0f);
    m_ui.setScale(1.0f);
}

void Application::renderLanguage()
{
    const float unit = static_cast<float>(guiScale());
    m_ui.setScale(unit);
    const float w = m_ui.canvasWidth();
    const float h = m_ui.canvasHeight();
    menuScreenBackdrop(w, h);
    const std::string title = "Language";
    m_ui.textWithShadow(title, std::floor((w - UIRenderer::textWidth(title, 1.0f)) / 2.0f), 12.0f, 1.0f, glm::vec4(1.0f));
    m_languageList.selected = 0;
    guiList(m_languageList, 33.0f, std::max(0.0f, h - 86.0f), 18.0f, 270.0f, 1, 0.0f, [&](int, float x, float y, bool) {
        const std::string name = "English (US)";
        m_ui.textWithShadow(name, std::floor(x - 2.0f + (270.0f - UIRenderer::textWidth(name, 1.0f)) / 2.0f), y + 1.0f, 1.0f, glm::vec4(1.0f));
    });
    const std::string warning = "(Language translations may not be 100% accurate)";
    m_ui.textWithShadow(warning, std::floor((w - UIRenderer::textWidth(warning, 1.0f)) / 2.0f), h - 45.0f, 1.0f,
                        glm::vec4(0xBA / 255.0f, 0xBA / 255.0f, 0xBA / 255.0f, 1.0f));
    const float x0 = std::floor((w - 308.0f) / 2.0f);
    guiButton(x0, h - 28.0f, 150.0f, 20.0f, "Font Settings...", false);
    if (guiButton(x0 + 158.0f, h - 28.0f, 150.0f, 20.0f, "Done")) m_screen = m_accessReturn;
    m_ui.setScale(1.0f);
}

bool Application::titleIconButton(float x, float y, float size, const char* icon, const char* tooltip)
{
    const float mouseX = static_cast<float>(m_input.mouseX()) / m_ui.scale();
    const float mouseY = static_cast<float>(m_input.mouseY()) / m_ui.scale();
    const bool hit = menuButton({ x, y, size, size }, "");
    const float unit = static_cast<float>(guiScale()) / m_ui.scale();
    const float iconSize = 15.0f * unit;
    const float ix = x + (size - iconSize) * 0.5f;
    const float iy = y + (size - iconSize) * 0.5f;
    if (!guiSprite(icon, ix, iy, iconSize, iconSize))
    {
        const glm::vec4 ink(1.0f);
        if (std::string(icon).find("language") != std::string::npos)
        {
            m_ui.quad(ix + iconSize * 0.2f, iy + iconSize * 0.2f, iconSize * 0.6f, iconSize * 0.6f, glm::vec4(0.3f, 0.5f, 0.9f, 1.0f));
            m_ui.quad(ix + iconSize * 0.45f, iy + iconSize * 0.2f, iconSize * 0.1f, iconSize * 0.6f, ink);
            m_ui.quad(ix + iconSize * 0.2f, iy + iconSize * 0.45f, iconSize * 0.6f, iconSize * 0.1f, ink);
        }
        else
        {
            m_ui.quad(ix + iconSize * 0.4f, iy + iconSize * 0.05f, iconSize * 0.2f, iconSize * 0.2f, ink);
            m_ui.quad(ix + iconSize * 0.1f, iy + iconSize * 0.3f, iconSize * 0.8f, iconSize * 0.12f, ink);
            m_ui.quad(ix + iconSize * 0.42f, iy + iconSize * 0.3f, iconSize * 0.16f, iconSize * 0.4f, ink);
            m_ui.quad(ix + iconSize * 0.3f, iy + iconSize * 0.68f, iconSize * 0.12f, iconSize * 0.3f, ink);
            m_ui.quad(ix + iconSize * 0.58f, iy + iconSize * 0.68f, iconSize * 0.12f, iconSize * 0.3f, ink);
        }
    }
    if (mouseX >= x && mouseX < x + size && mouseY >= y && mouseY < y + size) m_titleTip = tooltip;
    return hit;
}
