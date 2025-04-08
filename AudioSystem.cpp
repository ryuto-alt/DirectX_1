#include "AudioSystem.h"
#include "ResourceManager.h"
#include <cassert>

// ComベースコンポーネントのCOMライブラリを使用するためのリンク
#pragma comment(lib, "xaudio2.lib")

//
// AudioSource クラスの実装
//
AudioSource::AudioSource()
    : m_xAudio(nullptr)
    , m_clip(nullptr)
    , m_sourceVoice(nullptr)
    , m_isPlaying(false)
    , m_isPaused(false)
{
    ZeroMemory(&m_buffer, sizeof(XAUDIO2_BUFFER));
}

AudioSource::~AudioSource()
{
    if (m_sourceVoice) {
        m_sourceVoice->Stop();
        m_sourceVoice->FlushSourceBuffers();
        m_sourceVoice->DestroyVoice();
        m_sourceVoice = nullptr;
    }
}

bool AudioSource::Initialize(IXAudio2* xAudio, const std::shared_ptr<AudioClip>& clip)
{
    assert(xAudio != nullptr);
    assert(clip != nullptr);

    m_xAudio = xAudio;
    m_clip = clip;

    // WAVEフォーマットを設定
    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = clip->channelCount;
    waveFormat.nSamplesPerSec = clip->sampleRate;
    waveFormat.wBitsPerSample = clip->bitsPerSample;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    // ソースボイスの作成
    HRESULT hr = m_xAudio->CreateSourceVoice(
        m_sourceVoice.GetAddressOf(),
        &waveFormat,
        0,
        XAUDIO2_DEFAULT_FREQ_RATIO,
        nullptr,
        nullptr,
        nullptr
    );

    if (FAILED(hr)) {
        return false;
    }

    // オーディオバッファの設定
    m_buffer.AudioBytes = static_cast<UINT32>(clip->data.size());
    m_buffer.pAudioData = clip->data.data();
    m_buffer.Flags = XAUDIO2_END_OF_STREAM;
    m_buffer.LoopCount = 0;

    return true;
}

void AudioSource::Play()
{
    if (!m_sourceVoice) {
        return;
    }

    // 既に再生中の場合は一度停止
    if (m_isPlaying) {
        Stop();
    }

    // ループフラグの設定
    m_buffer.LoopCount = m_parameters.loop ? XAUDIO2_LOOP_INFINITE : 0;

    // バッファの送信
    HRESULT hr = m_sourceVoice->SubmitSourceBuffer(&m_buffer);
    if (FAILED(hr)) {
        return;
    }

    // 再生開始
    hr = m_sourceVoice->Start();
    if (SUCCEEDED(hr)) {
        m_isPlaying = true;
        m_isPaused = false;
    }
}

void AudioSource::Stop()
{
    if (!m_sourceVoice || !m_isPlaying) {
        return;
    }

    m_sourceVoice->Stop();
    m_sourceVoice->FlushSourceBuffers();
    m_isPlaying = false;
    m_isPaused = false;
}

void AudioSource::Pause()
{
    if (!m_sourceVoice || !m_isPlaying || m_isPaused) {
        return;
    }

    m_sourceVoice->Stop();
    m_isPaused = true;
}

void AudioSource::Resume()
{
    if (!m_sourceVoice || !m_isPlaying || !m_isPaused) {
        return;
    }

    m_sourceVoice->Start();
    m_isPaused = false;
}

void AudioSource::SetVolume(float volume)
{
    m_parameters.volume = volume;

    if (m_sourceVoice) {
        m_sourceVoice->SetVolume(volume);
    }
}

void AudioSource::SetPitch(float pitch)
{
    // ピッチの範囲を制限（XAudio2のFrequencyRatioは0.5～2.0の範囲）
    pitch = std::max(0.5f, std::min(pitch, 2.0f));
    m_parameters.pitch = pitch;

    if (m_sourceVoice) {
        m_sourceVoice->SetFrequencyRatio(pitch);
    }
}

