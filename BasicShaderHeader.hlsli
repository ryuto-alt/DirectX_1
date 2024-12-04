//���_�V�F�[�_�[->�s�N�Z���V�F�[�_�[�ւ̂����Ɏg�p����
//�\����

struct Output
{
    float4 svpos : SV_POSITION; //�V�X�e���p���_���W
    float4 normal : NORMAL;
    float2 uv : TEXCOORD; //uv�l
};
Texture2D<float4> tex : register(t0);
SamplerState smp : register(s0);

cbuffer cbuff0 : register(b0)
{
    matrix world;
    matrix viewproj;
};

//�}�e���A��
cbuffer Material : register(b1)
{
    float4 diffuse;
    float4 specular;
    float3 ambient;
}
