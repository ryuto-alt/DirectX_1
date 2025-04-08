#pragma once

#include <xaudio2.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <DirectXTex.h>
#include <wrl/client.h>

struct AudioClip;
class ResourceManager;

// オーディオパラメータ構造体
struct AudioParameters {
    float volume;           // 音量（0.0～1.0）
    float pitch;            // ピッチ（0.5～2.0）
    float pan;              // パン（-1.0～1.0、左～右）
    bool loop;              // ループフラグ

    AudioParameters()
        : volume(1.0f)
        , pitch(1.0f)
        , pan(0.0f)
        , loop(false)
    {
    }
};

// オーディオソースクラス
class AudioSource {
public:
    AudioSource();
    ~AudioSource();

    // オーディオソースの初期化
    bool Initialize(IXAudio2* xAudio, const std::shared_ptr<AudioClip>& clip);

    // 再生
    void Play();

    // 停止
    void Stop();

    // 一時停止
    void Pause();

    // 再開
    void Resume();

    // 音量の設定
    void SetVolume(float volume);

    // ピッチの設定
    void SetPitch(float pitch);

    // パンの設定
    void SetPan(float pan);

    // ループの設定
    void SetLoop(bool loop);

    // 再生中かどうか
    bool IsPlaying() const;

    // 再生位置の取得（秒）
    float GetPosition() const;

    // 再生位置の設定（秒）
    void SetPosition(float seconds);

    // 長さの取得（秒）
    float GetLength() const;

    // オーディオクリップの取得
    std::shared_ptr<AudioClip> GetClip() const;

private:
    IXAudio2* m_xAudio;                                        // XAudio2インターフェース
    std::shared_ptr<AudioClip> m_clip;                         // オーディオクリップ
    Microsoft::WRL::ComPtr<IXAudio2SourceVoice> m_sourceVoice; // ソースボイス
    XAUDIO2_BUFFER m_buffer;                                   // オーディオバッファ
    AudioParameters m_parameters;                              // オーディオパラメータ
    bool m_isPlaying;                                          // 再生中フラグ
    bool m_isPaused;                                           // 一時停止フラグ
};

// オーディオリスナークラス
class AudioListener {
public:
    AudioListener();
    ~AudioListener() = default;

    // 位置の設定
    void SetPosition(float x, float y, float z);

    // 前方向の設定
    void SetForward(float x, float y, float z);

    // 上方向の設定
    void SetUp(float x, float y, float z);

    // 速度の設定
    void SetVelocity(float x, float y, float z);

    // 位置の取得
    DirectX::XMFLOAT3 GetPosition() const;

    // 前方向の取得
    DirectX::XMFLOAT3 GetForward() const;

    // 上方向の取得
    DirectX::XMFLOAT3 GetUp() const;

    // 速度の取得
    DirectX::XMFLOAT3 GetVelocity() const;

private:
    DirectX::XMFLOAT3 m_position;  // 位置
    DirectX::XMFLOAT3 m_forward;   // 前方向
    DirectX::XMFLOAT3 m_up;        // 上方向
    DirectX::XMFLOAT3 m_velocity;  // 速度
};

// XAudio2コールバッククラス
class AudioCallback : public IXAudio2VoiceCallback {
public:
    AudioCallback() = default;
    ~AudioCallback() = default;

    // ボイスの処理パスが終了したときに呼び出される
    void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override {}

    // ボイスの処理パスが終了したときに呼び出される
    void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}

    // バッファストリームの再生が終了したときに呼び出される
    void STDMETHODCALLTYPE OnStreamEnd() override {}

    // バッファが開始されたときに呼び出される
    void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override {}

    // バッファの再生が終了したときに呼び出される
    void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override;

    // ループの再生位置に達したときに呼び出される
    void STDMETHODCALLTYPE OnLoopEnd(void* pBufferContext) override {}

    // ボイスエラーが発生したときに呼び出される
    void STDMETHODCALLTYPE OnVoiceError(void* pBufferContext, HRESULT Error) override {}

    // コールバック関数の登録
    void RegisterCallback(const std::function<void(void*)>& callback);

private:
    std::function<void(void*)> m_callback;
};

// オーディオシステムクラス
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    // 初期化
    bool Initialize();

    // シャットダウン
    void Shutdown();

    // 更新
    void Update();

    // オーディオソースの作成
    std::shared_ptr<AudioSource> CreateAudioSource(const std::wstring& clipPath);

    // オーディオソースの作成（既存のクリップから）
    std::shared_ptr<AudioSource> CreateAudioSource(const std::shared_ptr<AudioClip>& clip);

    // オーディオリスナーの取得
    AudioListener* GetListener();

    // マスターボリュームの設定
    void SetMasterVolume(float volume);

    // マスターボリュームの取得
    float GetMasterVolume() const;

    // ミュート設定
    void SetMute(bool mute);

    // ミュート状態の取得
    bool IsMuted() const;

    // リソースマネージャーの設定
    void SetResourceManager(ResourceManager* resourceManager);

    // XAudio2インターフェースの取得
    IXAudio2* GetXAudio() const;

    // マスターボイスの取得
    IXAudio2MasteringVoice* GetMasteringVoice() const;

private:
    Microsoft::WRL::ComPtr<IXAudio2> m_xAudio;                 // XAudio2インターフェース
    IXAudio2MasteringVoice* m_masteringVoice;                  // マスターボイス
    ResourceManager* m_resourceManager;                         // リソースマネージャー
    AudioListener m_listener;                                   // オーディオリスナー
    std::vector<std::shared_ptr<AudioSource>> m_audioSources;   // オーディオソースリスト
    AudioCallback m_callback;                                   // オーディオコールバック
    float m_masterVolume;                                       // マスターボリューム
    bool m_mute;                                                // ミュートフラグ
};