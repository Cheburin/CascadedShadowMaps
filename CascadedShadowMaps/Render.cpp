#include "main.h"

#include "DXUTgui.h"
#include "SDKmisc.h"
#include "SDKMesh.h"

extern GraphicResources * G;

extern SwapChainGraphicResources * SCG;

extern SceneState scene_state;

extern CascadeState cascade_state;

extern BlurHandling blur_handling;

extern CDXUTTextHelper*                    g_pTxtHelper;

ID3D11ShaderResourceView* null[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

using namespace SimpleMath;

inline void set_scene_constant_buffer(ID3D11DeviceContext* context){
	G->scene_constant_buffer->SetData(context, scene_state);
};

inline void set_cascade_constant_buffer(ID3D11DeviceContext* context){
	G->cascade_constant_buffer->SetData(context, cascade_state);
};
inline void set_blur_constant_buffer(ID3D11DeviceContext* context){
	//G->blur_constant_buffer->SetData(context, blur_handling);
};

void RenderText()
{
	g_pTxtHelper->Begin();
	g_pTxtHelper->SetInsertionPos(2, 0);
	g_pTxtHelper->SetForegroundColor(D3DXCOLOR(1.0f, 1.0f, 0.0f, 1.0f));
	g_pTxtHelper->DrawTextLine(DXUTGetFrameStats(true && DXUTIsVsyncEnabled()));
	g_pTxtHelper->DrawTextLine(DXUTGetDeviceStats());

	g_pTxtHelper->End();
}

void CALLBACK OnFrameMove(double fTime, float fElapsedTime, void* pUserContext)
{
	Camera::OnFrameMove(fTime, fElapsedTime, pUserContext);
}
void _stdcall ComputeNearAndFar(FLOAT& fNearPlane, FLOAT& fFarPlane, DirectX::XMFLOAT4 vLightCameraOrthographicMin, DirectX::XMFLOAT4 vLightCameraOrthographicMax, DirectX::XMFLOAT4* pvPointsInCameraView);
void _stdcall CreateFrustumPointsFromCascadeInterval(float fCascadeIntervalBegin, float fCascadeIntervalEnd, DirectX::SimpleMath::Matrix &vProjection, DirectX::SimpleMath::Vector4 * pvCornerPointsWorld);
void renderDXUTMesh(ID3D11DeviceContext* context, CDXUTSDKMesh* DXUTMesh, IEffect* effect, ID3D11InputLayout *pInputLayout, _In_opt_ std::function<void __cdecl()> setCustomState);
extern XMFLOAT4 vSceneAABBPointsModelSpace[8];
extern XMFLOAT4 vSceneAABBPointsLightSpace[8];
extern float iCascadePartitionsZeroToOne[8];
extern XMFLOAT4 vLightCameraOrthographicMin;
extern XMFLOAT4 vLightCameraOrthographicMax;
extern XMFLOAT4X4 g_matShadowProj[8];
extern float g_fCascadePartitionsFrustum[8];
extern D3D11_VIEWPORT g_CascadeRenderVP[8];
extern float g_cameraFarPlane;

void RenderShadowsForAllCascades(ID3D11DeviceContext* context);
void RenderScene(ID3D11DeviceContext* context);

void SaveVolumes(ID3D11DeviceContext* context, FCascade * Cascade, FFrustum * Frustum);
void RenderVolumes(ID3D11DeviceContext* context);

void CALLBACK OnD3D11FrameRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* context,
	double fTime, float fElapsedTime, void* pUserContext)
{
	FCascade __Cascade[4];

	FFrustum __Frustum[3];

	const XMFLOAT4 g_vFLTMAX = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
	const XMFLOAT4 g_vFLTMIN = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };

	///// bind constant buffer and samplers to shader slots
	context->VSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));

	context->PSSetConstantBuffers(1, 1, constantBuffersToArray(*(G->cascade_constant_buffer)));

	context->PSSetSamplers(0, 1, samplerStateToArray(G->render_states->AnisotropicWrap()));

	context->PSSetSamplers(1, 1, samplerStateToArray(G->PCFSampler.Get()));
	/////

	//render MeshTestScene 
	{
		auto light_camera1_view = Matrix(scene_state.light_camera1_view).Transpose();
		auto camera_inv_view = Matrix(scene_state.mInvView).Transpose();
		auto camera_view = Matrix(scene_state.mView).Transpose();
		auto camera_projection = Matrix(scene_state.mProjection).Transpose();

		float  fFrustumIntervalEnd, fFrustumIntervalBegin = 0.0f, fCameraNearFarRange = g_cameraFarPlane - 0.05f;

		// Transform the scene AABB to Light space.
		for (int index = 0; index < 8; ++index)
			vSceneAABBPointsLightSpace[index] = Vector4::Transform(Vector4(vSceneAABBPointsModelSpace[index]), light_camera1_view);
		
		// We loop over the cascades to calculate the orthographic projection for each cascade.
		for (INT iCascadeIndex = 0; iCascadeIndex < 3; ++iCascadeIndex)
		{
			fFrustumIntervalEnd = iCascadePartitionsZeroToOne[iCascadeIndex];
			fFrustumIntervalBegin /= 100.0f;
			fFrustumIntervalEnd /= 100.0f;
			fFrustumIntervalBegin = fFrustumIntervalBegin * fCameraNearFarRange;
			fFrustumIntervalEnd = fFrustumIntervalEnd * fCameraNearFarRange;

			Vector4 vFrustumPoints[8];
			// This function takes the began and end intervals along with the projection matrix and returns the 8
			// points that repreresent the cascade Interval
			CreateFrustumPointsFromCascadeInterval(fFrustumIntervalBegin, fFrustumIntervalEnd, camera_projection, vFrustumPoints);

			// This next section of code calculates the min and max values for the orthographic projection.
			XMFLOAT4 vLightCameraOrthographicMin = g_vFLTMAX;
			XMFLOAT4 vLightCameraOrthographicMax = g_vFLTMIN;
			for (int icpIndex = 0; icpIndex < 8; ++icpIndex)
			{
				vFrustumPoints[icpIndex] = Vector4::Transform(vFrustumPoints[icpIndex], camera_inv_view);
				Vector4 corner = Vector4::Transform(vFrustumPoints[icpIndex], light_camera1_view);
				vLightCameraOrthographicMin = Vector4(XMVectorMin(corner, Vector4(vLightCameraOrthographicMin)));
				vLightCameraOrthographicMax = Vector4(XMVectorMax(corner, Vector4(vLightCameraOrthographicMax)));
			}

			// This code removes the shimmering effect along the edges of shadows
			{
				Vector3 FP0 = Vector4(vFrustumPoints[0]);
				Vector3 FP6 = Vector4(vFrustumPoints[6]);

				Vector3 LMIN = Vector4(vLightCameraOrthographicMin);
				Vector3 LMAX = Vector4(vLightCameraOrthographicMax);

				float cascadeBound = (FP0 - FP6).Length();
				float k = cascadeBound / 1024.f;

				Vector3 boarderOffset = Vector3(0.5f, 0.5f, 0.0f) * (Vector3(cascadeBound, cascadeBound, cascadeBound) - (LMAX - LMIN));
				Vector3 invWorldUnitsPerTexel = Vector3(1.0f / k, 1.0f / k, 0.0f);
				Vector3 worldUnitsPerTexel = Vector3(k, k, 0.0f);

				LMIN -= boarderOffset;
				LMAX += boarderOffset;

				LMIN *= invWorldUnitsPerTexel;
				LMAX *= invWorldUnitsPerTexel;

				//vLightCameraOrthographicMin = Vector4(XMVectorFloor(LMIN)) * Vector4(worldUnitsPerTexel);
				//vLightCameraOrthographicMax = Vector4(XMVectorFloor(LMAX)) * Vector4(worldUnitsPerTexel);
			}

			FLOAT fNearPlane = 0.0f;
			FLOAT fFarPlane = 10000.0f;
			ComputeNearAndFar(fNearPlane, fFarPlane, vLightCameraOrthographicMin, vLightCameraOrthographicMax, vSceneAABBPointsLightSpace);

			g_matShadowProj[iCascadeIndex] = Matrix(XMMatrixOrthographicOffCenterLH(
				vLightCameraOrthographicMin.x,
				vLightCameraOrthographicMax.x,
				vLightCameraOrthographicMin.y,
				vLightCameraOrthographicMax.y, 
				fNearPlane, 
				fFarPlane
			));

			g_fCascadePartitionsFrustum[iCascadeIndex] = fFrustumIntervalEnd;

			__Frustum[iCascadeIndex].Quad[0] = Vector4::Transform(vFrustumPoints[4], camera_view);
			__Frustum[iCascadeIndex].Quad[1] = Vector4::Transform(vFrustumPoints[5], camera_view);
			__Frustum[iCascadeIndex].Quad[2] = Vector4::Transform(vFrustumPoints[6], camera_view);
			__Frustum[iCascadeIndex].Quad[3] = Vector4::Transform(vFrustumPoints[7], camera_view);
			__Frustum[iCascadeIndex].InvertOrientation = camera_inv_view.Invert().Transpose();
			__Frustum[iCascadeIndex].Orientation = camera_inv_view.Transpose();

			auto Min = Vector4(vLightCameraOrthographicMin.x, vLightCameraOrthographicMin.y, fNearPlane - 0.f*(fFarPlane-fNearPlane), 1);
			auto Max = Vector4(vLightCameraOrthographicMax.x, vLightCameraOrthographicMax.y, fFarPlane + 0.f*(fFarPlane-fNearPlane), 1);
			__Cascade[iCascadeIndex+1].Min = Min;
			__Cascade[iCascadeIndex+1].Max = Max;
			__Cascade[iCascadeIndex+1].InvertOrientation = light_camera1_view.Transpose();
			__Cascade[iCascadeIndex+1].Orientation = light_camera1_view.Invert().Transpose();
		}

		SaveVolumes(context, __Cascade, __Frustum);

		RenderShadowsForAllCascades(context);

		RenderScene(context);

		RenderVolumes(context);
	}

	RenderText();
}

