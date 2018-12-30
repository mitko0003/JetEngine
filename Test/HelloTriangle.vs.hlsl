float4 VS_main(in uint VertexIdx : SV_VertexID) : SV_POSITION
{
	float2 position[3] = {
		float2(-0.7f, 0.7f), 
		float2( 0.7f, 0.7f), 
		float2( 0.0f,-0.7f)
	};

	return float4( position[VertexIdx], 0.0f, 1.0f );
}