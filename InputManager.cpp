#include "InputManager.h"
#pragma comment(lib, "Xinput.lib")

InputManager::InputManager()
    : m_mousePosition({ 0, 0 })
    , m_mousePositionLastFrame({ 0, 0 })
    , m_mouseDelta({ 0, 0 })
    , m_mouseWheel(0)
    , m_mouseWheelLastFrame(0)
    , m_captureMouse(false)
    , m_hwnd(nullptr)
{
    // キーボード状態の初期化
    m_keys.fill(false);
    m_keysLastFrame.fill(false);

    // マウスボタン状態の初期化
    m_mouseButtons.fill(false);
    m_mouseButtonsLastFrame.fill(false);

    // ゲームパッド状態の初期化
    m_gamepadConnected.fill(false);
    ZeroMemory(m_gamepadStates.data(), sizeof(XINPUT_STATE) * MAX_PLAYERS);
    ZeroMemory(m_gamepadStatesLastFrame.data(), sizeof(XINPUT_STATE) * MAX_PLAYERS);
}

InputManager::~InputManager()
{
    Shutdown();
}

bool InputManager::Initialize(HWND hwnd)
{
    m_hwnd = hwnd;

    // マウス位置の初期化
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(m_hwnd, &cursorPos);
    m_mousePosition = { cursorPos.x, cursorPos.y };
    m_mousePositionLastFrame = m_mousePosition;

    return true;
}

void InputManager::Shutdown()
{
    // ゲームパッドの振動を停止
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (m_gamepadConnected[i]) {
            SetGamepadVibration(0.0f, 0.0f, i);
        }
    }

    m_hwnd = nullptr;
}

void InputManager::Update()
{
    // キーボード状態の保存
    m_keysLastFrame = m_keys;

    // マウスボタン状態の保存
    m_mouseButtonsLastFrame = m_mouseButtons;

    // マウス位置の更新
    m_mousePositionLastFrame = m_mousePosition;
    POINT cursorPos;
    GetCursorPos(&cursorPos);
    ScreenToClient(m_hwnd, &cursorPos);
    m_mousePosition = { cursorPos.x, cursorPos.y };

    // マウス移動量の計算
    m_mouseDelta.x = m_mousePosition.x - m_mousePositionLastFrame.x;
    m_mouseDelta.y = m_mousePosition.y - m_mousePositionLastFrame.y;

    // マウスホイールの状態を保存
    m_mouseWheelLastFrame = m_mouseWheel;

    // ゲームパッド状態の保存
    m_gamepadStatesLastFrame = m_gamepadStates;

    // ゲームパッド状態の更新
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));

        DWORD result = XInputGetState(i, &state);
        if (result == ERROR_SUCCESS) {
            m_gamepadStates[i] = state;
            m_gamepadConnected[i] = true;
        }
        else {
            m_gamepadConnected[i] = false;
        }
    }
}

bool InputManager::IsKeyDown(int keyCode) const
{
    if (keyCode < 0 || keyCode >= 256) {
        return false;
    }

    return m_keys[keyCode];
}

bool InputManager::IsKeyPressed(int keyCode) const
{
    if (keyCode < 0 || keyCode >= 256) {
        return false;
    }

    return m_keys[keyCode] && !m_keysLastFrame[keyCode];
}

bool InputManager::IsKeyReleased(int keyCode) const
{
    if (keyCode < 0 || keyCode >= 256) {
        return false;
    }

    return !m_keys[keyCode] && m_keysLastFrame[keyCode];
}

bool InputManager::IsMouseButtonDown(MouseButton button) const
{
    int index = static_cast<int>(button);
    if (index < 0 || index >= static_cast<int>(MouseButton::Count)) {
        return false;
    }

    return m_mouseButtons[index];
}

bool InputManager::IsMouseButtonPressed(MouseButton button) const
{
    int index = static_cast<int>(button);
    if (index < 0 || index >= static_cast<int>(MouseButton::Count)) {
        return false;
    }

    return m_mouseButtons[index] && !m_mouseButtonsLastFrame[index];
}

