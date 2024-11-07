#include "main.h"
#include "SDKMesh.h"

extern GraphicResources * G;

extern SceneState scene_state;

extern CascadeState cascade_state;

using namespace SimpleMath;

void loadMatrix_VP(Matrix & v, Matrix & p){
	v = Matrix(scene_state.mView).Transpose();
	p = Matrix(scene_state.mProjection).Transpose();
}
void loadMatrix_WP(Matrix & w, Matrix & p){
	w = Matrix(scene_state.mWorld).Transpose();
	p = Matrix(scene_state.mProjection).Transpose();
}
void storeMatrix(Matrix & w, Matrix & wv, Matrix & wvp){
	scene_state.mWorld = w.Transpose();
	scene_state.mWorldView = wv.Transpose();
	scene_state.mWorldViewProjection = wvp.Transpose();
}

void DrawQuad(ID3D11DeviceContext* pd3dImmediateContext, _In_ IEffect* effect,
	_In_opt_ std::function<void __cdecl()> setCustomState){
	effect->Apply(pd3dImmediateContext);
	setCustomState();

	pd3dImmediateContext->IASetInputLayout(nullptr);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);// D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pd3dImmediateContext->Draw(1, 0);
}

void renderDXUTMesh(ID3D11DeviceContext* context, CDXUTSDKMesh* DXUTMesh, IEffect* effect, ID3D11InputLayout *pInputLayout, _In_opt_ std::function<void __cdecl()> setCustomState)
{
	context->IASetInputLayout(pInputLayout);
	effect->Apply(context);
	setCustomState();
	DXUTMesh->Render(context, 0);
}

void set_scene_world_matrix(){
	Matrix w, v, p;
	loadMatrix_VP(v, p);

	w = Matrix::Identity;// .Translation(Vector3(0.0f, 6.0f, 8.0f));
	Matrix  wv = v;
	Matrix wvp = wv * p;

	scene_state.mObject2WorldView = scene_state.light_camera1_view;

	storeMatrix(w, wv, wvp);
}

void set_scene_light_camera1_view_matrix(const SimpleMath::Vector3& eye, const SimpleMath::Vector3& focus){
	Matrix v;

	v = XMMatrixLookAtLH(eye, focus, SimpleMath::Vector3(0.0f, 1.0f, 0.0f));

	scene_state.light_camera1_view = v.Transpose();
	scene_state.light_camera1_inv_view = v.Invert().Transpose();
}

void set_scene_object1_matrix(const SimpleMath::Matrix& m){
	scene_state.mObject1WorldViewProjection = (Matrix(scene_state.light_camera1_view).Transpose() * m).Transpose();
}

void set_scene_cascade(XMFLOAT4X4 * cascade, float * depth, int len){
	Matrix ToTexSpace = Matrix::CreateScale(0.5f, -0.5f, 1.0f) * Matrix::CreateTranslation(.5, .5, 0);
	for (INT index = 0; index < len; ++index){
		auto M = Matrix(cascade[index]) * ToTexSpace;
		cascade_state.CascadeScale[index].x = M._11;
		cascade_state.CascadeScale[index].y = M._22;
		cascade_state.CascadeScale[index].z = M._33;
		cascade_state.CascadeScale[index].w = 1;

		cascade_state.CascadeOffset[index].x = M._41;
		cascade_state.CascadeOffset[index].y = M._42;
		cascade_state.CascadeOffset[index].z = M._43;
		cascade_state.CascadeOffset[index].w = 0;

		cascade_state.CascadeFrustumsEyeSpaceDepths[index].x = depth[index];
	}
}

void scene_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void(ID3D11ShaderResourceView * texture, DirectX::XMFLOAT4X4 transformation)> setCustomState){
	//G->scene->draw(pd3dImmediateContext, effect, inputLayout, setCustomState);
}

void post_proccess(ID3D11DeviceContext* context, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	/*
	context->IASetInputLayout(inputLayout);

	effect->Apply(context);

	auto vertexBuffer = G->single_point_buffer.Get();
	UINT vertexStride = sizeof(XMFLOAT3);
	UINT vertexOffset = 0;

	context->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexStride, &vertexOffset);

    setCustomState();

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	context->Draw(1, 0);
	*/
	G->quad_mesh->Draw(context, effect, inputLayout, [=]
	{
		setCustomState();
	});
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ground_set_world_matrix(){
	Matrix w, v, p;
	loadMatrix_VP(v, p);

	w.Translation(Vector3(0.0f, 1.0f, 0.0f));
	Matrix  wv = w * v;
	Matrix wvp = wv * p;

	storeMatrix(w, wv, wvp);
}
void ground_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	G->ground_model->Draw(pd3dImmediateContext, effect, inputLayout, [=]
	{
		setCustomState();
	});
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void wall_set_world_matrix(){
	Matrix w, v, p;
	loadMatrix_VP(v, p);

	w.Translation(Vector3(0.0f, 6.0f, 8.0f));
	Matrix  wv = w * v;
	Matrix wvp = wv * p;

	storeMatrix(w, wv, wvp);
}
void wall_draw(ID3D11DeviceContext* pd3dImmediateContext, IEffect* effect, ID3D11InputLayout* inputLayout, _In_opt_ std::function<void __cdecl()> setCustomState){
	G->wall_model->Draw(pd3dImmediateContext, effect, inputLayout, [=]
	{
		setCustomState();
	});
}