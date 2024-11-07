#include "GeometricPrimitive.h"
#include "Effects.h"
#include "DirectXHelpers.h"
#include "Model.h"
#include "CommonStates.h"
#include "Keyboard.h"
#include "Mouse.h"
#include "SimpleMath.h"

#include "DirectXMath.h"

#include "DXUT.h"
#include "SDKMesh.h"
#include <wrl.h>

#include <map>
#include <algorithm>
#include <array>
#include <memory>
#include <assert.h>
#include <malloc.h>
#include <Exception>

#include "ConstantBuffer.h"

#include "AppBuffers.h"
#include "UniformBuffer.h"

using namespace DirectX;

struct EffectShaderFileDef{
	WCHAR * name;
	WCHAR * entry_point;
	WCHAR * shader_ver;
};
class IPostProcess
{
public:
	virtual ~IPostProcess() { }

	virtual void __cdecl Process(_In_ ID3D11DeviceContext* deviceContext, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr) = 0;
};

inline ID3D11RenderTargetView** renderTargetViewToArray(ID3D11RenderTargetView* rtv1, ID3D11RenderTargetView* rtv2 = 0, ID3D11RenderTargetView* rtv3 = 0){
	static ID3D11RenderTargetView* rtvs[10];
	rtvs[0] = rtv1;
	rtvs[1] = rtv2;
	rtvs[2] = rtv3;
	return rtvs;
};
inline ID3D11ShaderResourceView** shaderResourceViewToArray(ID3D11ShaderResourceView* rtv1, ID3D11ShaderResourceView* rtv2 = 0, ID3D11ShaderResourceView* rtv3 = 0, ID3D11ShaderResourceView* rtv4 = 0, ID3D11ShaderResourceView* rtv5 = 0){
	static ID3D11ShaderResourceView* srvs[10];
	srvs[0] = rtv1;
	srvs[1] = rtv2;
	srvs[2] = rtv3;
	srvs[3] = rtv4;
	srvs[4] = rtv5;
	return srvs;
};
inline ID3D11Buffer** constantBuffersToArray(ID3D11Buffer* c1, ID3D11Buffer* c2){
	static ID3D11Buffer* cbs[10];
	cbs[0] = c1;
	cbs[1] = c2;
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<SceneState> &cb){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb.GetBuffer();
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<CascadeState> &cb){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb.GetBuffer();
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<BlurHandling> &cb){
	static ID3D11Buffer* cbs[10];
	cbs[0] = cb.GetBuffer();
	return cbs;
};
inline ID3D11Buffer** constantBuffersToArray(DirectX::ConstantBuffer<SceneState> &c1, DirectX::ConstantBuffer<BlurHandling> &c2){
	static ID3D11Buffer* cbs[10];
	cbs[0] = c1.GetBuffer();
	cbs[1] = c2.GetBuffer();
	return cbs;
};
inline ID3D11SamplerState** samplerStateToArray(ID3D11SamplerState* ss1, ID3D11SamplerState* ss2 = 0){
	static ID3D11SamplerState* sss[10];
	sss[0] = ss1;
	sss[1] = ss2;
	return sss;
};

namespace Camera{
	void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext);
}
std::unique_ptr<DirectX::IEffect> createHlslEffect(ID3D11Device* device, std::map<const WCHAR*, EffectShaderFileDef>& fileDef);

bool LoadModel(char* filename, std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices);
std::unique_ptr<DirectX::ModelMeshPart> CreateModelMeshPart(ID3D11Device* device, std::function<void(std::vector<VertexPositionNormalTexture> & _vertices, std::vector<uint16_t> & _indices)> createGeometry);
void CreateSinglePointBuffer(ID3D11Buffer ** vertexBuffer, ID3D11Device* device, DirectX::IEffect * effect, ID3D11InputLayout ** layout);
std::unique_ptr<DirectX::ModelMeshPart> CreateQuadModelMeshPart(ID3D11Device* device);

void set_scene_world_matrix();
void set_scene_object1_matrix(const SimpleMath::Matrix& m);
void set_scene_cascade(XMFLOAT4X4 * cascade, float * depth, int len);
void scene_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState = nullptr);

void post_proccess(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState);

void ground_set_world_matrix();
void ground_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr);

void wall_set_world_matrix();
void wall_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState = nullptr);

void set_scene_light_camera1_view_matrix(const SimpleMath::Vector3& eye, const SimpleMath::Vector3& focus);

struct SceneNode{
	DirectX::XMFLOAT4X4 transformation;
	std::vector<DirectX::ModelMeshPart*> mesh;
	std::vector<SceneNode*> children;
	std::vector<Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> > texture;
	~SceneNode();
	void draw(_In_ ID3D11DeviceContext* deviceContext, _In_ IEffect* ieffect, _In_ ID3D11InputLayout* iinputLayout,
		_In_opt_ std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState);
};

struct BlurParams
{
	int Radius;
	int WeightLength;
	float Weights[64 * 2 + 1];

	BlurParams(int radius = 0)
	{
		WeightLength = (Radius = radius) * 2 + 1;
	}
};
BlurParams GaussianBlur(int radius);

struct FCascade
{
	SimpleMath::Vector4 Min;
	SimpleMath::Vector4 Max;
	SimpleMath::Matrix InvertOrientation;
	SimpleMath::Matrix Orientation;
};

struct FFrustum
{
	SimpleMath::Vector4 Quad[4];
	SimpleMath::Matrix InvertOrientation;
	SimpleMath::Matrix Orientation;
};

class GraphicResources {
public:
	std::unique_ptr<CommonStates> render_states;

	std::unique_ptr<DirectX::IEffect> debug_frustum_effect;

	std::unique_ptr<DirectX::IEffect> debug_box_effect;

	std::unique_ptr<DirectX::IEffect> quad_effect;

	std::unique_ptr<DirectX::IEffect> testscene_effect;
	
	std::unique_ptr<DirectX::IEffect> depth_testscene_effect;

	std::unique_ptr<DirectX::IEffect> model_effect;

	std::unique_ptr<DirectX::ConstantBuffer<SceneState> > scene_constant_buffer;

	std::unique_ptr<DirectX::ConstantBuffer<CascadeState> > cascade_constant_buffer;

	std::unique_ptr<DirectX::ModelMeshPart> ground_model;

	std::unique_ptr<DirectX::ModelMeshPart> wall_model;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> testscene_input_layout;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> ground_texture;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> wall_texture;

	std::unique_ptr<DirectX::ModelMeshPart> quad_mesh;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> quad_mesh_layout;

	//Microsoft::WRL::ComPtr<ID3D11Buffer> single_point_buffer;

	//Microsoft::WRL::ComPtr<ID3D11InputLayout> single_point_layout;

	CDXUTSDKMesh MeshTestScene;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CascadedShadowMapTextureSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> CascadedShadowMapTextureV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> CascadedShadowMapTextureT;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> PCFSampler;

	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rsScene;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> rsShadow;

	std::unique_ptr<framework::UniformBuffer> CascadeBuffer;
	std::unique_ptr<framework::UniformBuffer> FrustumBuffer;

	Microsoft::WRL::ComPtr<ID3D11BlendState> alfaBlend;
};

class SwapChainGraphicResources {
public:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depthStencilSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilT;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> colorSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> colorV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> colorT;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> normalSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> normalV;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> normalT;
};