bool InputManager::IsMouseButtonReleased(MouseButton button) const
{
    int index = static_cast<int>(button);
    if (index < 0 || index >= static_cast<int>(MouseButton::Count)) {
        return false;
    }

    return !m_mouseButtons[index] && m_mouseButtonsLastFrame[index];
}

DirectX::XMINT2 InputManager::GetMousePosition() const
{
    return m_mousePosition;
}

DirectX::XMINT2 InputManager::GetMouseDelta() const
{
    return m_mouseDelta;
}

int InputManager::GetMouseWheel() const
{
    return m_mouseWheel;
}

int InputManager::GetMouseWheelDelta() const
{
    return m_mouseWheel - m_mouseWheelLastFrame;
}

bool InputManager::IsGamepadConnected(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS) {
        return false;
    }

    return m_gamepadConnected[playerIndex];
}

bool InputManager::IsGamepadButtonDown(GamepadButton button, int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS || !m_gamepadConnected[playerIndex]) {
        return false;
    }

    WORD buttonMask = GetGamepadButtonMask(button);
    return (m_gamepadStates[playerIndex].Gamepad.wButtons & buttonMask) != 0;
}

bool InputManager::IsGamepadButtonPressed(GamepadButton button, int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS || !m_gamepadConnected[playerIndex]) {
        return false;
    }

    WORD buttonMask = GetGamepadButtonMask(button);
    bool isDown = (m_gamepadStates[playerIndex].Gamepad.wButtons & buttonMask) != 0;
    bool wasDown = (m_gamepadStatesLastFrame[playerIndex].Gamepad.wButtons & buttonMask) != 0;

    return isDown && !wasDown;
}

bool InputManager::IsGamepadButtonReleased(GamepadButton button, int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS || !m_gamepadConnected[playerIndex]) {
        return false;
    }

    WORD buttonMask = GetGamepadButtonMask(button);
    bool isDown = (m_gamepadStates[playerIndex].Gamepad.wButtons & buttonMask) != 0;
    bool wasDown = (m_gamepadStatesLastFrame[playerIndex].Gamepad.wButtons & buttonMask) != 0;

    return !isDown && wasDown;
}

DirectX::XMFLOAT2 InputManager::GetGamepadLeftStick(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS || !m_gamepadConnected[playerIndex]) {
        return DirectX::XMFLOAT2(0.0f, 0.0f);
    }

    float normLX = ApplyDeadzone(m_gamepadStates[playerIndex].Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
    float normLY = ApplyDeadzone(m_gamepadStates[playerIndex].Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

    return DirectX::XMFLOAT2(normLX, normLY);
}

DirectX::XMFLOAT2 InputManager::GetGamepadRightStick(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS || !m_gamepadConnected[playerIndex]) {
        return DirectX::XMFLOAT2(0.0f, 0.0f);
    }

    float normRX = ApplyDeadzone(m_gamepadStates[playerIndex].Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
    float normRY = ApplyDeadzone(m_gamepadStates[playerIndex].Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

    return DirectX::XMFLOAT2(normRX, normRY);
}

float InputManager::GetGamepadLeftTrigger(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS || !m_gamepadConnected[playerIndex]) {
        return 0.0f;
    }

    BYTE trigger = m_gamepadStates[playerIndex].Gamepad.bLeftTrigger;
    if (trigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
        return 0.0f;
    }

    return static_cast<float>(trigger) / 255.0f;
}

float InputManager::GetGamepadRightTrigger(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS || !m_gamepadConnected[playerIndex]) {
        return 0.0f;
    }

    BYTE trigger = m_gamepadStates[playerIndex].Gamepad.bRightTrigger;
    if (trigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD) {
        return 0.0f;
    }

    return static_cast<float>(trigger) / 255.0f;
}