void AudioSource::SetPan(float pan)
{
    // パンの範囲を制限（-1.0～1.0）
    pan = std::max(-1.0f, std::min(pan, 1.0f));
    m_parameters.pan = pan;

    if (m_sourceVoice) {
        // パンの実装（左右のチャンネルの音量を調整）
        float left = (pan <= 0.0f) ? 1.0f : (1.0f - pan);
        float right = (pan >= 0.0f) ? 1.0f : (1.0f + pan);

        XAUDIO2_VOICE_DETAILS details;
        m_sourceVoice->GetVoiceDetails(&details);

        // 出力チャンネルマスクの設定
        if (details.InputChannels == 1) {
            // モノラル入力の場合
            float outputMatrix[2] = { left, right };
            m_sourceVoice->SetOutputMatrix(nullptr, 1, 2, outputMatrix);
        }
        else if (details.InputChannels == 2) {
            // ステレオ入力の場合
            float outputMatrix[4] = { left, 0.0f, 0.0f, right };
            m_sourceVoice->SetOutputMatrix(nullptr, 2, 2, outputMatrix);
        }
    }
}

void AudioSource::SetLoop(bool loop)
{
    m_parameters.loop = loop;

    // 再生中の場合はループ設定を更新
    if (m_isPlaying && m_sourceVoice) {
        Stop();
        Play();
    }
}

bool AudioSource::IsPlaying() const
{
    return m_isPlaying && !m_isPaused;
}

float AudioSource::GetPosition() const
{
    if (!m_sourceVoice || !m_clip) {
        return 0.0f;
    }

    XAUDIO2_VOICE_STATE state;
    m_sourceVoice->GetState(&state);

    // 再生サンプル数から秒数を計算
    return static_cast<float>(state.SamplesPlayed) / static_cast<float>(m_clip->sampleRate);
}

void AudioSource::SetPosition(float seconds)
{
    if (!m_sourceVoice || !m_clip) {
        return;
    }

    // 現在の再生状態を保存
    bool wasPlaying = IsPlaying();

    // 一度停止
    Stop();

    // サンプル位置を計算
    UINT32 samplePosition = static_cast<UINT32>(seconds * m_clip->sampleRate);

    // バッファの再設定
    m_buffer.PlayBegin = samplePosition;
    m_buffer.PlayLength = 0; // 0=最後まで再生

    // 再生状態だった場合は再開
    if (wasPlaying) {
        Play();
    }
}

float AudioSource::GetLength() const
{
    if (!m_clip) {
        return 0.0f;
    }

    // データサイズからサンプル数を計算し、秒数に変換
    UINT32 numSamples = static_cast<UINT32>(m_clip->data.size()) /
        (m_clip->channelCount * m_clip->bitsPerSample / 8);

    return static_cast<float>(numSamples) / static_cast<float>(m_clip->sampleRate);
}

std::shared_ptr<AudioClip> AudioSource::GetClip() const
{
    return m_clip;
}

//
// AudioListener クラスの実装
//
AudioListener::AudioListener()
    : m_position(0.0f, 0.0f, 0.0f)
    , m_forward(0.0f, 0.0f, 1.0f)
    , m_up(0.0f, 1.0f, 0.0f)
    , m_velocity(0.0f, 0.0f, 0.0f)
{
}

void AudioListener::SetPosition(float x, float y, float z)
{
    m_position = { x, y, z };
}

void AudioListener::SetForward(float x, float y, float z)
{
    m_forward = { x, y, z };
}

void AudioListener::SetUp(float x, float y, float z)
{
    m_up = { x, y, z };
}

void AudioListener::SetVelocity(float x, float y, float z)
{
    m_velocity = { x, y, z };
}

DirectX::XMFLOAT3 AudioListener::GetPosition() const
{
    return m_position;
}

DirectX::XMFLOAT3 AudioListener::GetForward() const
{
    return m_forward;
}

DirectX::XMFLOAT3 AudioListener::GetUp() const
{
    return m_up;
}

DirectX::XMFLOAT3 AudioListener::GetVelocity() const
{
    return m_velocity;
}