bool bVolumesSaved;

extern bool bSaveVolumes;

void SaveVolumes(ID3D11DeviceContext* context, FCascade * Cascade, FFrustum * Frustum)
{
	if (bSaveVolumes)
	{
		for (int i = 1; i < 4; i++)
		{
			G->CascadeBuffer->setElement(i, Cascade[i]);
		}
		
		for (int i = 0; i < 3; i++)
		{
			G->FrustumBuffer->setElement(i, Frustum[i]);
		}

		G->CascadeBuffer->applyChanges(context);
		G->FrustumBuffer->applyChanges(context);

		bVolumesSaved = true;
		bSaveVolumes = false;
	}
}

void DrawPoint(ID3D11DeviceContext* pd3dImmediateContext, _In_ IEffect* effect,
	_In_opt_ std::function<void __cdecl()> setCustomState){
	effect->Apply(pd3dImmediateContext);
	setCustomState();

	pd3dImmediateContext->IASetInputLayout(nullptr);
	pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	pd3dImmediateContext->Draw(1, 0);
}

void RenderVolumes(ID3D11DeviceContext* context)
{
	if (bVolumesSaved)
	{
		auto save_proj = scene_state.mProjection;
		//float ratio = scene_state.vFrustumParams.x / scene_state.vFrustumParams.y;
		//float np = 0.05, fp = 100000, fov = D3DX_PI / 3;
		//Matrix m = DirectX::XMMatrixPerspectiveFovLH(fov, ratio, np, fp);
		//scene_state.mProjection = m.Transpose();

		static const Vector4 vCascadeColorsMultiplier[9] =
		{
			Vector4(.5f, .5f, .5f, .5f),
			Vector4(1.5f, 0.0f, 0.0f, .5f),
			Vector4(0.0f, 1.5f, 0.0f, .5f),
			Vector4(0.0f, 0.0f, 5.5f, .5f),
			Vector4(1.5f, 0.0f, 5.5f, .5f),
			Vector4(1.5f, 1.5f, 0.0f, .5f),
			Vector4(1.0f, 1.0f, 1.0f, .5f),
			Vector4(0.0f, 1.0f, 5.5f, .5f),
			Vector4(0.5f, 3.5f, 0.75f, .5f)
		};

		context->GSSetConstantBuffers(0, 1, constantBuffersToArray(*(G->scene_constant_buffer)));
		context->GSSetShaderResources(1, 2, shaderResourceViewToArray(G->CascadeBuffer->getView().asShaderView(), G->FrustumBuffer->getView().asShaderView()));
		context->PSSetShaderResources(1, 2, shaderResourceViewToArray(G->CascadeBuffer->getView().asShaderView(), G->FrustumBuffer->getView().asShaderView()));

		context->OMSetBlendState(G->alfaBlend.Get(), Colors::Black, 0xFFFFFFFF);
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);

		{
			for (int i = 0; i < 2; i++)
			{
				set_scene_world_matrix();
				scene_state.vDebugColor = vCascadeColorsMultiplier[i];
				scene_state.vDebugIndex.w = i;
				set_scene_constant_buffer(context);

				DrawPoint(context, G->debug_box_effect.get(), [=]{
					context->RSSetState(G->render_states->CullClockwise());
				});
			}
			for (int i = 0; i < 1; i++)
			{
				set_scene_world_matrix();
				scene_state.vDebugColor = Vector4(.5f, .5f, .5f, .5f);
				scene_state.vDebugIndex.w = i;
				set_scene_constant_buffer(context);

				DrawPoint(context, G->debug_frustum_effect.get(), [=]{
					context->RSSetState(G->render_states->CullClockwise());
				});
			}
			for (int i = 0; i < 2; i++)
			{
				set_scene_world_matrix();
				scene_state.vDebugColor = vCascadeColorsMultiplier[i];
				scene_state.vDebugIndex.w = i;
				set_scene_constant_buffer(context);

				DrawPoint(context, G->debug_box_effect.get(), [=]{
					context->RSSetState(G->render_states->CullCounterClockwise());
				});
			}
			for (int i = 0; i < 1; i++)
			{
				set_scene_world_matrix();
				scene_state.vDebugColor = Vector4(.5f, .5f, .5f, .5f);
				scene_state.vDebugIndex.w = i;
				set_scene_constant_buffer(context);

				DrawPoint(context, G->debug_frustum_effect.get(), [=]{
					context->RSSetState(G->render_states->CullCounterClockwise());
				});
			}
		}

		//context->OMSetDepthStencilState(G->render_states->DepthNone(), 0);

		context->OMSetBlendState(G->render_states->Opaque(), Colors::Black, 0xFFFFFFFF);
		context->OMSetDepthStencilState(G->render_states->DepthDefault(), 0);

		scene_state.mProjection = save_proj;
	}
}