void InputManager::SetGamepadVibration(float leftMotor, float rightMotor, int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= MAX_PLAYERS || !m_gamepadConnected[playerIndex]) {
        return;
    }

    XINPUT_VIBRATION vibration;
    ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

    // 0.0～1.0の範囲を0～65535の範囲に変換
    vibration.wLeftMotorSpeed = static_cast<WORD>(leftMotor * 65535.0f);
    vibration.wRightMotorSpeed = static_cast<WORD>(rightMotor * 65535.0f);

    XInputSetState(playerIndex, &vibration);
}

void InputManager::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_KEYDOWN:
        m_keys[wParam & 0xFF] = true;
        break;

    case WM_KEYUP:
        m_keys[wParam & 0xFF] = false;
        break;

    case WM_LBUTTONDOWN:
        m_mouseButtons[static_cast<int>(MouseButton::Left)] = true;
        break;

    case WM_LBUTTONUP:
        m_mouseButtons[static_cast<int>(MouseButton::Left)] = false;
        break;

    case WM_MBUTTONDOWN:
        m_mouseButtons[static_cast<int>(MouseButton::Middle)] = true;
        break;

    case WM_MBUTTONUP:
        m_mouseButtons[static_cast<int>(MouseButton::Middle)] = false;
        break;

    case WM_RBUTTONDOWN:
        m_mouseButtons[static_cast<int>(MouseButton::Right)] = true;
        break;

    case WM_RBUTTONUP:
        m_mouseButtons[static_cast<int>(MouseButton::Right)] = false;
        break;

    case WM_XBUTTONDOWN:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
            m_mouseButtons[static_cast<int>(MouseButton::XButton1)] = true;
        }
        else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2) {
            m_mouseButtons[static_cast<int>(MouseButton::XButton2)] = true;
        }
        break;

    case WM_XBUTTONUP:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) {
            m_mouseButtons[static_cast<int>(MouseButton::XButton1)] = false;
        }
        else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON2) {
            m_mouseButtons[static_cast<int>(MouseButton::XButton2)] = false;
        }
        break;

    case WM_MOUSEWHEEL:
        m_mouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        break;

    case WM_MOUSEMOVE:
        m_mousePosition.x = GET_X_LPARAM(lParam);
        m_mousePosition.y = GET_Y_LPARAM(lParam);
        break;
    }
}

float InputManager::ApplyDeadzone(float value, float deadzone) const
{
    // デッドゾーン処理
    if (value < 0) {
        if (value > -deadzone) {
            return 0.0f;
        }
        else {
            return (value + deadzone) / (32768.0f - deadzone);
        }
    }
    else {
        if (value < deadzone) {
            return 0.0f;
        }
        else {
            return (value - deadzone) / (32767.0f - deadzone);
        }
    }
}

WORD InputManager::GetGamepadButtonMask(GamepadButton button) const
{
    switch (button) {
    case GamepadButton::DPadUp:        return XINPUT_GAMEPAD_DPAD_UP;
    case GamepadButton::DPadDown:      return XINPUT_GAMEPAD_DPAD_DOWN;
    case GamepadButton::DPadLeft:      return XINPUT_GAMEPAD_DPAD_LEFT;
    case GamepadButton::DPadRight:     return XINPUT_GAMEPAD_DPAD_RIGHT;
    case GamepadButton::Start:         return XINPUT_GAMEPAD_START;
    case GamepadButton::Back:          return XINPUT_GAMEPAD_BACK;
    case GamepadButton::LeftThumb:     return XINPUT_GAMEPAD_LEFT_THUMB;
    case GamepadButton::RightThumb:    return XINPUT_GAMEPAD_RIGHT_THUMB;
    case GamepadButton::LeftShoulder:  return XINPUT_GAMEPAD_LEFT_SHOULDER;
    case GamepadButton::RightShoulder: return XINPUT_GAMEPAD_RIGHT_SHOULDER;
    case GamepadButton::A:             return XINPUT_GAMEPAD_A;
    case GamepadButton::B:             return XINPUT_GAMEPAD_B;
    case GamepadButton::X:             return XINPUT_GAMEPAD_X;
    case GamepadButton::Y:             return XINPUT_GAMEPAD_Y;
    default:                           return 0;
    }
}