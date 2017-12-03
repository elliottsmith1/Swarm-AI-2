#define NUM_LEAVES_PER_TREE 100

cbuffer cbPerScene
{
	float4x4 leafOnTree[NUM_LEAVES_PER_TREE];
};

cbuffer cbPerObject
{
	float4x4 WVP;
	float4x4 World;

	float4 difColor;
	bool hasTexture;
	bool hasNormMap;

	bool isInstance;
	bool isLeaf;

};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};


VS_OUTPUT VS(float4 inPos : POSITION, float4 inColor : COLOR, float3 instancePos : INSTANCEPOS, uint instanceID : SV_InstanceID)
{
	VS_OUTPUT output;

	//if (isInstance)
	//{
	//	uint currTree = (instanceID / NUM_LEAVES_PER_TREE);
	//	uint currLeafInTree = instanceID - (currTree * NUM_LEAVES_PER_TREE);
	//	inPos = mul(inPos, leafOnTree[currLeafInTree]);

	//	// set position using instance data
	//	inPos += float4(instancePos, 0.0f);
	//}

    output.Pos = mul(inPos, WVP);
	//output.worldPos = mul(inPos, World);
    output.Color = inColor;

    return output;
}

float4 PS(VS_OUTPUT input) : SV_TARGET
{
    return input.Color;
}