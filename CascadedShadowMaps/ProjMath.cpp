#include "DirectXHelpers.h"
#include "SimpleMath.h"

#include "DirectXMath.h"

#include "DXUT.h"

namespace XNA{
	#include <xnamath.h>

	#pragma warning(push)
	#pragma warning(disable: 4324)
		_DECLSPEC_ALIGN_16_ struct Frustum
		{
			XMFLOAT3 Origin;            // Origin of the frustum (and projection).
			XMFLOAT4 Orientation;       // Unit quaternion representing rotation.

			FLOAT RightSlope;           // Positive X slope (X/Z).
			FLOAT LeftSlope;            // Negative X slope.
			FLOAT TopSlope;             // Positive Y slope (Y/Z).
			FLOAT BottomSlope;          // Negative Y slope.
			FLOAT Near, Far;            // Z of the near plane and far plane.
		};
	#pragma warning(pop)
}

using namespace XNA;

//-----------------------------------------------------------------------------
// Build a frustum from a persepective projection matrix.  The matrix may only
// contain a projection; any rotation, translation or scale will cause the
// constructed frustum to be incorrect.
//-----------------------------------------------------------------------------
VOID ComputeFrustumFromProjection(Frustum* pOut, XMMATRIX* pProjection)
{
	// Corners of the projection frustum in homogenous space.
	static XMVECTOR HomogenousPoints[6] =
	{
		{ 1.0f, 0.0f, 1.0f, 1.0f },   // right (at far plane)
		{ -1.0f, 0.0f, 1.0f, 1.0f },   // left
		{ 0.0f, 1.0f, 1.0f, 1.0f },   // top
		{ 0.0f, -1.0f, 1.0f, 1.0f },   // bottom

		{ 0.0f, 0.0f, 0.0f, 1.0f },     // near
		{ 0.0f, 0.0f, 1.0f, 1.0f }      // far
	};

	XMVECTOR Determinant;
	XMMATRIX matInverse = XMMatrixInverse(&Determinant, *pProjection);

	// Compute the frustum corners in world space.
	XMVECTOR Points[6];

	for (INT i = 0; i < 6; i++)
	{
		// Transform point.
		Points[i] = XMVector4Transform(HomogenousPoints[i], matInverse);
	}

	pOut->Origin = XMFLOAT3(0.0f, 0.0f, 0.0f);
	pOut->Orientation = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	// Compute the slopes.
	Points[0] = Points[0] * XMVectorReciprocal(XMVectorSplatZ(Points[0]));
	Points[1] = Points[1] * XMVectorReciprocal(XMVectorSplatZ(Points[1]));
	Points[2] = Points[2] * XMVectorReciprocal(XMVectorSplatZ(Points[2]));
	Points[3] = Points[3] * XMVectorReciprocal(XMVectorSplatZ(Points[3]));

	pOut->RightSlope = XMVectorGetX(Points[0]);
	pOut->LeftSlope = XMVectorGetX(Points[1]);
	pOut->TopSlope = XMVectorGetY(Points[2]);
	pOut->BottomSlope = XMVectorGetY(Points[3]);

	// Compute near and far.
	Points[4] = Points[4] * XMVectorReciprocal(XMVectorSplatW(Points[4]));
	Points[5] = Points[5] * XMVectorReciprocal(XMVectorSplatW(Points[5]));

	pOut->Near = XMVectorGetZ(Points[4]);
	pOut->Far = XMVectorGetZ(Points[5]);

	return;
}

