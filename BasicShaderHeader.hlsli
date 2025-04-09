// 定数バッファ
cbuffer cbuff0 : register(b0)
{
    matrix mat; // 変換行列
};

// テクスチャとサンプラー
Texture2D tex : register(t0);
SamplerState smp : register(s0);

// 頂点シェーダーの出力構造体
struct Output
{
    float4 svpos : SV_POSITION;
    float2 uv : TEXCOORD;
};