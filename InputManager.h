#pragma once

#include <Windows.h>
#include <Xinput.h>
#include <unordered_map>
#include <array>
#include <string>
#include <memory>
#include <DirectXMath.h>

// キー入力管理クラス
class InputManager {
public:
    // マウスボタン列挙型
    enum class MouseButton {
        Left,
        Middle,
        Right,
        XButton1,
        XButton2,
        Count
    };

    // ゲームパッドボタン列挙型
    enum class GamepadButton {
        DPadUp,
        DPadDown,
        DPadLeft,
        DPadRight,
        Start,
        Back,
        LeftThumb,
        RightThumb,
        LeftShoulder,
        RightShoulder,
        A,
        B,
        X,
        Y,
        Count
    };

    InputManager();
    ~InputManager();

    // 初期化
    bool Initialize(HWND hwnd);

    // シャットダウン
    void Shutdown();

    // 入力状態の更新
    void Update();

    // キーが押されているか
    bool IsKeyDown(int keyCode) const;

    // キーが現在のフレームで押されたか
    bool IsKeyPressed(int keyCode) const;

    // キーが現在のフレームで離されたか
    bool IsKeyReleased(int keyCode) const;

    // マウスボタンが押されているか
    bool IsMouseButtonDown(MouseButton button) const;

    // マウスボタンが現在のフレームで押されたか
    bool IsMouseButtonPressed(MouseButton button) const;

    // マウスボタンが現在のフレームで離されたか
    bool IsMouseButtonReleased(MouseButton button) const;

    // マウス位置の取得
    DirectX::XMINT2 GetMousePosition() const;

    // マウス移動量の取得
    DirectX::XMINT2 GetMouseDelta() const;

    // マウスホイールの回転量
    int GetMouseWheel() const;

    // マウスホイールの回転量の変化
    int GetMouseWheelDelta() const;

    // ゲームパッドが接続されているか
    bool IsGamepadConnected(int playerIndex = 0) const;

    // ゲームパッドのボタンが押されているか
    bool IsGamepadButtonDown(GamepadButton button, int playerIndex = 0) const;

    // ゲームパッドのボタンが現在のフレームで押されたか
    bool IsGamepadButtonPressed(GamepadButton button, int playerIndex = 0) const;

    // ゲームパッドのボタンが現在のフレームで離されたか
    bool IsGamepadButtonReleased(GamepadButton button, int playerIndex = 0) const;

    // 左スティックの値を取得
    DirectX::XMFLOAT2 GetGamepadLeftStick(int playerIndex = 0) const;

    // 右スティックの値を取得
    DirectX::XMFLOAT2 GetGamepadRightStick(int playerIndex = 0) const;

    // 左トリガーの値を取得（0.0～1.0）
    float GetGamepadLeftTrigger(int playerIndex = 0) const;

    // 右トリガーの値を取得（0.0～1.0）
    float GetGamepadRightTrigger(int playerIndex = 0) const;

    // ゲームパッドの振動を設定
    void SetGamepadVibration(float leftMotor, float rightMotor, int playerIndex = 0);

    // ウィンドウプロシージャからの入力処理
    void ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
    // キーボード状態
    std::array<bool, 256> m_keys;              // 現在のキー状態
    std::array<bool, 256> m_keysLastFrame;     // 前フレームのキー状態

    // マウス状態
    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseButtons;          // 現在のマウスボタン状態
    std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseButtonsLastFrame; // 前フレームのマウスボタン状態
    DirectX::XMINT2 m_mousePosition;          // マウス位置
    DirectX::XMINT2 m_mousePositionLastFrame; // 前フレームのマウス位置
    DirectX::XMINT2 m_mouseDelta;             // マウス移動量
    int m_mouseWheel;                         // マウスホイールの回転量
    int m_mouseWheelLastFrame;                // 前フレームのマウスホイールの回転量

    // ゲームパッド状態（最大4プレイヤー）
    static const int MAX_PLAYERS = 4;
    std::array<XINPUT_STATE, MAX_PLAYERS> m_gamepadStates;                              // 現在のゲームパッド状態
    std::array<XINPUT_STATE, MAX_PLAYERS> m_gamepadStatesLastFrame;                     // 前フレームのゲームパッド状態
    std::array<bool, MAX_PLAYERS> m_gamepadConnected;                                   // ゲームパッド接続状態

    // マウスの捕捉状態
    bool m_captureMouse;

    // ウィンドウハンドル
    HWND m_hwnd;

    // デッドゾーン処理
    float ApplyDeadzone(float value, float deadzone) const;

    // ゲームパッドボタンのXInputビット値を取得
    WORD GetGamepadButtonMask(GamepadButton button) const;
};