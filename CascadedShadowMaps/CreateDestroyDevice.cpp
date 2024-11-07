#include "main.h"

#include "DXUTgui.h"
#include "SDKmisc.h"
#include "SDKMesh.h"

HWND DXUTgetWindow();

GraphicResources * G;

SceneState scene_state;

CascadeState cascade_state;

BlurHandling blur_handling;

BlurParams blurParams;

std::unique_ptr<Keyboard> _keyboard;
std::unique_ptr<Mouse> _mouse;

CDXUTDialogResourceManager          g_DialogResourceManager;
CDXUTTextHelper*                    g_pTxtHelper = NULL;

#include <codecvt>
std::unique_ptr<SceneNode> loadSponza(ID3D11Device* device, ID3D11InputLayout** l, DirectX::IEffect *e);

inline float lerp(float x1, float x2, float t){
	return x1*(1.0 - t) + x2*t;
}

inline float nextFloat(float x1, float x2){
	return lerp(x1, x2, (float)std::rand() / (float)RAND_MAX);
}


static const XMFLOAT4 g_vFLTMAX = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
static const XMFLOAT4 g_vFLTMIN = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };

XMFLOAT4 g_vSceneAABBMin = g_vFLTMAX;
XMFLOAT4 g_vSceneAABBMax = g_vFLTMIN;

XMFLOAT4 vSceneAABBPointsModelSpace[8];
XMFLOAT4 vSceneAABBPointsLightSpace[8];

float iCascadePartitionsZeroToOne[8];

XMFLOAT4X4 g_matShadowProj[8];
float g_fCascadePartitionsFrustum[8];
D3D11_VIEWPORT g_CascadeRenderVP[8];

float g_cameraFarPlane;

using namespace SimpleMath;