//
// AudioCallback クラスの実装
//
void STDMETHODCALLTYPE AudioCallback::OnBufferEnd(void* pBufferContext)
{
    // 再生終了時のコールバックを呼び出す
    if (m_callback) {
        m_callback(pBufferContext);
    }
}

void AudioCallback::RegisterCallback(const std::function<void(void*)>& callback)
{
    m_callback = callback;
}

//
// AudioSystem クラスの実装
//
AudioSystem::AudioSystem()
    : m_xAudio(nullptr)
    , m_masteringVoice(nullptr)
    , m_resourceManager(nullptr)
    , m_masterVolume(1.0f)
    , m_mute(false)
{
}

AudioSystem::~AudioSystem()
{
    Shutdown();
}

bool AudioSystem::Initialize()
{
    // COMの初期化
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        return false;
    }

    // XAudio2インターフェースの作成
    hr = XAudio2Create(m_xAudio.GetAddressOf(), 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        CoUninitialize();
        return false;
    }

    // マスターボイスの作成
    hr = m_xAudio->CreateMasteringVoice(&m_masteringVoice);
    if (FAILED(hr)) {
        m_xAudio.Reset();
        CoUninitialize();
        return false;
    }

    // マスターボリュームの設定
    m_masteringVoice->SetVolume(m_masterVolume);

    // コールバックの登録
    m_callback.RegisterCallback([this](void* context) {
        // バッファ再生終了時の処理
        // 現在は特に何もしない
        });

    return true;
}

void AudioSystem::Shutdown()
{
    // すべてのオーディオソースを解放
    m_audioSources.clear();

    // マスターボイスの解放
    if (m_masteringVoice) {
        m_masteringVoice->DestroyVoice();
        m_masteringVoice = nullptr;
    }

    // XAudio2の解放
    m_xAudio.Reset();

    // COMの終了処理
    CoUninitialize();
}

void AudioSystem::Update()
{
    // 非アクティブなオーディオソースの削除
    m_audioSources.erase(
        std::remove_if(
            m_audioSources.begin(),
            m_audioSources.end(),
            [](const std::shared_ptr<AudioSource>& source) {
                return source.use_count() == 1; // AudioSystemだけが参照している場合
            }
        ),
        m_audioSources.end()
    );
}

std::shared_ptr<AudioSource> AudioSystem::CreateAudioSource(const std::wstring& clipPath)
{
    if (!m_resourceManager) {
        return nullptr;
    }

    // オーディオクリップのロード
    std::shared_ptr<AudioClip> clip = m_resourceManager->LoadAudio(clipPath);
    if (!clip) {
        return nullptr;
    }

    return CreateAudioSource(clip);
}

std::shared_ptr<AudioSource> AudioSystem::CreateAudioSource(const std::shared_ptr<AudioClip>& clip)
{
    if (!clip) {
        return nullptr;
    }

    // オーディオソースの作成
    std::shared_ptr<AudioSource> source = std::make_shared<AudioSource>();
    if (!source->Initialize(m_xAudio.Get(), clip)) {
        return nullptr;
    }

    // リストに追加
    m_audioSources.push_back(source);

    return source;
}

AudioListener* AudioSystem::GetListener()
{
    return &m_listener;
}

void AudioSystem::SetMasterVolume(float volume)
{
    m_masterVolume = volume;

    if (m_masteringVoice) {
        m_masteringVoice->SetVolume(m_mute ? 0.0f : m_masterVolume);
    }
}

float AudioSystem::GetMasterVolume() const
{
    return m_masterVolume;
}

void AudioSystem::SetMute(bool mute)
{
    m_mute = mute;

    if (m_masteringVoice) {
        m_masteringVoice->SetVolume(m_mute ? 0.0f : m_masterVolume);
    }
}

bool AudioSystem::IsMuted() const
{
    return m_mute;
}

void AudioSystem::SetResourceManager(ResourceManager* resourceManager)
{
    m_resourceManager = resourceManager;
}

IXAudio2* AudioSystem::GetXAudio() const
{
    return m_xAudio.Get();
}

IXAudio2MasteringVoice* AudioSystem::GetMasteringVoice() const
{
    return m_masteringVoice;
}