struct VS_input
{
	float2 pos : POSITION;
};

float4 VS_main(in VS_input input) : SV_POSITION
{
	return float4( input.pos, 0.0f, 1.0f );
}