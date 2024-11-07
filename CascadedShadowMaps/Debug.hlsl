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

struct FCascade
{
	float4 Min;
	float4 Max;
	Matrix InvertOrientation;
	matrix Orientation;
};

struct FFrustum
{
	float4 Quad[4];
	Matrix InvertOrientation;
	matrix Orientation;
};

StructuredBuffer<FCascade> CascadeData : register(t1);
StructuredBuffer<FFrustum> FrustumData : register(t2);

static const float4 cubeVerts[8] = 
{
	float4(-0.5, -0.5, -0.5, 1),// LB  0
	float4(-0.5, 0.5, -0.5, 1), // LT  1
	float4(0.5, -0.5, -0.5, 1), // RB  2
	float4(0.5, 0.5, -0.5, 1),  // RT  3
	float4(-0.5, -0.5, 0.5, 1), // LB  4
	float4(-0.5, 0.5, 0.5, 1),  // LT  5
	float4(0.5, -0.5, 0.5, 1),  // RB  6
	float4(0.5, 0.5, 0.5, 1)    // RT  7
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

struct DEBUG_VS_OUTPUT
{
	float4 color: TEXCOORD0;
    float4 clip_pos       : SV_POSITION; // Output position
	float4 A: TEXCOORD1;
	float4 B: TEXCOORD2;
	float4 C: TEXCOORD3;
	float4 P: TEXCOORD4;	
};

DEBUG_VS_OUTPUT DEBUG_VS(uint VertexID : SV_VERTEXID)
{
	DEBUG_VS_OUTPUT output;

	output.clip_pos = float4(0,0,0,1);

    return output;	
}

[maxvertexcount(36)]
void BOX_GS(point DEBUG_VS_OUTPUT pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<DEBUG_VS_OUTPUT> triStream )
{
    float4x4 Orientation = CascadeData[vDebugIndex.w].Orientation; //{ 1,0,0,0,   0,1,0,0,   0,0,1,0,   0,0,0,1 }; //CascadeData[vDebugIndex.w].Orientation;
    float4 Max = CascadeData[vDebugIndex.w].Max; //float4(110.967712f,18.7685623f,59.3948822f, 1); //CascadeData[vDebugIndex.w].Max;
    float4 Min = CascadeData[vDebugIndex.w].Min; //float4(-814.799561f, -13.7459068f, -58.8112488f, 1); //CascadeData[vDebugIndex.w].Min;
 	float4 Color = vDebugColor;

    float4 Center = 0.5f*(Max + Min);
    float4 Extent = (Max - Min);
    float4x4 Translate = 
    {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        Center.x,Center.y,Center.z,1
    };

	DEBUG_VS_OUTPUT v[8];
	[unroll]
	for (int j = 0; j < 8; j++)
	{
        float4 Vertex = cubeVerts[j];

		float4 model_pos = float4(Vertex.x*Extent.x,Vertex.y*Extent.y,Vertex.z*Extent.z,1);
        float4 clip_pos = mul( model_pos, Translate);
        clip_pos = mul(clip_pos, Orientation);
        clip_pos = mul(clip_pos, g_mWorldViewProjection);

		v[j].P = model_pos;
		v[j].clip_pos = clip_pos;
		v[j].color = Color;
	}
	
	for (int i = 0; i < 6; i++)
	{
		DEBUG_VS_OUTPUT A = v[cubeIndices[i * 4 + 1]];
		DEBUG_VS_OUTPUT B = v[cubeIndices[i * 4 + 2]];
		DEBUG_VS_OUTPUT C = v[cubeIndices[i * 4 + 0]];
		A.A = B.A = C.A = A.P;
		A.B = B.B = C.B = B.P;
		A.C = B.C = C.C = C.P;
		triStream.Append(A);
		triStream.Append(B);
		triStream.Append(C);
		triStream.RestartStrip();

		A = v[cubeIndices[i * 4 + 3]];
		B = v[cubeIndices[i * 4 + 2]];
		C = v[cubeIndices[i * 4 + 1]];
		A.A = B.A = C.A = A.P;
		A.B = B.B = C.B = B.P;
		A.C = B.C = C.C = C.P;
		triStream.Append(A);
		triStream.Append(B);
		triStream.Append(C);
		triStream.RestartStrip();
	}
}

DEBUG_VS_OUTPUT Tr(int Index)
{
    matrix Orientation = FrustumData[vDebugIndex.w].Orientation;
    DEBUG_VS_OUTPUT v = {vDebugColor, mul(FrustumData[vDebugIndex.w].Quad[Index], Orientation), float4(0,0,0,0), float4(0,0,0,0), float4(0,0,0,0), FrustumData[vDebugIndex.w].Quad[Index]};
    v.clip_pos = mul(v.clip_pos, g_mWorldViewProjection);
    return v;
}

[maxvertexcount(36)]
void FRUSTUM_GS(point DEBUG_VS_OUTPUT pnt[1], uint primID : SV_PrimitiveID,  inout TriangleStream<DEBUG_VS_OUTPUT> triStream )
{
    matrix Orientation = FrustumData[vDebugIndex.w].Orientation;
    DEBUG_VS_OUTPUT v1 = {vDebugColor, mul(float4(0,0,0,1), Orientation), float4(0,0,0,0), float4(0,0,0,0), float4(0,0,0,0), float4(0,0,0,1)};
    v1.clip_pos = mul(v1.clip_pos, g_mWorldViewProjection);

	for (int i = 0; i < 4; i++)
    {
        DEBUG_VS_OUTPUT v2 = Tr(i);
        DEBUG_VS_OUTPUT v3 = Tr((i+1)%4);

		v1.A = v2.A = v3.A = v1.P;
		v1.B = v2.B = v3.B = v2.P;
		v1.C = v2.C = v3.C = v3.P;
        triStream.Append(v1);
		triStream.Append(v2);
		triStream.Append(v3);
		triStream.RestartStrip();
    }

	{    
		DEBUG_VS_OUTPUT va = Tr(2);
		DEBUG_VS_OUTPUT vb = Tr(1);
		DEBUG_VS_OUTPUT vc = Tr(0);
		va.A = vb.A = vc.A = va.P;
		va.B = vb.B = vc.B = vb.P;
		va.C = vb.C = vc.C = vc.P;
		triStream.Append(va);
		triStream.Append(vb);
		triStream.Append(vc);
		triStream.RestartStrip();
	}

	{    
		DEBUG_VS_OUTPUT va = Tr(2);
		DEBUG_VS_OUTPUT vb = Tr(0);
		DEBUG_VS_OUTPUT vc = Tr(3);
		va.A = vb.A = vc.A = va.P;
		va.B = vb.B = vc.B = vb.P;
		va.C = vb.C = vc.C = vc.P;
		triStream.Append(va);
		triStream.Append(vb);
		triStream.Append(vc);
		triStream.RestartStrip();
	}
}

struct Targets
{
    float4 color: SV_Target0;
};

Targets DEBUG_PS_BOX(
	float4 color: TEXCOORD0,
    float4 clip_pos       : SV_POSITION,
	float4 A: TEXCOORD1,
	float4 B: TEXCOORD2,
	float4 C: TEXCOORD3,
	float4 P: TEXCOORD4
):SV_TARGET
{
   Targets output;
    output.color = color;

   float3 BA = B.xyz - A.xyz;

   float3 CA = C.xyz - A.xyz;

   float3 CB = C.xyz - B.xyz;

   float3 PA = P.xyz - A.xyz;

   float3 PB = P.xyz - B.xyz;

   BA = normalize(BA);

   CA = normalize(CA);

   CB = normalize(CB);

   int i = 0;
   if(abs(dot(BA,CA))<0.0001f)
   {
      i = 3;
   }
   if(abs(dot(BA,CB))<0.0001f)
   {
      i = 2;
   }
   if(abs(dot(CB,CA))<0.0001f)
   {
      i = 1;
   }

   if(abs(length(PA - dot(PA, BA)*BA))<0.08 && i!=1) //dot(PA, BA)>.0 && 
     output.color = float4(1,1,1,1);   
   else if(abs(length(PA - dot(PA, CA)*CA))<0.08 && i!=2) //dot(PA, CA)>.0 &&
     output.color = float4(1,1,1,1);     
   else if(abs(length(PB - dot(PB, CB)*CB))<0.08 && i!=3) //dot(PB, CB)>.0 && 
     output.color = float4(1,1,1,1);  

	//////
	{
		float4x4 InvertOrientation1 = CascadeData[1].InvertOrientation; //{ 1,0,0,0,   0,1,0,0,   0,0,1,0,   0,0,0,1 }; //CascadeData[vDebugIndex.w].Orientation;
		float4 Max = CascadeData[1].Max; //float4(110.967712f,18.7685623f,59.3948822f, 1); //CascadeData[vDebugIndex.w].Max;
		float4 Min = CascadeData[1].Min; //float4(-814.799561f, -13.7459068f, -58.8112488f, 1); //CascadeData[vDebugIndex.w].Min;
    	float4 Center = 0.5f*(Max + Min);

		float4 Plane = float4(normalize(cross(CA,CB)), -dot(normalize(cross(CA,CB)), A.xyz));

		matrix Orientation2 = FrustumData[0].Orientation;
		for(int Index = 0; Index<4; Index++)
		{
			float4 D = FrustumData[0].Quad[Index];
			float4 R = mul(mul(D, Orientation2), InvertOrientation1) - Center; 

			if(abs(dot(Plane, float4(R.xyz,1.f)))<0.0001f)
			{
				if(length(P-R) < 5)
				{
					output.color = float4(1,1,1,1);
				} 
			}		
		}
		float4 D = float4(0,0,0,1);
		float4 R = mul(mul(D, Orientation2), InvertOrientation1) - Center; 

		if(abs(dot(Plane, float4(R.xyz,1.f)))<0.0001f)
		{
			if(length(P-R) < 5)
			{
				output.color = float4(1,1,1,1);
			} 
		}		
	}
	//////	
    return output;
}

Targets DEBUG_PS_FRUSTUM(
	float4 color: TEXCOORD0,
    float4 clip_pos       : SV_POSITION,	
	float4 A: TEXCOORD1,
	float4 B: TEXCOORD2,
	float4 C: TEXCOORD3,
	float4 P: TEXCOORD4
):SV_TARGET
{
   Targets output;
    output.color = color;

   float3 BA = B.xyz - A.xyz;

   float3 CA = C.xyz - A.xyz;

   float3 CB = C.xyz - B.xyz;

   float3 PA = P.xyz - A.xyz;

   float3 PB = P.xyz - B.xyz;

   BA = normalize(BA);

   CA = normalize(CA);

   CB = normalize(CB);

   int i = 0;
   if(abs(dot(BA,CA))<0.0001f)
   {
      i = 3;
   }
   if(abs(dot(BA,CB))<0.0001f)
   {
      i = 2;
   }
   if(abs(dot(CB,CA))<0.0001f)
   {
      i = 1;
   }

   if(abs(length(PA - dot(PA, BA)*BA))<0.08 && i!=1) //dot(PA, BA)>.0 && 
     output.color = float4(0,0,0,1);   
   else if(abs(length(PA - dot(PA, CA)*CA))<0.08 && i!=2) //dot(PA, CA)>.0 &&
     output.color = float4(0,0,0,1);     
   else if(abs(length(PB - dot(PB, CB)*CB))<0.08 && i!=3) //dot(PB, CB)>.0 && 
     output.color = float4(0,0,0,1);  

    return output;
}
