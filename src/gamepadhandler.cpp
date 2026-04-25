#include "gamepadhandler.h"
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QWindow>

#ifdef HAVE_SDL2
#include <QLibrary>
#include <QTimer>
#include <SDL2/SDL.h>

// Runtime-loaded SDL2 function pointers
static int (*pSDL_Init)(Uint32) = nullptr;
static void (*pSDL_Quit)() = nullptr;
static int (*pSDL_NumJoysticks)() = nullptr;
static SDL_bool (*pSDL_IsGameController)(int) = nullptr;
static SDL_GameController *(*pSDL_GameControllerOpen)(int) = nullptr;
static int (*pSDL_PollEvent)(SDL_Event *) = nullptr;
static const char *(*pSDL_GetError)() = nullptr;

static bool loadSDL2()
{
    static bool tried = false;
    static bool ok = false;
    if (tried)
        return ok;
    tried = true;

    QLibrary lib;
    for (const char *name : {"SDL2-2.0", "SDL2", "libSDL2-2.0.so.0"}) {
        lib.setFileName(QLatin1String(name));
        if (lib.load())
            break;
    }
    if (!lib.isLoaded())
        return false;

#define LOAD(fn) p##fn = reinterpret_cast<decltype(p##fn)>(lib.resolve(#fn))
    LOAD(SDL_Init);
    LOAD(SDL_Quit);
    LOAD(SDL_NumJoysticks);
    LOAD(SDL_IsGameController);
    LOAD(SDL_GameControllerOpen);
    LOAD(SDL_PollEvent);
    LOAD(SDL_GetError);
#undef LOAD

    ok = pSDL_Init && pSDL_Quit && pSDL_NumJoysticks && pSDL_IsGameController && pSDL_GameControllerOpen && pSDL_PollEvent && pSDL_GetError;
    return ok;
}
#endif

GamepadHandler::GamepadHandler(QObject *parent)
    : QObject(parent)
{
#ifdef HAVE_SDL2
    if (!loadSDL2()) {
        qInfo() << "SDL2 not available — gamepad support disabled";
        return;
    }

    if (pSDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        qWarning() << "SDL2 gamepad init failed:" << pSDL_GetError();
        return;
    }

    for (int i = 0; i < pSDL_NumJoysticks(); ++i) {
        if (pSDL_IsGameController(i) && pSDL_GameControllerOpen(i)) {
            m_connected = true;
            Q_EMIT connectedChanged();
        }
    }

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(16);
    connect(m_pollTimer, &QTimer::timeout, this, &GamepadHandler::pollEvents);
    m_pollTimer->start();

    m_axisRepeatTimer = new QTimer(this);
    m_axisRepeatTimer->setSingleShot(false);
    connect(m_axisRepeatTimer, &QTimer::timeout, this, &GamepadHandler::onAxisRepeat);
#endif
}

GamepadHandler::~GamepadHandler()
{
#ifdef HAVE_SDL2
    if (pSDL_Quit)
        pSDL_Quit();
#endif
}

bool GamepadHandler::isConnected() const
{
#ifdef HAVE_SDL2
    return m_connected;
#else
    return false;
#endif
}

void GamepadHandler::sendKey(int qtKey)
{
    QWindow *window = QGuiApplication::focusWindow();
    if (!window)
        return;
    QKeyEvent press(QEvent::KeyPress, qtKey, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &press);
    QKeyEvent release(QEvent::KeyRelease, qtKey, Qt::NoModifier);
    QCoreApplication::sendEvent(window, &release);
}

#ifdef HAVE_SDL2
void GamepadHandler::pollEvents()
{
    SDL_Event ev;
    while (pSDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_CONTROLLERDEVICEADDED:
            pSDL_GameControllerOpen(ev.cdevice.which);
            if (!m_connected) {
                m_connected = true;
                Q_EMIT connectedChanged();
            }
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            m_connected = pSDL_NumJoysticks() > 0;
            Q_EMIT connectedChanged();
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            switch (ev.cbutton.button) {
            case SDL_CONTROLLER_BUTTON_A:
                Q_EMIT aPressed();
                break;
            case SDL_CONTROLLER_BUTTON_B:
                Q_EMIT bPressed();
                break;
            case SDL_CONTROLLER_BUTTON_X:
                Q_EMIT xPressed();
                break;
            case SDL_CONTROLLER_BUTTON_Y:
                Q_EMIT yPressed();
                break;
            case SDL_CONTROLLER_BUTTON_BACK:
                Q_EMIT selectPressed();
                break;
            case SDL_CONTROLLER_BUTTON_START:
                Q_EMIT startPressed();
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                Q_EMIT dpadUp();
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                Q_EMIT dpadDown();
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                Q_EMIT dpadLeft();
                break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                Q_EMIT dpadRight();
                break;
            }
            break;
        case SDL_CONTROLLERAXISMOTION:
            switch (ev.caxis.axis) {
            case SDL_CONTROLLER_AXIS_LEFTX:
                m_leftX = ev.caxis.value;
                break;
            case SDL_CONTROLLER_AXIS_LEFTY:
                m_leftY = ev.caxis.value;
                break;
            case SDL_CONTROLLER_AXIS_RIGHTX:
                m_rightX = ev.caxis.value;
                break;
            case SDL_CONTROLLER_AXIS_RIGHTY:
                m_rightY = ev.caxis.value;
                break;
            }
            break;
        }
    }

    int dir = axisDirection();
    if (dir != m_axisDirLast) {
        m_axisDirLast = dir;
        m_axisRepeatTimer->stop();
        if (dir != 0) {
            emitAxisDir(dir);
            m_axisRepeatTimer->setInterval(400);
            m_axisRepeatTimer->start();
        }
    }
}

int GamepadHandler::axisDirection() const
{
    int x = qAbs(m_leftX) > qAbs(m_rightX) ? m_leftX : m_rightX;
    int y = qAbs(m_leftY) > qAbs(m_rightY) ? m_leftY : m_rightY;
    if (qAbs(x) <= kDeadzone && qAbs(y) <= kDeadzone)
        return 0;
    if (qAbs(x) >= qAbs(y))
        return x > 0 ? 4 : 3;
    return y > 0 ? 2 : 1;
}

void GamepadHandler::emitAxisDir(int dir)
{
    switch (dir) {
    case 1:
        Q_EMIT dpadUp();
        break;
    case 2:
        Q_EMIT dpadDown();
        break;
    case 3:
        Q_EMIT dpadLeft();
        break;
    case 4:
        Q_EMIT dpadRight();
        break;
    }
}

void GamepadHandler::onAxisRepeat()
{
    int dir = axisDirection();
    if (dir == 0) {
        m_axisRepeatTimer->stop();
        m_axisDirLast = 0;
        return;
    }
    m_axisDirLast = dir;
    emitAxisDir(dir);
    if (m_axisRepeatTimer->interval() != 150) {
        m_axisRepeatTimer->setInterval(150);
        m_axisRepeatTimer->start();
    }
}
#endif
