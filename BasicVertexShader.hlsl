#include"BasicShaderHeader.hlsli"

Output BasicVS(float4 pos : POSITION, float2 uv : TEXCOORD)
{
    Output output; //�s�N�Z���V�F�[�_�[�ɂ킽���l
    output.svpos = mul(mat,pos);
    output.uv = uv;
    return output;

}