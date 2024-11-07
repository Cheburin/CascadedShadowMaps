cbuffer cbMain : register( b0 )
{
	matrix    g_mWorld;                         // World matrix
	matrix    g_mView;                          // View matrix
	matrix    g_mProjection;                    // Projection matrix
	matrix    g_mWorldViewProjection;           // WVP matrix
	matrix    g_mWorldView;                     // WV matrix
	matrix    g_mInvView;                       // Inverse of view matrix

	matrix    g_mObject1;                // VP matrix
	matrix    g_mObject1WorldView;                       // Inverse of view matrix
	matrix    g_mObject1WorldViewProjection;                       // Inverse of view matrix

	matrix    g_mObject2;                // VP matrix
	matrix    g_mObject2WorldView;                       // Inverse of view matrix
	matrix    g_mObject2WorldViewProjection;                       // Inverse of view matrix

	float4    g_vFrustumNearFar;              // Screen resolution
	float4    g_vFrustumParams;              // Screen resolution
	float4    g_viewLightPos;                   //

	matrix    light_camera1_projecion;                       // Inverse of view matrix
	matrix    light_camera1_view; 
	matrix    light_camera1_inv_view;

	float4    vDebugIndex;
	float4    vDebugColor;
};

struct PosNormalTex2d
{
//    float3 pos : SV_Position;
//    float3 normal   : NORMAL;
//    float2 tex      : TEXCOORD0;
    float3 pos : SV_Position;
    float3 normal   : NORMAL;
	float4 tangent  : TANGENT;
	float4 color    : COLOR0;
    float2 tex      : TEXCOORD0;
};

struct PosNormalTangentTex2d
{
    float3 pos : SV_Position;
    float3 normal   : NORMAL;
	float4 tangent  : TANGENT;
	float4 color    : COLOR0;
    float2 tex      : TEXCOORD0;
};

struct ClipPosNormalTangentTex2d
{
    float4 pos : SV_POSITION;
    float3 normal   : NORMAL;
	float4 tangent  : TANGENT;
	float4 color    : COLOR0;
    float2 tex      : TEXCOORD0;
};

struct ClipPosColor
{
    float4 color          : TEXCOORD1;
    float4 clip_pos       : SV_POSITION; // Output position
};

struct PosNormTex2d
{
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 tex    : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

struct TESTSCENE_VS_OUTPUT
{
    float2 tex            : TEXCOORD0;
	float3 normal         : TEXCOORD1;
	float3 light_dir      : TEXCOORD2;
	float3 position       : TEXCOORD3;
	float  vDepth		  : TEXCOORD4;
	float3 vTexShadow	  : TEXCOORD5;
    float4 clip_pos       : SV_POSITION; // Output position
};

TESTSCENE_VS_OUTPUT TESTSCENE_VS( in PosNormTex2d i )
{
    TESTSCENE_VS_OUTPUT output;

    output.tex = i.tex;

	output.normal = mul( float4( i.normal, 0.0 ), g_mWorld).xyz;
    
	output.light_dir = -1.0*(light_camera1_inv_view._m20_m21_m22);

    output.position = i.pos;

	output.vDepth =     mul( float4( i.pos, 1.0 ), g_mWorldView ).z ; 

    output.clip_pos =   mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );

	output.vTexShadow = mul( float4( i.pos, 1.0 ), g_mObject2WorldView ).xyz;

    return output;
}; 

struct DEPTH_TESTSCENE_VS_OUTPUT
{
     float4 clip_pos       : SV_POSITION; // Output position
};

DEPTH_TESTSCENE_VS_OUTPUT DEPTH_TESTSCENE_VS( in PosNormTex2d i )
{
    DEPTH_TESTSCENE_VS_OUTPUT output;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mObject1WorldViewProjection );
    
    return output;
}; 

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

struct GROUND_VS_OUTPUT
{
    float2 tex            : TEXCOORD1;
    float4 clip_pos       : SV_POSITION; // Output position
};

GROUND_VS_OUTPUT GROUND_VS( in PosNormalTex2d i )
{
    GROUND_VS_OUTPUT output;

    output.tex = i.tex;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );
    
    return output;
}; 
///////////////////////////////////////////////////////////////////////////////////////////////////

struct SKY_VS_OUTPUT
{
	float3 tex            : TEXCOORD1;
    float4 clip_pos       : SV_POSITION; // Output position
};

SKY_VS_OUTPUT SKY_VS( in PosNormalTex2d i )
{
    SKY_VS_OUTPUT output;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection ).xyww;
    
	output.tex = i.pos;

    return output;
}; 

/////////////////////////////////////////////////////////////////////////////////////////////////////

struct BOX_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION; // Output position
};

BOX_VS_OUTPUT BOX_VS(uint VertexID : SV_VERTEXID)
{
	BOX_VS_OUTPUT output;

	output.clip_pos = float4(0,0,0,1);

    return output;	
}

static const float4 cubeVerts[8] = 
{
	float4(0, 0, 0, 1),// LB  0
	float4(0, 1, 0, 1), // LT  1
	float4(1, 0, 0, 1), // RB  2
	float4(1, 1, 0, 1),  // RT  3
	float4(0, 0, 1, 1), // LB  4
	float4(0, 1, 1, 1),  // LT  5
	float4(1, 0, 1, 1),  // RB  6
	float4(1, 1, 1, 1)    // RT  7
};

static const int cubeIndices[24] =
{
	0, 1, 2, 3, // front
	7, 6, 3, 2, // right
	7, 5, 6, 4, // back
	4, 0, 6, 2, // bottom
	1, 0, 5, 4, // left
	3, 1, 7, 5  // top
};

