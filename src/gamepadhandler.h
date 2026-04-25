#pragma once
#include <QObject>

class QTimer;

class GamepadHandler : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit GamepadHandler(QObject *parent = nullptr);
    ~GamepadHandler();

    bool isConnected() const;

    Q_INVOKABLE void sendKey(int qtKey);

Q_SIGNALS:
    void aPressed();
    void bPressed();
    void xPressed();
    void yPressed();
    void selectPressed();
    void startPressed();
    void dpadUp();
    void dpadDown();
    void dpadLeft();
    void dpadRight();
    void connectedChanged();

private:
#ifdef HAVE_SDL2
    bool m_connected = false;
    QTimer *m_pollTimer = nullptr;
    QTimer *m_axisRepeatTimer = nullptr;
    int m_leftX = 0, m_leftY = 0;
    int m_rightX = 0, m_rightY = 0;
    int m_axisDirLast = 0;
    static constexpr int kDeadzone = 8000;
    void pollEvents();
    int axisDirection() const;
    void emitAxisDir(int dir);
    void onAxisRepeat();
#endif
};