//--------------------------------------------------------------------------------------
void _stdcall CreateFrustumPointsFromCascadeInterval(float fCascadeIntervalBegin,
	float fCascadeIntervalEnd,
	DirectX::SimpleMath::Matrix &_vProjection,
	DirectX::SimpleMath::Vector4 * _pvCornerPointsWorld)
{
	XNA::XMMATRIX vProjection;
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			vProjection.m[i][j] = _vProjection.m[i][j];
	XNA::XMVECTOR pvCornerPointsWorld[8];
   
	////////
	XNA::Frustum vViewFrust;
	ComputeFrustumFromProjection(&vViewFrust, &vProjection);
	vViewFrust.Near = fCascadeIntervalBegin;
	vViewFrust.Far = fCascadeIntervalEnd;

	static const XMVECTORU32 vGrabY = { 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000 };
	static const XMVECTORU32 vGrabX = { 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 };

	XMVECTORF32 vRightTop = { vViewFrust.RightSlope, vViewFrust.TopSlope, 1.0f, 1.0f };
	XMVECTORF32 vLeftBottom = { vViewFrust.LeftSlope, vViewFrust.BottomSlope, 1.0f, 1.0f };
	XMVECTORF32 vNear = { vViewFrust.Near, vViewFrust.Near, vViewFrust.Near, 1.0f };
	XMVECTORF32 vFar = { vViewFrust.Far, vViewFrust.Far, vViewFrust.Far, 1.0f };
	XMVECTOR vRightTopNear = XMVectorMultiply(vRightTop, vNear);
	XMVECTOR vRightTopFar = XMVectorMultiply(vRightTop, vFar);
	XMVECTOR vLeftBottomNear = XMVectorMultiply(vLeftBottom, vNear);
	XMVECTOR vLeftBottomFar = XMVectorMultiply(vLeftBottom, vFar);

	pvCornerPointsWorld[0] = vRightTopNear;
	pvCornerPointsWorld[1] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabX);
	pvCornerPointsWorld[2] = vLeftBottomNear;
	pvCornerPointsWorld[3] = XMVectorSelect(vRightTopNear, vLeftBottomNear, vGrabY);

	pvCornerPointsWorld[4] = vRightTopFar;
	pvCornerPointsWorld[5] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabX);
	pvCornerPointsWorld[6] = vLeftBottomFar;
	pvCornerPointsWorld[7] = XMVectorSelect(vRightTopFar, vLeftBottomFar, vGrabY);
	////////

	for (int i = 0; i < 8; i++)
	{
		_pvCornerPointsWorld[i].x = pvCornerPointsWorld[i].m128_f32[0];
		_pvCornerPointsWorld[i].y = pvCornerPointsWorld[i].m128_f32[1];
		_pvCornerPointsWorld[i].z = pvCornerPointsWorld[i].m128_f32[2];
		_pvCornerPointsWorld[i].w = pvCornerPointsWorld[i].m128_f32[3];
	}
			
}