void RenderShadowsForAllCascades(ID3D11DeviceContext* context)
{
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = 1024*8;
	vp.Height = 1024;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	context->RSSetViewports(1, &vp);

	context->ClearDepthStencilView(G->CascadedShadowMapTextureV.Get(), D3D11_CLEAR_DEPTH, 1.0, 0);
	context->OMSetRenderTargets(1, renderTargetViewToArray(0), G->CascadedShadowMapTextureV.Get());

	context->RSSetState(G->rsShadow.Get());

	for (INT iCascadeIndex = 0; iCascadeIndex < 3; ++iCascadeIndex)
	{
		context->RSSetViewports(1, &g_CascadeRenderVP[iCascadeIndex]);

		set_scene_world_matrix();
		set_scene_object1_matrix(Matrix(g_matShadowProj[iCascadeIndex]));

		set_scene_constant_buffer(context);

		renderDXUTMesh(context, &(G->MeshTestScene), G->depth_testscene_effect.get(), G->testscene_input_layout.Get(), [=](){
		});
	}
}

void RenderScene(ID3D11DeviceContext* context)
{
	D3D11_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = scene_state.vFrustumParams.x;
	vp.Height = scene_state.vFrustumParams.y;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	context->RSSetViewports(1, &vp);

	float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	context->ClearRenderTargetView(DXUTGetD3D11RenderTargetView(), ClearColor);
	context->ClearDepthStencilView(DXUTGetD3D11DepthStencilView(), D3D10_CLEAR_DEPTH, 1.0, 0);
	context->OMSetRenderTargets(1, renderTargetViewToArray(DXUTGetD3D11RenderTargetView()), DXUTGetD3D11DepthStencilView());

	set_scene_world_matrix();
	set_scene_cascade(g_matShadowProj, g_fCascadePartitionsFrustum, 3);

	set_scene_constant_buffer(context);
	set_cascade_constant_buffer(context);

	renderDXUTMesh(context, &(G->MeshTestScene), G->testscene_effect.get(), G->testscene_input_layout.Get(), [=](){
		context->PSSetShaderResources(3, 1, shaderResourceViewToArray(G->CascadedShadowMapTextureSRV.Get()));

		context->RSSetState(G->rsScene.Get());
	});

	context->PSSetShaderResources(3, 1, shaderResourceViewToArray(0));
}