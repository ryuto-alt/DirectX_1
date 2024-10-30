//頂点シェーダー->ピクセルシェーダーへのやり取りに使用する
//構造体

struct Output
{
    float4 svpos : SV_POSITION; //システム用頂点座標
    float2 uv : TEXCOORD; //uv値
};
Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);