void _stdcall ComputeNearAndFar(FLOAT& fNearPlane,
	FLOAT& fFarPlane,
	DirectX::XMFLOAT4 LightCameraOrthographicMin,
	DirectX::XMFLOAT4 LightCameraOrthographicMax,
	DirectX::XMFLOAT4* PointsInCameraView)
{
	XNA::XMVECTOR vLightCameraOrthographicMin;
	XNA::XMVECTOR vLightCameraOrthographicMax;
	XNA::XMVECTOR pvPointsInCameraView[8];

	vLightCameraOrthographicMin.m128_f32[0] = LightCameraOrthographicMin.x;
	vLightCameraOrthographicMin.m128_f32[1] = LightCameraOrthographicMin.y;
	vLightCameraOrthographicMin.m128_f32[2] = LightCameraOrthographicMin.z;
	vLightCameraOrthographicMin.m128_f32[3] = LightCameraOrthographicMin.w;

	vLightCameraOrthographicMax.m128_f32[0] = LightCameraOrthographicMax.x;
	vLightCameraOrthographicMax.m128_f32[1] = LightCameraOrthographicMax.y;
	vLightCameraOrthographicMax.m128_f32[2] = LightCameraOrthographicMax.z;
	vLightCameraOrthographicMax.m128_f32[3] = LightCameraOrthographicMax.w;

	for (int i = 0; i < 8; i++)
	{
		pvPointsInCameraView[i].m128_f32[0] = PointsInCameraView[i].x;
		pvPointsInCameraView[i].m128_f32[1] = PointsInCameraView[i].y;
		pvPointsInCameraView[i].m128_f32[2] = PointsInCameraView[i].z;
		pvPointsInCameraView[i].m128_f32[3] = PointsInCameraView[i].w;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Triangle
	{
		XNA::XMVECTOR pt[3];
		BOOL culled;
	};
	// Initialize the near and far planes
	fNearPlane = FLT_MAX;
	fFarPlane = -FLT_MAX;

	Triangle triangleList[16];
	INT iTriangleCnt = 1;

	triangleList[0].pt[0] = pvPointsInCameraView[0];
	triangleList[0].pt[1] = pvPointsInCameraView[1];
	triangleList[0].pt[2] = pvPointsInCameraView[2];
	triangleList[0].culled = false;

	// These are the indices used to tesselate an AABB into a list of triangles.
	static const INT iAABBTriIndexes[] =
	{
		0, 1, 2, 1, 2, 3,
		4, 5, 6, 5, 6, 7,
		0, 2, 4, 2, 4, 6,
		1, 3, 5, 3, 5, 7,
		0, 1, 4, 1, 4, 5,
		2, 3, 6, 3, 6, 7
	};

	INT iPointPassesCollision[3];

	// At a high level: 
	// 1. Iterate over all 12 triangles of the AABB.  
	// 2. Clip the triangles against each plane. Create new triangles as needed.
	// 3. Find the min and max z values as the near and far plane.

	//This is easier because the triangles are in camera spacing making the collisions tests simple comparisions.

	float fLightCameraOrthographicMinX = XMVectorGetX(vLightCameraOrthographicMin);
	float fLightCameraOrthographicMaxX = XMVectorGetX(vLightCameraOrthographicMax);
	float fLightCameraOrthographicMinY = XMVectorGetY(vLightCameraOrthographicMin);
	float fLightCameraOrthographicMaxY = XMVectorGetY(vLightCameraOrthographicMax);

	for (INT AABBTriIter = 0; AABBTriIter < 12; ++AABBTriIter)
	{

		triangleList[0].pt[0] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 0]];
		triangleList[0].pt[1] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 1]];
		triangleList[0].pt[2] = pvPointsInCameraView[iAABBTriIndexes[AABBTriIter * 3 + 2]];
		iTriangleCnt = 1;
		triangleList[0].culled = FALSE;

		// Clip each invidual triangle against the 4 frustums.  When ever a triangle is clipped into new triangles, 
		//add them to the list.
		for (INT frustumPlaneIter = 0; frustumPlaneIter < 4; ++frustumPlaneIter)
		{

			FLOAT fEdge;
			INT iComponent;

			if (frustumPlaneIter == 0)
			{
				fEdge = fLightCameraOrthographicMinX; // todo make float temp
				iComponent = 0;
			}
			else if (frustumPlaneIter == 1)
			{
				fEdge = fLightCameraOrthographicMaxX;
				iComponent = 0;
			}
			else if (frustumPlaneIter == 2)
			{
				fEdge = fLightCameraOrthographicMinY;
				iComponent = 1;
			}
			else
			{
				fEdge = fLightCameraOrthographicMaxY;
				iComponent = 1;
			}

			for (INT triIter = 0; triIter < iTriangleCnt; ++triIter)
			{
				// We don't delete triangles, so we skip those that have been culled.
				if (!triangleList[triIter].culled)
				{
					INT iInsideVertCount = 0;
					XMVECTOR tempOrder;
					// Test against the correct frustum plane.
					// This could be written more compactly, but it would be harder to understand.

					if (frustumPlaneIter == 0)
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (XMVectorGetX(triangleList[triIter].pt[triPtIter]) >
								XMVectorGetX(vLightCameraOrthographicMin))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}
					else if (frustumPlaneIter == 1)
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (XMVectorGetX(triangleList[triIter].pt[triPtIter]) <
								XMVectorGetX(vLightCameraOrthographicMax))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}
					else if (frustumPlaneIter == 2)
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (XMVectorGetY(triangleList[triIter].pt[triPtIter]) >
								XMVectorGetY(vLightCameraOrthographicMin))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}
					else
					{
						for (INT triPtIter = 0; triPtIter < 3; ++triPtIter)
						{
							if (XMVectorGetY(triangleList[triIter].pt[triPtIter]) <
								XMVectorGetY(vLightCameraOrthographicMax))
							{
								iPointPassesCollision[triPtIter] = 1;
							}
							else
							{
								iPointPassesCollision[triPtIter] = 0;
							}
							iInsideVertCount += iPointPassesCollision[triPtIter];
						}
					}

					// Move the points that pass the frustum test to the begining of the array.
					if (iPointPassesCollision[1] && !iPointPassesCollision[0])
					{
						tempOrder = triangleList[triIter].pt[0];
						triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
						triangleList[triIter].pt[1] = tempOrder;
						iPointPassesCollision[0] = TRUE;
						iPointPassesCollision[1] = FALSE;
					}
					if (iPointPassesCollision[2] && !iPointPassesCollision[1])
					{
						tempOrder = triangleList[triIter].pt[1];
						triangleList[triIter].pt[1] = triangleList[triIter].pt[2];
						triangleList[triIter].pt[2] = tempOrder;
						iPointPassesCollision[1] = TRUE;
						iPointPassesCollision[2] = FALSE;
					}
					if (iPointPassesCollision[1] && !iPointPassesCollision[0])
					{
						tempOrder = triangleList[triIter].pt[0];
						triangleList[triIter].pt[0] = triangleList[triIter].pt[1];
						triangleList[triIter].pt[1] = tempOrder;
						iPointPassesCollision[0] = TRUE;
						iPointPassesCollision[1] = FALSE;
					}

					if (iInsideVertCount == 0)
					{ // All points failed. We're done,  
						triangleList[triIter].culled = true;
					}
					else if (iInsideVertCount == 1)
					{// One point passed. Clip the triangle against the Frustum plane
						triangleList[triIter].culled = false;

						// 
						XMVECTOR vVert0ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[0];
						XMVECTOR vVert0ToVert2 = triangleList[triIter].pt[2] - triangleList[triIter].pt[0];

						// Find the collision ratio.
						FLOAT fHitPointTimeRatio = fEdge - XMVectorGetByIndex(triangleList[triIter].pt[0], iComponent);
						// Calculate the distance along the vector as ratio of the hit ratio to the component.
						FLOAT fDistanceAlongVector01 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert1, iComponent);
						FLOAT fDistanceAlongVector02 = fHitPointTimeRatio / XMVectorGetByIndex(vVert0ToVert2, iComponent);
						// Add the point plus a percentage of the vector.
						vVert0ToVert1 *= fDistanceAlongVector01;
						vVert0ToVert1 += triangleList[triIter].pt[0];
						vVert0ToVert2 *= fDistanceAlongVector02;
						vVert0ToVert2 += triangleList[triIter].pt[0];

						triangleList[triIter].pt[1] = vVert0ToVert2;
						triangleList[triIter].pt[2] = vVert0ToVert1;

					}
					else if (iInsideVertCount == 2)
					{ // 2 in  // tesselate into 2 triangles


						// Copy the triangle\(if it exists) after the current triangle out of
						// the way so we can override it with the new triangle we're inserting.
						triangleList[iTriangleCnt] = triangleList[triIter + 1];

						triangleList[triIter].culled = false;
						triangleList[triIter + 1].culled = false;

						// Get the vector from the outside point into the 2 inside points.
						XMVECTOR vVert2ToVert0 = triangleList[triIter].pt[0] - triangleList[triIter].pt[2];
						XMVECTOR vVert2ToVert1 = triangleList[triIter].pt[1] - triangleList[triIter].pt[2];

						// Get the hit point ratio.
						FLOAT fHitPointTime_2_0 = fEdge - XMVectorGetByIndex(triangleList[triIter].pt[2], iComponent);
						FLOAT fDistanceAlongVector_2_0 = fHitPointTime_2_0 / XMVectorGetByIndex(vVert2ToVert0, iComponent);
						// Calcaulte the new vert by adding the percentage of the vector plus point 2.
						vVert2ToVert0 *= fDistanceAlongVector_2_0;
						vVert2ToVert0 += triangleList[triIter].pt[2];

						// Add a new triangle.
						triangleList[triIter + 1].pt[0] = triangleList[triIter].pt[0];
						triangleList[triIter + 1].pt[1] = triangleList[triIter].pt[1];
						triangleList[triIter + 1].pt[2] = vVert2ToVert0;

						//Get the hit point ratio.
						FLOAT fHitPointTime_2_1 = fEdge - XMVectorGetByIndex(triangleList[triIter].pt[2], iComponent);
						FLOAT fDistanceAlongVector_2_1 = fHitPointTime_2_1 / XMVectorGetByIndex(vVert2ToVert1, iComponent);
						vVert2ToVert1 *= fDistanceAlongVector_2_1;
						vVert2ToVert1 += triangleList[triIter].pt[2];
						triangleList[triIter].pt[0] = triangleList[triIter + 1].pt[1];
						triangleList[triIter].pt[1] = triangleList[triIter + 1].pt[2];
						triangleList[triIter].pt[2] = vVert2ToVert1;
						// Cncrement triangle count and skip the triangle we just inserted.
						++iTriangleCnt;
						++triIter;


					}
					else
					{ // all in
						triangleList[triIter].culled = false;

					}
				}// end if !culled loop            
			}
		}
		for (INT index = 0; index < iTriangleCnt; ++index)
		{
			if (!triangleList[index].culled)
			{
				// Set the near and far plan and the min and max z values respectivly.
				for (int vertind = 0; vertind < 3; ++vertind)
				{
					float fTriangleCoordZ = XMVectorGetZ(triangleList[index].pt[vertind]);
					if (fNearPlane > fTriangleCoordZ)
					{
						fNearPlane = fTriangleCoordZ;
					}
					if (fFarPlane  <fTriangleCoordZ)
					{
						fFarPlane = fTriangleCoordZ;
					}
				}
			}
		}
	}
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
}