HRESULT CALLBACK OnD3D11CreateDevice(ID3D11Device* device, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc,
	void* pUserContext)
{
	std::srand(unsigned(std::time(0)));

	HRESULT hr;

	ID3D11DeviceContext* context = DXUTGetD3D11DeviceContext();

	G = new GraphicResources();
	G->render_states = std::make_unique<CommonStates>(device);

	G->scene_constant_buffer = std::make_unique<ConstantBuffer<SceneState> >(device);
	G->cascade_constant_buffer = std::make_unique<ConstantBuffer<CascadeState> >(device);

	_keyboard = std::make_unique<Keyboard>();
	_mouse = std::make_unique<Mouse>();
	HWND hwnd = DXUTgetWindow();
	_mouse->SetWindow(hwnd);

	g_DialogResourceManager.OnD3D11CreateDevice(device, context);
	g_pTxtHelper = new CDXUTTextHelper(device, context, &g_DialogResourceManager, 15);

	//effects
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"TESTSCENE_VS", L"vs_5_0" };
		shaderDef[L"PS"] = { L"ModelPS.hlsl", L"TESTSCENE_PS", L"ps_5_0" };

		G->testscene_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"ModelVS.hlsl", L"DEPTH_TESTSCENE_VS", L"vs_5_0" };

		G->depth_testscene_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"Debug.hlsl", L"DEBUG_VS", L"vs_5_0" };
		shaderDef[L"GS"] = { L"Debug.hlsl", L"BOX_GS", L"gs_5_0" };
		shaderDef[L"PS"] = { L"Debug.hlsl", L"DEBUG_PS_BOX", L"ps_5_0" };

		G->debug_box_effect = createHlslEffect(device, shaderDef);
	}
	{
		std::map<const WCHAR*, EffectShaderFileDef> shaderDef;
		shaderDef[L"VS"] = { L"Debug.hlsl", L"DEBUG_VS", L"vs_5_0" };
		shaderDef[L"GS"] = { L"Debug.hlsl", L"FRUSTUM_GS", L"gs_5_0" };
		shaderDef[L"PS"] = { L"Debug.hlsl", L"DEBUG_PS_FRUSTUM", L"ps_5_0" };

		G->debug_frustum_effect = createHlslEffect(device, shaderDef);
	}
	//models
	{
		hr = G->MeshTestScene.Create(device, L"testscene.sdkmesh");
		auto& mesh = G->MeshTestScene;
		for (UINT i = 0; i < mesh.GetNumMeshes(); ++i)
		{
			SDKMESH_MESH* msh = mesh.GetMesh(i);
			const auto _min = msh->BoundingBoxCenter - msh->BoundingBoxExtents;
			const auto _max = msh->BoundingBoxCenter + msh->BoundingBoxExtents;

			g_vSceneAABBMin = Vector4(XMVectorMin(Vector4(Vector3((const float*)_min)), Vector4(g_vSceneAABBMin)));
			g_vSceneAABBMax = Vector4(XMVectorMax(Vector4(Vector3((const float*)_max)), Vector4(g_vSceneAABBMax)));
		}

		g_cameraFarPlane = (Vector3(g_vSceneAABBMax.x, g_vSceneAABBMax.y, g_vSceneAABBMax.z) - Vector3(g_vSceneAABBMin.x, g_vSceneAABBMin.y, g_vSceneAABBMin.z)).Length();

		//
		static const Vector4 vExtentsMap[] =
		{
			{ 1.0f, 1.0f, -1.0f, 0.0f },
			{ -1.0f, 1.0f, -1.0f, 0.0f },
			{ 1.0f, -1.0f, -1.0f, 0.0f },
			{ -1.0f, -1.0f, -1.0f, 0.0f },
			{ 1.0f, 1.0f, 1.0f, 0.0f },
			{ -1.0f, 1.0f, 1.0f, 0.0f },
			{ 1.0f, -1.0f, 1.0f, 0.0f },
			{ -1.0f, -1.0f, 1.0f, 0.0f }
		};
		//
		auto center = (Vector4(g_vSceneAABBMin) + Vector4(g_vSceneAABBMax))*0.5;
		center.w = 1.0f;
		auto half_extent = (Vector4(g_vSceneAABBMax) - Vector4(g_vSceneAABBMin))*0.5;

		for (INT index = 0; index < 8; ++index)
		{
			vSceneAABBPointsModelSpace[index] = center + half_extent * vExtentsMap[index];
		}

	}
	//Input layouts
	{
		const D3D11_INPUT_ELEMENT_DESC layout_mesh[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		const void *buffer;
		size_t size;
		G->testscene_effect->GetVertexShaderBytecode(&buffer, &size);
		hr = device->CreateInputLayout(layout_mesh, ARRAYSIZE(layout_mesh), buffer, size, G->testscene_input_layout.ReleaseAndGetAddressOf());
	}
	//light_camera1
	{
		set_scene_light_camera1_view_matrix(Vector3(-320.0f, 300.0f, -220.3f), Vector3(0.0f, 0.0f, 0.0f));
	}
	//Cascade Partitions Zero To One
	{
		iCascadePartitionsZeroToOne[0] = 5;
		iCascadePartitionsZeroToOne[1] = 15;
		iCascadePartitionsZeroToOne[2] = 60;
		iCascadePartitionsZeroToOne[3] = 100;
		iCascadePartitionsZeroToOne[4] = 100;
		iCascadePartitionsZeroToOne[5] = 100;
		iCascadePartitionsZeroToOne[6] = 100;
		iCascadePartitionsZeroToOne[7] = 100;
	}
	{
		for (INT index = 0; index < 8; ++index)
		{
			g_CascadeRenderVP[index].Height = 1024;
			g_CascadeRenderVP[index].Width = 1024;
			g_CascadeRenderVP[index].MaxDepth = 1.0f;
			g_CascadeRenderVP[index].MinDepth = 0.0f;
			g_CascadeRenderVP[index].TopLeftX = (FLOAT)(1024 * index);
			g_CascadeRenderVP[index].TopLeftY = 0;
		}
	}
	{
		///
		D3D11_TEXTURE2D_DESC dtd =
		{
			1024 * 8,//UINT Width;
			1024,//UINT Height;
			1,//UINT MipLevels;
			1,//UINT ArraySize;
			DXGI_FORMAT_R32_TYPELESS,//DXGI_FORMAT Format;
			1,//DXGI_SAMPLE_DESC SampleDesc;
			0,
			D3D11_USAGE_DEFAULT,//D3D11_USAGE Usage;
			D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE,//UINT BindFlags;
			0,//UINT CPUAccessFlags;
			0//UINT MiscFlags;    
		};

		hr = device->CreateTexture2D(&dtd, NULL, G->CascadedShadowMapTextureT.ReleaseAndGetAddressOf());
		///
		D3D11_DEPTH_STENCIL_VIEW_DESC  dsvd =
		{
			DXGI_FORMAT_D32_FLOAT,
			D3D11_DSV_DIMENSION_TEXTURE2D,
			0
		};
		hr = device->CreateDepthStencilView(G->CascadedShadowMapTextureT.Get(), &dsvd, G->CascadedShadowMapTextureV.ReleaseAndGetAddressOf());

		D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd =
		{
			DXGI_FORMAT_R32_FLOAT,
			D3D11_SRV_DIMENSION_TEXTURE2D,
			0,
			0
		};
		dsrvd.Texture2D.MipLevels = 1;

		hr = device->CreateShaderResourceView(G->CascadedShadowMapTextureT.Get(), &dsrvd, G->CascadedShadowMapTextureSRV.ReleaseAndGetAddressOf());
	}
	///Shadow Sampler
	{
		D3D11_SAMPLER_DESC SamDescShad =
		{
			D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,// D3D11_FILTER Filter;
			D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressU;
			D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressV;
			D3D11_TEXTURE_ADDRESS_BORDER, //D3D11_TEXTURE_ADDRESS_MODE AddressW;
			0,//FLOAT MipLODBias;
			0,//UINT MaxAnisotropy;
			D3D11_COMPARISON_LESS, //D3D11_COMPARISON_FUNC ComparisonFunc;
			0.0, 0.0, 0.0, 0.0,//FLOAT BorderColor[ 4 ];
			0,//FLOAT MinLOD;
			0//FLOAT MaxLOD;   
		};
		device->CreateSamplerState(&SamDescShad, G->PCFSampler.ReleaseAndGetAddressOf());
	}
	///RenderState
	{
		D3D11_RASTERIZER_DESC drd =
		{
			D3D11_FILL_SOLID,//D3D11_FILL_MODE FillMode;
			D3D11_CULL_NONE,//D3D11_CULL_MODE CullMode;
			FALSE,//BOOL FrontCounterClockwise;
			0,//INT DepthBias;
			0.0,//FLOAT DepthBiasClamp;
			0.0,//FLOAT SlopeScaledDepthBias;
			TRUE,//BOOL DepthClipEnable;
			FALSE,//BOOL ScissorEnable;
			TRUE,//BOOL MultisampleEnable;
			FALSE//BOOL AntialiasedLineEnable;   
		};
		device->CreateRasterizerState(&drd, G->rsScene.ReleaseAndGetAddressOf());

		drd.SlopeScaledDepthBias = 1.0;
		device->CreateRasterizerState(&drd, G->rsShadow.ReleaseAndGetAddressOf());
	}

	std::unique_ptr<framework::UniformBuffer> CascadeBuffer;
	std::unique_ptr<framework::UniformBuffer> FrustumBuffer;
	G->CascadeBuffer = std::make_unique<framework::UniformBuffer>();
	G->FrustumBuffer = std::make_unique<framework::UniformBuffer>();

	G->CascadeBuffer->initDefaultStructured<FCascade>(device, 4);
	G->FrustumBuffer->initDefaultStructured<FFrustum>(device, 3);

	FCascade SceneBox;
	SceneBox.Min = Vector4(g_vSceneAABBMin.x, g_vSceneAABBMin.y, g_vSceneAABBMin.z, 1);
	SceneBox.Max = Vector4(g_vSceneAABBMax.x, g_vSceneAABBMax.y, g_vSceneAABBMax.z, 1);
	SceneBox.Orientation = Matrix::Identity;
	G->CascadeBuffer->setElement(0, SceneBox);

	{
		D3D11_BLEND_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.RenderTarget[0].BlendEnable = true;

		desc.RenderTarget[0].SrcBlend = desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		desc.RenderTarget[0].DestBlend = desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		desc.RenderTarget[0].BlendOp = desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

		desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		HRESULT hr = device->CreateBlendState(&desc, G->alfaBlend.ReleaseAndGetAddressOf());
	}

	return S_OK;
}

//--------------------------------------------------------------------------------------
// Release D3D11 resources created in OnD3D11CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D11DestroyDevice(void* pUserContext)
{
	delete g_pTxtHelper;

	g_DialogResourceManager.OnD3D11DestroyDevice();

	_mouse = 0;

	_keyboard = 0;

	G->MeshTestScene.Destroy();

	delete G;
}