[maxvertexcount(36)]
void BOX_GS(point BOX_VS_OUTPUT pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<BOX_VS_OUTPUT> triStream )
{
	BOX_VS_OUTPUT v[8];
	[unroll]
	for (int j = 0; j < 8; j++)
	{
		v[j].clip_pos = mul(cubeVerts[j], g_mWorldViewProjection);
	}
	
	[unroll]
	for (int i = 0; i < 6; i++)
	{
		triStream.Append(v[cubeIndices[i * 4 + 0]]);
		triStream.Append(v[cubeIndices[i * 4 + 1]]);
		triStream.Append(v[cubeIndices[i * 4 + 2]]);
		triStream.Append(v[cubeIndices[i * 4 + 3]]);
		triStream.RestartStrip();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

struct  FIGURE_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION;
	float3 t       		  : TEXCOORD0;
	float3 b       		  : TEXCOORD1;
	float3 n       		  : TEXCOORD2;
	float2 uv      	   	  : TEXCOORD3;

	float3 lightPos       : TEXCOORD4;
	float3 vertexPos      : TEXCOORD5;

	float3 vertexPosRelToLight : TEXCOORD6;
	float4 renderDepthPass     : TEXCOORD7;
};

FIGURE_VS_OUTPUT FIGURE_VS( in PosNormalTangentTex2d i )
{
    FIGURE_VS_OUTPUT output;

    float3 lightPos = float3( 0, 10, 20 );

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );

	output.vertexPos = mul( float4( i.pos, 1.0 ), g_mWorldView ).xyz;

	output.vertexPosRelToLight = mul( float4( i.pos, 1.0 ), g_mWorld ).xyz - lightPos;

	output.lightPos = mul( float4( lightPos, 1.0 ), g_mView ).xyz;
    
	output.n = normalize(mul(float4(i.normal.xyz,  0), g_mWorldView).xyz);
	output.t = normalize(mul(float4(i.tangent.xyz, 0), g_mWorldView).xyz);
	output.b = normalize(cross(output.n, output.t))*i.tangent.w;
	
	output.uv = i.tex;

    output.renderDepthPass = 0;

    return output;
}; 
///////////////////////////////////////////////////////////////////////////////////////////////////

struct RAY_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION; // Output position
};

RAY_VS_OUTPUT RAY_VS(uint VertexID : SV_VERTEXID)
{
	RAY_VS_OUTPUT output;

	output.clip_pos = float4(0,0,0,1);

    return output;	
}

RAY_VS_OUTPUT CREATE_RAY_VS_OUTPUT(float4 arg){
	RAY_VS_OUTPUT output;

	output.clip_pos = arg;

	return output;
}

[maxvertexcount(3)]
void RAY_GS(point RAY_VS_OUTPUT pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<RAY_VS_OUTPUT> triStream )
{
	float4 p = float4(0,0,0,0);//g_ray_s + 1000.0*g_ray_v;
	p = mul(p, g_mWorldView);
	triStream.Append(CREATE_RAY_VS_OUTPUT(mul(p, g_mWorldViewProjection)));
	triStream.Append(CREATE_RAY_VS_OUTPUT(mul(float4(p.xyz,1), g_mProjection)));
	triStream.Append(CREATE_RAY_VS_OUTPUT(mul(p, g_mWorldViewProjection)));
	triStream.RestartStrip();
}
///////////////////////////////////////////////////////////////////////////////////////////////////

struct  SPHERE_VS_OUTPUT
{
    float4 clip_pos       : SV_POSITION; // Output position
};

SPHERE_VS_OUTPUT SPHERE_VS( in PosNormalTex2d i )
{
    SPHERE_VS_OUTPUT output;

    output.clip_pos = mul( float4( i.pos, 1.0 ), g_mWorldViewProjection );
    
    return output;
}; 
///////////////////////////////////////////////////////////////////////////////////////////////////
///Render TBN
ClipPosNormalTangentTex2d RENDER_TBN_VS( in PosNormalTangentTex2d i )
{
    ClipPosNormalTangentTex2d output;

	output.pos = float4(i.pos,1);
	output.normal = i.normal;
	output.tangent = i.tangent;

	return output;
}

void axis(in float4 color, in float4 position, in float4 direction, inout LineStream<ClipPosColor> SpriteStream){
    ClipPosColor output;

    output.color = color;
    output.clip_pos = mul(mul( position, g_mView ), g_mProjection );
    SpriteStream.Append( output );

    output.color = color;
    output.clip_pos = mul(mul(position + .5 * direction, g_mView ), g_mProjection );
    SpriteStream.Append( output );

    SpriteStream.RestartStrip();
}

[maxvertexcount(2)]
void RENDER_TBN_GS(point ClipPosNormalTangentTex2d input[1], uint primID : SV_PrimitiveID, inout LineStream<ClipPosColor> SpriteStream){
  float3 n = normalize(mul(input[0].normal.xyz,  (float3x3)g_mWorld));
  float3 t = normalize(mul(input[0].tangent.xyz, (float3x3)g_mWorld));

  float3 b = normalize(cross(n, t))*input[0].tangent.w;	

  //axis(float4(1,0,0,1), mul(input[0].pos, g_mWorld), float4(t, 0), SpriteStream);
  //axis(float4(0,1,0,1), mul(input[0].pos, g_mWorld), float4(b, 0), SpriteStream);
  axis(float4(0,0,1,1), mul(input[0].pos, g_mWorld), float4(n, 0), SpriteStream);
}