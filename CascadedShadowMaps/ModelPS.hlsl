cbuffer cbCascade : register( b1 )
{
	float4    CascadeScale[8];
	float4    CascadeOffset[8];
	float4    CascadeFrustumsEyeSpaceDepths[8];	
};

SamplerState linearSampler : register( s0 );

SamplerComparisonState PCFSampler : register( s1 );

Texture2D  texDiffuse    : register( t0 );

Texture2D  texNormal     : register( t1 );

TextureCube  texCube     : register( t2 );

Texture2D  texShadow     : register( t3 );

static const float4 vCascadeColorsMultiplier[8] = 
{
    float4 ( 1.5f, 0.0f, 0.0f, 1.0f ),
    float4 ( 0.0f, 1.5f, 0.0f, 1.0f ),
    float4 ( 0.0f, 0.0f, 5.5f, 1.0f ),
    float4 ( 1.5f, 0.0f, 5.5f, 1.0f ),
    float4 ( 1.5f, 1.5f, 0.0f, 1.0f ),
    float4 ( 1.0f, 1.0f, 1.0f, 1.0f ),
    float4 ( 0.0f, 1.0f, 5.5f, 1.0f ),
    float4 ( 0.5f, 3.5f, 0.75f, 1.0f )
};

struct Targets
{
    float4 color: SV_Target0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CalculatePCFPercentLit ( in float3 vShadowTexCoord, out float fPercentLit) 
{
/////////////////////////////// 
    int CascadeCapacity = 8;   
    int SubShadowMapSize = 1024;
    int iPCFBlurForLoopStart = -1;
    int iPCFBlurForLoopEnd = 2;
    float fShadowBiasFromGUI = 0.002;

/////////////////////////////// 
    float fTexelSize = 1.0/(float)SubShadowMapSize;
    float fNativeTexelSizeInX = fTexelSize / (float)CascadeCapacity;
    float fBlurRowSize = iPCFBlurForLoopEnd - iPCFBlurForLoopStart;
    fBlurRowSize *= fBlurRowSize;

///////////////////////////////    
    fPercentLit = 0.0f;
    for( int x = iPCFBlurForLoopStart; x < iPCFBlurForLoopEnd; ++x ) 
    {
        for( int y = iPCFBlurForLoopStart; y < iPCFBlurForLoopEnd; ++y ) 
        {
            float depthcompare = vShadowTexCoord.z;
            depthcompare -= fShadowBiasFromGUI;
            fPercentLit += texShadow.SampleCmpLevelZero( PCFSampler, 
                float2( 
                    vShadowTexCoord.x + ( ( (float) x ) * fNativeTexelSizeInX ) , 
                    vShadowTexCoord.y + ( ( (float) y ) * fTexelSize ) 
                    ), 
                depthcompare );
        }
    }
    fPercentLit /= (float)fBlurRowSize;  
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Targets TESTSCENE_PS(
    float2 tex            : TEXCOORD0,
	float3 normal         : TEXCOORD1,
	float3 light_dir      : TEXCOORD2,
	float3 position       : TEXCOORD3,
	float  vDepth		  : TEXCOORD4,
	float3 vTexShadow	  : TEXCOORD5,
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

///////////////////////////////
   int CascadeCapacity = 8;
   int CascadeLength = 3;
   int SubShadowMapSize = 1024;

///////////////////////////////
   float SubShadowMapNormalSize = 1.0f/(float)CascadeCapacity;
   float m_fMinBorderPadding  = 1.0/(float)SubShadowMapSize;
   float m_fMaxBorderPadding  = ((float)SubShadowMapSize - 1.0)/(float)SubShadowMapSize;

///////////////////////////////
   float3 Nn = normalize(normal);
   float3 Ln = normalize(light_dir);

///////////////////////////////
   float4 vVisualizeCascadeColor = float4(1.0f,1.0f,1.0f,1.0f);
   int iCurrentCascadeIndex = 0;
   int iCascadeFound = 0;
   float3 vShadowMapTextureCoord = 0.0f;
   float fPercentLit = 1.0;

///////////////////////////////
   for( int iCascadeIndex = 0; iCascadeIndex < CascadeLength && iCascadeFound == 0; ++iCascadeIndex ) 
   {
       vShadowMapTextureCoord = vTexShadow * CascadeScale[iCascadeIndex].xyz;
       vShadowMapTextureCoord += CascadeOffset[iCascadeIndex].xyz;

       if ( min( vShadowMapTextureCoord.x, vShadowMapTextureCoord.y ) > m_fMinBorderPadding
            && max( vShadowMapTextureCoord.x, vShadowMapTextureCoord.y ) < m_fMaxBorderPadding )
       {  
          iCurrentCascadeIndex = iCascadeIndex; 
          iCascadeFound = 1;  
       }
   }

///////////////////////////////
   vShadowMapTextureCoord.x *= SubShadowMapNormalSize;
   vShadowMapTextureCoord.x += ( SubShadowMapNormalSize * (float)iCurrentCascadeIndex );   

///////////////////////////////
   vVisualizeCascadeColor = vCascadeColorsMultiplier[iCurrentCascadeIndex];

///////////////////////////////
   CalculatePCFPercentLit ( vShadowMapTextureCoord, fPercentLit );

///////////////////////////////
   //float4 diffuseColor = texDiffuse.Sample( linearSampler, tex );
   //vVisualizeCascadeColor = float4(1.0f,1.0f,1.0f,1.0f);

   float3 vLightDir1 = float3( -1.0f, 1.0f, -1.0f ); 
   float3 vLightDir2 = float3( 1.0f, 1.0f, -1.0f ); 
   float3 vLightDir3 = float3( 0.0f, -1.0f, 0.0f );
   float3 vLightDir4 = float3( 1.0f, 1.0f, 1.0f );     

   float fLighting = 
                      saturate( dot( vLightDir1 , Nn ) )*0.05f +
                      saturate( dot( vLightDir2 , Nn ) )*0.05f +
                      saturate( dot( vLightDir3 , Nn ) )*0.05f +
                      saturate( dot( vLightDir4 , Nn ) )*0.05f ;
    
   float4 vShadowLighting = fLighting * 0.5f;
   fLighting += saturate( dot( Ln , Nn ) );
   fLighting = lerp( vShadowLighting, fLighting, fPercentLit );

///////////////////////////////
   output.color = fLighting*vVisualizeCascadeColor;//*diffuseColor;

   return output;
};


///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
Targets GROUND_PS(
    float2 tex            : TEXCOORD1,
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = float4(0.5,0.5,0.5,1);

   return output;
};
///////////////////////////////////////////////////////////////////////////////////////////////
Targets SKY_PS(
    float3 tex            : TEXCOORD1,
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = texCube.Sample(linearSampler, tex);

   return output;
};
///////////////////////////////////////////////////////////////////////////////////////////////

Targets BOX_PS(
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = float4(0,0,1,0.5);

   return output;
};

///////////////////////////////////////////////////////////////////////////////////////////////
void lighting_calculations(
        float3 AmbiColor,
        float3 LightColor, 
        float3 LightPos, 
        float3 VertexPos, 
        float3 VertexNormal,

		out float3 DiffuseContrib
        )
{
    float3 Ln = normalize(LightPos - VertexPos);    
    float3 Nn = VertexNormal;
    float3 Vn = normalize( - VertexPos);
    float3 Hn = normalize(Vn + Ln);
    float4 litV = lit(dot(Ln,Nn),dot(Hn,Nn),0);
    
    DiffuseContrib = litV.y * LightColor + AmbiColor;
}
///////////////////////////////////////////////////////////////////////////////////////////////
float3x3 GetTBN(float3 t, float3 b, float3 n)
{
    float3 _t = normalize(t);
    float3 _b = normalize(b);
    float3 _n = normalize(n);    

    float3x3 tbn = { 
        _t.x, _t.y, _t.z,
	    _b.x, _b.y, _b.z,
	    _n.x, _n.y, _n.z,
    }; 

    return tbn;
}
float3 UnpackNormal(float3 n)
{
    return n * 2.0 - float3(1.0, 1.0, 1.0);
}
Targets FIGURE_PS(
    float4 clip_pos       : SV_POSITION,
	float3 t       : TEXCOORD0,
	float3 b       : TEXCOORD1,
	float3 n       : TEXCOORD2,
	float2 uv      : TEXCOORD3,

	float3 lightPos: TEXCOORD4,
	float3 fragPos:  TEXCOORD5,
   	float3 fragPosRelToLight : TEXCOORD6,

    float4 renderDepthPass   : TEXCOORD7
):SV_TARGET
{ 
   Targets output;

   float sampleDepth = .0;
   float projectDepth = .0;
   if(renderDepthPass.x == 0.)
   {
       sampleDepth = texCube.Sample(linearSampler, fragPosRelToLight).r;
       float maxFragPosRelToLightCoord = max( abs( fragPosRelToLight.x ), max( abs( fragPosRelToLight.y ), abs( fragPosRelToLight.z ) ) );
       // the math is: -1.0f / maxCoord * (Zn * Zf / (Zf - Zn) + Zf / (Zf - Zn) (should match the shadow projection matrix)
       projectDepth =  -(1.0f / maxFragPosRelToLightCoord) * (1000.0 * 0.1 / (1000.0 - 0.1)) + (1000.0 / (1000.0 - 0.1)) - 0.00001;
   }

   float3 DiffuseContrib;
   if(projectDepth > sampleDepth)
   {
       DiffuseContrib = float3(0.,0.,0.);
   }
   else
   {   
       float3 fragNormal = UnpackNormal(texNormal.Sample(linearSampler, uv).xyz);
       fragNormal = normalize(fragNormal);
       fragNormal = mul(fragNormal, GetTBN(t,b,n));
       fragNormal = normalize(fragNormal);

       float3 AmbiColor = float3(0.,0.,0.);
       float3 LightColor = float3(1.,1.,1.);

       lighting_calculations(AmbiColor, LightColor, lightPos, fragPos, fragNormal, DiffuseContrib);
   }

   float3 MaterialColor = float3(1.0,1.0,1.0);
   output.color = float4(DiffuseContrib*MaterialColor,1);

   return output;
};
///////////////////////////////////////////////////////////////////////////////////////////////
Targets BENCH_PS(
    float4 clip_pos       : SV_POSITION,
	float3 t       : TEXCOORD0,
	float3 b       : TEXCOORD1,
	float3 n       : TEXCOORD2,
	float2 uv      : TEXCOORD3,

	float3 lightPos: TEXCOORD4,
	float3 fragPos: TEXCOORD5,
   	float3 fragPosRelToLight : TEXCOORD6,

    float4 renderDepthPass   : TEXCOORD7
):SV_TARGET
{
   Targets output;

   float sampleDepth = .0;
   float projectDepth = .0;
   if(renderDepthPass.x == 0.)
   {
       sampleDepth = texCube.Sample(linearSampler, fragPosRelToLight).r;
       float maxFragPosRelToLightCoord = max( abs( fragPosRelToLight.x ), max( abs( fragPosRelToLight.y ), abs( fragPosRelToLight.z ) ) );
       // the math is: -1.0f / maxCoord * (Zn * Zf / (Zf - Zn) + Zf / (Zf - Zn) (should match the shadow projection matrix)
       projectDepth =  -(1.0f / maxFragPosRelToLightCoord) * (1000.0 * 0.1 / (1000.0 - 0.1)) + (1000.0 / (1000.0 - 0.1)) - 0.00001;
   }

   float3 DiffuseContrib;
   if(projectDepth > sampleDepth)
   {
       DiffuseContrib = float3(0.,0.,0.);
   }
   else
   {
       float3 fragNormal = normalize(n);

       float3 AmbiColor = float3(0.,0.,0.);
       float3 LightColor = float3(1.,1.,1.);

       lighting_calculations(AmbiColor, LightColor, lightPos, fragPos, fragNormal, DiffuseContrib);
   }

   float3 MaterialColor = float3(1.0,1.0,1.0);
   output.color = float4(DiffuseContrib*MaterialColor,1);

   return output;
}
///////////////////////////////////////////////////////////////////////////////////////////////

Targets RAY_PS(
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = float4(1,0,0,1);

   return output;
};


Targets RENDER_TBN_PS(
    float4 color          : TEXCOORD1,
    float4 clip_pos       : SV_POSITION
):SV_TARGET
{ 
   Targets output;

   output.color = color;

   return output;
};