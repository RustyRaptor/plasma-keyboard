/*
    SPDX-FileCopyrightText: 2024 Aleix Pol i Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
    SPDX-FileCopyrightText: 2025 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "inputmethodconnection.h"

#include "inputengine.h"
#include "inputmethod_p.h"
#include "inputmethodhints.h"
#include "kwinfakeinput.h"
#include "llmcompletionclient.h"
#include "plasmakeyboardsettings.h"
#include "suggestioncontroller.h"
#include "transcriptionclient.h"
#include "virtualkeyboardcontext.h"
#include "vkbdwaylandinputbackend.h"
#include "voiceinput.h"

#include "overlay/longpresstrigger.h"
#include "overlay/overlaycontroller.h"
#include "overlay/prefixquerytrigger.h"
#include "overlay/textexpansiontrigger.h"

#include <QElapsedTimer>
#include <QtWaylandClient/private/qwayland-text-input-unstable-v1.h>
#include <xkbcommon/xkbcommon-keysyms.h>

Q_GLOBAL_STATIC(InputMethod, s_im)

static QList<Qt::Key> initCapture()
{
    return {
        Qt::Key_Left,
        Qt::Key_Right,
        Qt::Key_Up,
        Qt::Key_Down,
    };
}

Q_GLOBAL_STATIC_WITH_ARGS(const QList<Qt::Key>, KEYBOARD_NAVIGATION_CAPTURE_KEYS, (initCapture()));
Q_GLOBAL_STATIC_WITH_ARGS(const QList<Qt::Key>,
                          KEYBOARD_NAVIGATION_ACTIVE_CAPTURE_KEYS,
                          (initCapture() + QList<Qt::Key>{Qt::Key_Return, Qt::Key_Enter, Qt::Key_Escape}));

static uint keysymForQtKey(int key)
{
    switch (key) {
    case Qt::Key_Backspace:
        return XKB_KEY_BackSpace;
    case Qt::Key_Delete:
        return XKB_KEY_Delete;
    case Qt::Key_Return:
        return XKB_KEY_Return;
    case Qt::Key_Enter:
        return XKB_KEY_KP_Enter;
    case Qt::Key_Left:
        return XKB_KEY_Left;
    case Qt::Key_Right:
        return XKB_KEY_Right;
    case Qt::Key_Up:
        return XKB_KEY_Up;
    case Qt::Key_Down:
        return XKB_KEY_Down;
    case Qt::Key_Home:
        return XKB_KEY_Home;
    case Qt::Key_End:
        return XKB_KEY_End;
    case Qt::Key_Space:
        return XKB_KEY_space;
    default:
        return XKB_KEY_NoSymbol;
    }
}

static uint currentTimestamp()
{
    static QElapsedTimer timer = [] {
        QElapsedTimer value;
        value.start();
        return value;
    }();
    return uint(timer.elapsed());
}

InputMethodConnection::InputMethodConnection(QObject *parent)
    : QObject(parent)
    , m_input(&(*s_im))
    , m_vkbdInputBackend(new VkbdWaylandInputBackend(this))
    , m_virtualKeyboardContext(new VirtualKeyboardContext(m_vkbdInputBackend, this))
    , m_fakeInput(new KWinFakeInput(m_vkbdInputBackend, this))
    , m_overlayController(new OverlayController(&m_input, this))
{
    m_input.setGrabbing(true);

    m_visibility.setHideDelay(PlasmaKeyboardSettings::self()->panelHideDelayMs());
    connect(PlasmaKeyboardSettings::self(), &PlasmaKeyboardSettings::panelHideDelayMsChanged, this, [this] {
        m_visibility.setHideDelay(PlasmaKeyboardSettings::self()->panelHideDelayMs());
    });
    connect(&m_visibility, &PanelVisibility::visibleChanged, this, &InputMethodConnection::updateWindowVisible);

    auto *settings = PlasmaKeyboardSettings::self();
    connect(settings, &PlasmaKeyboardSettings::suggestionsEnabledChanged, this, &InputMethodConnection::updateSuggestionSettings);
    connect(settings, &PlasmaKeyboardSettings::suggestionModelApiChanged, this, &InputMethodConnection::updateSuggestionSettings);
    connect(settings, &PlasmaKeyboardSettings::suggestionModelEndpointChanged, this, &InputMethodConnection::updateSuggestionSettings);
    connect(settings, &PlasmaKeyboardSettings::suggestionModelNameChanged, this, &InputMethodConnection::updateSuggestionSettings);
    connect(settings, &PlasmaKeyboardSettings::suggestionModelApiKeyChanged, this, &InputMethodConnection::updateSuggestionSettings);
    updateSuggestionSettings();

    m_voiceInput = new VoiceInput(m_virtualKeyboardContext->inputEngine(), this);
    connect(settings, &PlasmaKeyboardSettings::voiceInputApiChanged, this, &InputMethodConnection::updateVoiceInputSettings);
    connect(settings, &PlasmaKeyboardSettings::voiceInputEndpointChanged, this, &InputMethodConnection::updateVoiceInputSettings);
    connect(settings, &PlasmaKeyboardSettings::voiceInputModelChanged, this, &InputMethodConnection::updateVoiceInputSettings);
    connect(settings, &PlasmaKeyboardSettings::voiceInputApiKeyChanged, this, &InputMethodConnection::updateVoiceInputSettings);
    connect(settings, &PlasmaKeyboardSettings::voiceInputLanguageChanged, this, &InputMethodConnection::updateVoiceInputSettings);
    updateVoiceInputSettings();

    // Stop listening when the keyboard goes away
    connect(&m_visibility, &PanelVisibility::visibleChanged, this, [this] {
        if (!m_visibility.visible() && m_voiceInput->state() == VoiceInput::Recording) {
            m_voiceInput->cancel();
        }
    });
    connect(&m_visibility, &PanelVisibility::pinnedChanged, this, &InputMethodConnection::pinnedChanged);
    connect(&m_visibility, &PanelVisibility::detachedChanged, this, &InputMethodConnection::detachedChanged);

    m_overlayController->registerTrigger(new LongPressTrigger(m_virtualKeyboardContext->inputEngine(), m_overlayController));
    m_overlayController->registerTrigger(new PrefixQueryTrigger(m_overlayController));
    m_overlayController->registerTrigger(new TextExpansionTrigger(m_overlayController));

    const auto syncBackendState = [this] {
        m_vkbdInputBackend->setActive(m_input.hasContext());
        m_vkbdInputBackend->setInputMethodHints(
            mapInputMethodHints(m_input.contentHint(), m_input.contentPurpose(), PlasmaKeyboardSettings::self()->autoCapitalizationEnabled()));
        m_vkbdInputBackend->setSurroundingState(m_input.surroundingText(), m_input.cursorPos(), m_input.anchorPos());
    };

    // ** Wayland input-method-v1 (KWin) hooks **
    // Compositor -> plasma-keyboard events

    connect(&m_input, &InputPlugin::contextChanged, this, [this, syncBackendState] {
        syncBackendState();

        if (m_overlayController) {
            m_overlayController->cancelOverlay();
        }

        if (!m_input.hasContext()) {
            m_vkbdInputBackend->reset();
        }

        m_visibility.setHasContext(m_input.hasContext());
    });

    connect(&m_input, &InputPlugin::surroundingTextChanged, this, [this, syncBackendState] {
        syncBackendState();

        if (m_overlayController) {
            m_overlayController->handleSurroundingTextChanged();
        }

        m_visibility.notifyContextActivity();
    });

    connect(&m_input, &InputPlugin::contentTypeChanged, this, [this, syncBackendState] {
        syncBackendState();
    });

    connect(&m_input, &InputPlugin::resetRequested, this, [this] {
        m_vkbdInputBackend->reset();
    });

    connect(&m_input, &InputPlugin::keyPressed, this, [this](QKeyEvent *keyEvent) {
        if (m_overlayController && m_overlayController->processKeyPress(keyEvent)) {
            keyEvent->accept();
            return;
        }

        if (!m_window || !m_window->isExposed()) {
            return;
        }

        if (!PlasmaKeyboardSettings::self()->keyboardNavigationEnabled()) {
            return;
        }

        if (!m_keyboardNavigationAvailable) {
            return;
        }

        const auto keys = m_keyboardNavigationActive ? *KEYBOARD_NAVIGATION_ACTIVE_CAPTURE_KEYS : *KEYBOARD_NAVIGATION_CAPTURE_KEYS;
        for (const auto key : keys) {
            if (keyEvent->key() == key) {
                keyEvent->accept();
                Q_EMIT keyNavigationPressed(key);
                break;
            }
        }
    });

    connect(&m_input, &InputPlugin::keyReleased, this, [this](QKeyEvent *keyEvent) {
        if (m_overlayController && m_overlayController->processKeyRelease(keyEvent)) {
            keyEvent->accept();
            return;
        }

        if (!m_window || !m_window->isExposed()) {
            return;
        }

        if (!PlasmaKeyboardSettings::self()->keyboardNavigationEnabled()) {
            return;
        }

        if (!m_keyboardNavigationAvailable) {
            return;
        }

        const auto keys = m_keyboardNavigationActive ? *KEYBOARD_NAVIGATION_ACTIVE_CAPTURE_KEYS : *KEYBOARD_NAVIGATION_CAPTURE_KEYS;
        for (const auto key : keys) {
            if (keyEvent->key() == key) {
                keyEvent->accept();
                Q_EMIT keyNavigationReleased(key);
                break;
            }
        }
    });

    connect(PlasmaKeyboardSettings::self(), &PlasmaKeyboardSettings::autoCapitalizationEnabledChanged, this, syncBackendState);

    // ** Virtual keyboard backend hooks **
    // plasma-keyboard (keyboard engine) -> compositor

    connect(m_vkbdInputBackend, &VkbdWaylandInputBackend::preeditTextRequested, this, [this](const QString &text) {
        if (!m_window || !m_window->isExposed()) {
            return;
        }

        if (!text.isEmpty()) {
            const int utf8Length = text.toUtf8().size();
            m_input.setPreEditStyle(0, utf8Length, QtWayland::zwp_text_input_v1::preedit_style_underline);
            m_input.setPreEditCursor(utf8Length);
        }
        m_input.setPreEditString(text);
    });

    connect(m_vkbdInputBackend, &VkbdWaylandInputBackend::commitTextRequested, this, [this](const QString &text) {
        if (!m_window || !m_window->isExposed()) {
            return;
        }

        if (!m_input.hasContext()) {
            // Pinned or lingering without a text input: emulate the keys instead
            m_fakeInput->sendText(text);
            return;
        }

        m_input.commit(text);
    });

    connect(m_vkbdInputBackend, &VkbdWaylandInputBackend::deleteSurroundingTextRequested, this, [this](int index, int length) {
        if (!m_window || !m_window->isExposed()) {
            return;
        }

        if (!m_input.hasContext()) {
            // Without surrounding text only deleting right before the cursor can be emulated
            if (index == -length) {
                for (int i = 0; i < length; ++i) {
                    m_fakeInput->sendKeyPressed(Qt::Key_Backspace, true);
                    m_fakeInput->sendKeyPressed(Qt::Key_Backspace, false);
                }
            }
            return;
        }

        m_input.deleteSurroundingText(index, length);
    });

    const auto sendKeyPressed = [this](int key, bool pressed) {
        if (!m_window || !m_window->isExposed()) {
            return false;
        }

        if (m_fakeInput->shouldUseFakeInput(key) || (!m_input.hasContext() && m_fakeInput->supportsKey(key))) {
            // Send this key over kwin-fake-input instead of input-method-v1 (true keyboard emulation)
            return m_fakeInput->sendKeyPressed(key, pressed);
        }

        const uint sym = keysymForQtKey(key);
        if (sym == XKB_KEY_NoSymbol) {
            return false;
        }

        const uint timestamp = currentTimestamp();
        m_input.keysym(timestamp, sym, pressed ? InputPlugin::Pressed : InputPlugin::Released, 0);
        return true;
    };

    connect(m_vkbdInputBackend, &VkbdWaylandInputBackend::keyClickRequested, this, [sendKeyPressed](int key, bool *handled) {
        if (!handled) {
            return;
        }

        *handled = sendKeyPressed(key, true) && sendKeyPressed(key, false);
    });

    connect(m_vkbdInputBackend, &VkbdWaylandInputBackend::keyPressedRequested, this, [sendKeyPressed](int key, bool pressed, bool *handled) {
        if (!handled) {
            return;
        }

        *handled = sendKeyPressed(key, pressed);
    });

    syncBackendState();
}

QWindow *InputMethodConnection::window() const
{
    return m_window;
}

void InputMethodConnection::setWindow(QWindow *window)
{
    if (m_window == window) {
        return;
    }

    m_window = window;
    updateWindowVisible();
    Q_EMIT windowChanged();
}

bool InputMethodConnection::keyboardNavigationAvailable() const
{
    return m_keyboardNavigationAvailable;
}

void InputMethodConnection::setKeyboardNavigationAvailable(bool available)
{
    if (m_keyboardNavigationAvailable == available) {
        return;
    }

    m_keyboardNavigationAvailable = available;
    Q_EMIT keyboardNavigationAvailableChanged();
}

bool InputMethodConnection::keyboardNavigationActive() const
{
    return m_keyboardNavigationActive;
}

void InputMethodConnection::setKeyboardNavigationActive(bool active)
{
    if (m_keyboardNavigationActive == active) {
        return;
    }

    m_keyboardNavigationActive = active;
    Q_EMIT keyboardNavigationActiveChanged();
}

void InputMethodConnection::hide()
{
    m_visibility.hide();
    m_vkbdInputBackend->reset();
}

bool InputMethodConnection::pinned() const
{
    return m_visibility.pinned();
}

void InputMethodConnection::setPinned(bool pinned)
{
    m_visibility.setPinned(pinned);
}

bool InputMethodConnection::detached() const
{
    return m_visibility.detached();
}

VirtualKeyboardContext *InputMethodConnection::virtualKeyboardContext() const
{
    return m_virtualKeyboardContext;
}

OverlayController *InputMethodConnection::overlayController() const
{
    return m_overlayController;
}

void InputMethodConnection::updateSuggestionSettings()
{
    auto *settings = PlasmaKeyboardSettings::self();
    auto *suggestions = m_virtualKeyboardContext->suggestionController();
    suggestions->setEnabled(settings->suggestionsEnabled());

    using Api = PlasmaKeyboardSettings::EnumSuggestionModelApi;
    const auto api = settings->suggestionModelApi();
    if (api == Api::Disabled) {
        suggestions->setLlmClient(nullptr);
        return;
    }

    if (!m_llmClient) {
        m_llmClient = new LlmCompletionClient(this);
    }
    switch (api) {
    case Api::Ollama:
        m_llmClient->setApi(LlmCompletionClient::Api::Ollama);
        break;
    case Api::OpenAiCompatible:
        m_llmClient->setApi(LlmCompletionClient::Api::OpenAiCompatible);
        break;
    default:
        m_llmClient->setApi(LlmCompletionClient::Api::LlamaCpp);
        break;
    }
    m_llmClient->setEndpoint(QUrl::fromUserInput(settings->suggestionModelEndpoint()));
    m_llmClient->setModel(settings->suggestionModelName());
    m_llmClient->setApiKey(settings->suggestionModelApiKey());
    suggestions->setLlmClient(m_llmClient);
}

void InputMethodConnection::updateVoiceInputSettings()
{
    auto *settings = PlasmaKeyboardSettings::self();
    using Api = PlasmaKeyboardSettings::EnumVoiceInputApi;
    const auto api = settings->voiceInputApi();

    auto *client = m_voiceInput->transcriptionClient();
    client->setApi(api == Api::OpenAiCompatible ? TranscriptionClient::Api::OpenAiCompatible : TranscriptionClient::Api::WhisperCpp);
    client->setEndpoint(QUrl::fromUserInput(settings->voiceInputEndpoint()));
    client->setModel(settings->voiceInputModel());
    client->setApiKey(settings->voiceInputApiKey());
    client->setLanguage(settings->voiceInputLanguage());
    m_voiceInput->setAvailable(api != Api::Disabled);
}

VoiceInput *InputMethodConnection::voiceInput() const
{
    return m_voiceInput;
}

void InputMethodConnection::updateWindowVisible()
{
    if (!m_window) {
        return;
    }

    m_window->setVisible(m_visibility.visible());
}

#include "moc_inputmethodconnection.cpp"
