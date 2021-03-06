#include "camera.h"
#include <limits>
#include "graphics/graphics.h"

float Camera::get_zoom_factor() const
{
	if (_viewport_size.height == 0)
		return 0.0f;

	return _ortho_size / (float(_viewport_size.height) / 2.0f);
}

float Camera::get_ppu() const
{
	return float(_viewport_size.height) / (2.0f * _ortho_size);
}

void Camera::set_viewport_size(const uSize& viewportSize)
{
	_viewport_size = viewportSize;
	set_aspect_ratio(float(viewportSize.width) / float(viewportSize.height));
}

void Camera::set_orthographic_size(float size)
{
	_ortho_size = size;

	touch();
}

void Camera::set_fov(float fFOVY)
{
	// Skip if no-op
	if (_fov == fFOVY)
		return;

	// Update projection matrix and view frustum
	_fov = fFOVY;

	touch();
}

void Camera::set_projection_mode(ProjectionMode Mode)
{
	// Bail if this is a no op.
	if (Mode == _projection_mode)
		return;

	// Alter the projection mode.
	_projection_mode = Mode;

	touch();
}

void Camera::set_near_clip(float fDistance)
{
	// Skip if this is a no-op
	if (fDistance == _near_clip)
		return;

	// Store value
	_near_clip = fDistance;

	touch();

	// Make sure near clip is less than the far clip
	if (_near_clip > _far_clip)
		set_far_clip(_near_clip);
}

void Camera::set_far_clip(float fDistance)
{
	// Skip if this is a no-op
	if (fDistance == _far_clip)
		return;

	// Store value
	_far_clip = fDistance;

	touch();

	// Make sure near clip is less than the far clip
	if (_near_clip > _far_clip)
		set_near_clip(_far_clip);
}

math::bbox Camera::get_local_bounding_box()
{
	if (_projection_mode == ProjectionMode::Perspective)
	{
		float fNearSize = math::tan(math::radians<float>(_fov*0.5f)) * _near_clip;
		float fFarSize = math::tan(math::radians<float>(_fov*0.5f)) * _far_clip;
		return math::bbox(-fFarSize* _aspect_ratio, -fFarSize, _near_clip, fFarSize* _aspect_ratio, fFarSize, _far_clip);
	}
	else
	{
		float spread = _far_clip - _near_clip;
		math::vec3 center = { 0.0f, 0.0f, (_far_clip + _near_clip) * 0.5f };
		float orthographicSize = get_ortho_size();
		math::vec3 size = { orthographicSize * 2.0f * _aspect_ratio, orthographicSize * 2.0f, spread };	
		return math::bbox(center - (size / 2.0f), center + (size / 2.0f));
	}
}

void Camera::set_aspect_ratio(float fAspect, bool bLocked /* = false */)
{
	// Is this a no-op?
	if (fAspect == _aspect_ratio)
	{
		_aspect_locked = bLocked;
		return;

	} // End if aspect is the same

	// Update camera properties
	_aspect_ratio = fAspect;
	_aspect_locked = bLocked;
	_aspect_dirty = true;
	_frustum_dirty = true;
	_projection_dirty = true;
}

bool Camera::is_aspect_locked() const
{
	return (get_projection_mode() == ProjectionMode::Orthographic || _aspect_locked);
}

const math::transform_t & Camera::get_projection()
{

	// Only update matrix if something has changed
	if (get_projection_mode() == ProjectionMode::Perspective)
	{
		if (_projection_dirty)
		{
			// Generate the updated perspective projection matrix
			float fov_radians = math::radians<float>(get_fov());
			_projection = math::perspective(fov_radians, _aspect_ratio, _near_clip, _far_clip, gfx::is_homogeneous_depth());
			_projection[2][0] += _aa_data.z;
			_projection[2][1] += _aa_data.w;
			// Matrix has been updated
			_projection_dirty = false;
			_aspect_dirty = false;

		} // End if projection matrix needs updating
		else if (_aspect_dirty)
		{
			// Just alter the aspect ratio
			_projection[0][0] = _projection[1][1] / _aspect_ratio;

			// Matrix has been updated
			_aspect_dirty = false;

		} // End if only aspect ratio changed

	} // End if perspective
	else if (get_projection_mode() == ProjectionMode::Orthographic)
	{
		if (_projection_dirty || _aspect_dirty)
		{
			// Generate the updated orthographic projection matrix
			float zoom = get_zoom_factor();
			const fRect rect =
			{
				-(float)_viewport_size.width / 2.0f,
				(float)_viewport_size.height / 2.0f,
				(float)_viewport_size.width / 2.0f,
				-(float)_viewport_size.height / 2.0f
			};
			_projection = math::ortho(rect.left * zoom, rect.right * zoom, rect.bottom * zoom, rect.top * zoom, get_near_clip(), get_far_clip(), gfx::is_homogeneous_depth());
			_projection[2][0] += _aa_data.z;
			_projection[2][1] += _aa_data.w;
			// Matrix has been updated
			_projection_dirty = false;
			_aspect_dirty = false;

		} // End if projection matrix needs updating

	} // End if orthographic

	// Return the projection matrix.
	return _projection;
}

void Camera::look_at(const math::vec3 & vEye, const math::vec3 & vAt)
{
	_view.look_at(vEye, vAt);

	touch();
}

void Camera::look_at(const math::vec3 & vEye, const math::vec3 & vAt, const math::vec3 & vUp)
{
	_view.look_at(vEye, vAt, vUp);

	touch();
}

math::vec3 Camera::get_position() const
{
	return math::inverse(_view).get_position();
}

math::vec3 Camera::x_unit_axis() const
{
	return math::inverse(_view).x_unit_axis();
}
math::vec3 Camera::y_unit_axis() const
{
	return math::inverse(_view).y_unit_axis();
}

math::vec3 Camera::z_unit_axis() const
{
	return math::inverse(_view).z_unit_axis();
}

const math::frustum & Camera::get_frustum()
{
	// Recalculate frustum if necessary
	if (_frustum_dirty == true && _frustum_locked == false)
	{
		_frustum.update(get_view(), get_projection(), gfx::is_homogeneous_depth());
		_frustum_dirty = false;

		// Also build the frustum / volume that represents the space between the
		// camera position and its near plane. This frustum represents the
		// 'volume' that can end up clipping geometry.
		using namespace math::VolumeGeometry;
		using namespace math::VolumePlane;
		_clipping_volume = _frustum;
		_clipping_volume.planes[Far].data.w = -_clipping_volume.planes[Near].data.w; // At near plane
		_clipping_volume.planes[Near].data.w = -math::dot((math::vec3&)_clipping_volume.planes[Near], get_position()); // At camera

		// The corner points also need adjusting in this case such that they sit
		// precisely on the new planes.
		_clipping_volume.points[LeftBottomFar] = _clipping_volume.points[LeftBottomNear];
		_clipping_volume.points[LeftTopFar] = _clipping_volume.points[LeftTopNear];
		_clipping_volume.points[RightBottomFar] = _clipping_volume.points[RightBottomNear];
		_clipping_volume.points[RightTopFar] = _clipping_volume.points[RightTopNear];
		_clipping_volume.points[LeftBottomNear] = _clipping_volume.position;
		_clipping_volume.points[LeftTopNear] = _clipping_volume.position;
		_clipping_volume.points[RightBottomNear] = _clipping_volume.position;
		_clipping_volume.points[RightTopNear] = _clipping_volume.position;

	} // End if recalc frustum

	// Return the frustum
	return _frustum;
}

const math::frustum & Camera::get_clipping_volume()
{
	// Reclaculate frustum if necessary
	if (_frustum_dirty == true && _frustum_locked == false)
		get_frustum();

	// Return the clipping volume
	return _clipping_volume;
}

math::VolumeQuery::E Camera::bounds_in_frustum(const math::bbox & AABB)
{
	// Recompute the frustum as necessary.
	const math::frustum& f = get_frustum();

	// Request that frustum classifies
	return f.classify_aabb(AABB);
}

math::VolumeQuery::E Camera::bounds_in_frustum(const math::bbox &AABB, const math::transform_t & t)
{
	// Recompute the frustum as necessary.
	const math::frustum & f = get_frustum();

	// Request that frustum classifies
	return math::frustum::classify_obb(f, AABB, t);
}

bool Camera::world_to_viewport(const uPoint& viewport_pos, const uSize& viewport_size, const math::vec3& pos, math::vec3 & point, bool bClipX /* = true */, bool bClipY /* = true */, bool bClipZ /* = true */)
{
	// Ensure we have an up-to-date projection and view matrix
	auto view_proj = get_view_projection();

	// Transform the point into clip space
	math::vec4 vClip = view_proj.matrix() * math::vec4{ pos.x, pos.y, pos.z, 1.0f };

	// Was this clipped?
	if (bClipX == true && (vClip.x < -vClip.w || vClip.x > vClip.w)) return false;
	if (bClipY == true && (vClip.y < -vClip.w || vClip.y > vClip.w)) return false;
	if (bClipZ == true && (vClip.z < 0.0f || vClip.z > vClip.w)) return false;

	// Project!
	const float recipW = 1.0f / vClip.w;
	vClip.x *= recipW;
	vClip.y *= recipW;
	vClip.z *= recipW;

	// Transform to final screen space position
	point.x = ((vClip.x * 0.5f) + 0.5f) * (float)viewport_size.width + _viewport_pos.x;
	point.y = ((vClip.y * -0.5f) + 0.5f) * (float)viewport_size.height + _viewport_pos.y;
	
	point.z = vClip.z;

	// Point on screen!
	return true;
}

bool Camera::viewport_to_ray(const math::vec2 & point, math::vec3 & vecRayStart, math::vec3 & vecRayDir)
{
	math::vec3 vCursor;

	// Ensure we have an up-to-date projection and view matrix
	math::transform_t mtxInvView;
	math::transform_t mtxProj = get_projection();
	math::transform_t mtxView = get_view();
	mtxInvView = math::inverse(mtxView);

	// Transform the pick position from view port space into camera space
	vCursor.x = (((2.0f * (point.x - _viewport_pos.x)) / (float)_viewport_size.width) - 1) / mtxProj[0][0];
	vCursor.y = -(((2.0f * (point.y - _viewport_pos.y)) / (float)_viewport_size.height) - 1) / mtxProj[1][1];
	vCursor.z = 1.0f;

	// Transform the camera space pick ray into 3D space
	if (get_projection_mode() == ProjectionMode::Orthographic)
	{
		// Obtain the ray from the cursor position
		vecRayStart = math::transform_t::transform_coord(vCursor, mtxInvView);
		vecRayDir = (math::vec3&)mtxInvView[2];

	} // End If IsOrthohraphic
	else
	{
		// Obtain the ray from the cursor position
		vecRayStart = mtxInvView.get_position();
		vecRayDir.x = vCursor.x * mtxInvView[0][0] + vCursor.y * mtxInvView[1][0] + vCursor.z * mtxInvView[2][0];
		vecRayDir.y = vCursor.x * mtxInvView[0][1] + vCursor.y * mtxInvView[1][1] + vCursor.z * mtxInvView[2][1];
		vecRayDir.z = vCursor.x * mtxInvView[0][2] + vCursor.y * mtxInvView[1][2] + vCursor.z * mtxInvView[2][2];

	} // End If !IsOrthohraphic
	// Success!
	return true;
}

bool Camera::viewport_to_world(const math::vec2 & point, const math::plane & Plane, math::vec3 & WorldPos, bool clip)
{
	math::vec3 vPickRayDir, vPickRayOrig;
	float   fProjRayLength, fDistance;

	if (clip &&
		((point.x < _viewport_pos.x) || (point.x > (_viewport_pos.x + _viewport_size.width)) ||
		(point.y < _viewport_pos.y) || (point.y > (_viewport_pos.y + _viewport_size.height))))
		return false;

	// Convert the screen coordinates to a ray.
	if (viewport_to_ray(point, vPickRayOrig, vPickRayDir) == false)
		return false;

	// Get the length of the 'adjacent' side of the virtual triangle formed
	// by the direction and normal.
	fProjRayLength = math::dot(vPickRayDir, (math::vec3&)Plane);
	if (math::abs<float>(fProjRayLength) < math::epsilon<float>())
		return false;

	// Calculate distance to plane along its normal
	fDistance = math::dot(vPickRayOrig, (math::vec3&)Plane) + Plane.data.w;

	// If both the "direction" and origin are on the same side of the plane
	// then we can't possibly intersect (perspective rule only)
	if (get_projection_mode() == ProjectionMode::Perspective)
	{
		int nSign1 = (fDistance > 0) ? 1 : (fDistance < 0) ? -1 : 0;
		int nSign2 = (fProjRayLength > 0) ? 1 : (fProjRayLength < 0) ? -1 : 0;
		if (nSign1 == nSign2)
			return false;

	} // End if perspective

	  // Calculate the actual interval (Distance along the adjacent side / length of adjacent side).
	fDistance /= -fProjRayLength;

	// Store the results
	WorldPos = vPickRayOrig + (vPickRayDir * fDistance);

	// Success!
	return true;
}

bool Camera::viewport_to_major_axis(const math::vec2 & point, const math::vec3 & Origin, math::vec3 & WorldPos, math::vec3 & MajorAxis)
{
	return viewport_to_major_axis(point, Origin, z_unit_axis(), WorldPos, MajorAxis);
}

bool Camera::viewport_to_major_axis(const math::vec2 & point, const math::vec3 & Origin, const math::vec3 & Normal, math::vec3 & WorldPos, math::vec3 & MajorAxis)
{
	// First select the major axis plane based on the specified normal
	MajorAxis = math::vec3(1, 0, 0); // YZ

	// Get absolute normal vector
	float x = math::abs<float>(Normal.x);
	float y = math::abs<float>(Normal.y);
	float z = math::abs<float>(Normal.z);

	// If all the components are effectively equal, select one plane
	if (math::abs<float>(x - y) < math::epsilon<float>() && math::abs<float>(x - z) < math::epsilon<float>())
	{
		MajorAxis = math::vec3(0, 0, 1); // XY

	} // End if components equal
	else
	{
		// Calculate which component of the normal is the major axis
		float fNorm = x;
		if (fNorm < y) { fNorm = y; MajorAxis = math::vec3(0, 1, 0); } // XZ
		if (fNorm < z) { fNorm = z; MajorAxis = math::vec3(0, 0, 1); } // XY

	} // End if perform compare

	  // Generate the intersection plane based on this information
	  // and pass through to the standard viewportToWorld method
	math::plane p = math::plane::fromPointNormal(Origin, MajorAxis);
	return viewport_to_world(point, p, WorldPos, false);
}

bool Camera::viewport_to_camera(const math::vec3 & point, math::vec3 & CameraPos)
{
	// Ensure that we have an up-to-date projection and view matrix
	auto & mtxProj = get_projection();
	auto & mtxView = get_view();

	// Transform the pick position from screen space into camera space
	CameraPos.x = (((2.0f * (point.x - _viewport_pos.x)) / (float)_viewport_size.width) - 1) / mtxProj[0][0];
	CameraPos.y = -(((2.0f * (point.y - _viewport_pos.y)) / (float)_viewport_size.height) - 1) / mtxProj[1][1];
	CameraPos.z = get_near_clip();

	// Success!
	return true;
}

float Camera::estimate_zoom_factor(const math::plane & Plane)
{
	math::vec3 vWorld;

	// Just return the actual zoom factor if this is orthographic
	if (get_projection_mode() == ProjectionMode::Orthographic)
		return get_zoom_factor();

	// Otherwise, estimate is based on the distance from the grid plane.
	viewport_to_world(math::vec2((float)_viewport_size.width / 2, (float)_viewport_size.height / 2), Plane, vWorld, false);

	// Perform full position based estimation
	return estimate_zoom_factor(vWorld);
}

//-----------------------------------------------------------------------------
// Name : estimateZoomFactor ()
/// <summary>
/// Given the current viewport type and projection mode, estimate the "zoom"
/// factor that can be used for scaling various operations relative to the
/// "scale" of an object as it appears in the viewport at the specified position.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::estimate_zoom_factor(const math::vec3 & WorldPos)
{
	return estimate_zoom_factor(WorldPos, std::numeric_limits<float>::max());
}

float Camera::estimate_zoom_factor(const math::plane & Plane, float fMax)
{
	// Just return the actual zoom factor if this is orthographic
	if (get_projection_mode() == ProjectionMode::Orthographic)
	{
		float fFactor = get_zoom_factor();
		return math::min(fMax, fFactor);

	} // End if Orthographic
	// Otherwise, estimate is based on the distance from the grid plane.
	math::vec3 vWorld;
	viewport_to_world(math::vec2((float)_viewport_size.width / 2, (float)_viewport_size.height / 2), Plane, vWorld, false);

	// Perform full position based estimation
	return estimate_zoom_factor(vWorld, fMax);
}

float Camera::estimate_zoom_factor(const math::vec3 & WorldPos, float fMax)
{
	// Just return the actual zoom factor if this is orthographic
	if (get_projection_mode() == ProjectionMode::Orthographic)
	{
		float fFactor = get_zoom_factor();
		return math::min(fMax, fFactor);

	} // End if Orthographic

	// New Zoom factor is based on the distance to this position 
	// along the camera's look vector.
	math::vec3 viewPos = math::transform_t::transform_coord(WorldPos, get_view());
	float distance = viewPos.z / ((float)_viewport_size.height * (45.0f / get_fov()));
	return std::min<float>(fMax, distance);
}

math::vec3 Camera::estimate_pick_tolerance(float WireTolerance, const math::vec3 & Pos, const math::transform_t & ObjectTransform)
{
	// Scale tolerance based on estimated world space zoom factor.
	math::vec3 v;
	ObjectTransform.transform_coord(v, Pos);
	WireTolerance *= estimate_zoom_factor(v);

	// Convert into object space tolerance.
	math::vec3 ObjectWireTolerance;
	math::vec3 vAxisScale = ObjectTransform.get_scale();
	ObjectWireTolerance.x = WireTolerance / vAxisScale.x;
	ObjectWireTolerance.y = WireTolerance / vAxisScale.y;
	ObjectWireTolerance.z = WireTolerance / vAxisScale.z;
	return ObjectWireTolerance;
}

void Camera::record_current_matrices()
{
	_last_view = get_view();
	_last_projection = get_projection();
}

void Camera::set_aa_data(const uSize& viewportSize, std::uint32_t currentSubpixelIndex, std::uint32_t temporalAASamples)
{
	if (temporalAASamples > 1)
	{
		float SampleX = math::halton(currentSubpixelIndex, 2) - 0.5f;
		float SampleY = math::halton(currentSubpixelIndex, 3) - 0.5f;
		if (temporalAASamples == 2)
		{

			float SamplesX[] = { -4.0f / 16.0f, 4.0f / 16.0f };
			float SamplesY[] = { 4.0f / 16.0f, -4.0f / 16.0f };

			std::uint32_t Index = currentSubpixelIndex;
			SampleX = SamplesX[Index];
			SampleY = SamplesY[Index];
		}
		else if (temporalAASamples == 3)
		{
			// 3xMSAA
			//   A..
			//   ..B
			//   .C.
			// Rolling circle pattern (A,B,C).
			float SamplesX[] = { -2.0f / 3.0f,  2.0f / 3.0f,  0.0f / 3.0f };
			float SamplesY[] = { -2.0f / 3.0f,  0.0f / 3.0f,  2.0f / 3.0f };
			std::uint32_t Index = currentSubpixelIndex;
			SampleX = SamplesX[Index];
			SampleY = SamplesY[Index];
		}
		else if (temporalAASamples == 4)
		{
			// 4xMSAA
			// Pattern docs: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476218(v=vs.85).aspx
			//   .N..
			//   ...E
			//   W...
			//   ..S.
			// Rolling circle pattern (N,E,S,W).
			float SamplesX[] = { -2.0f / 16.0f,  6.0f / 16.0f, 2.0f / 16.0f, -6.0f / 16.0f };
			float SamplesY[] = { -6.0f / 16.0f, -2.0f / 16.0f, 6.0f / 16.0f,  2.0f / 16.0f };
			std::uint32_t Index = currentSubpixelIndex;
			SampleX = SamplesX[Index];
			SampleY = SamplesY[Index];
		}
		else if (temporalAASamples == 8)
		{
			// This works better than various orderings of 8xMSAA.
			std::uint32_t Index = currentSubpixelIndex;
			SampleX = math::halton(Index, 2) - 0.5f;
			SampleY = math::halton(Index, 3) - 0.5f;
		}
		else
		{
			// More than 8 samples can improve quality.
			std::uint32_t Index = currentSubpixelIndex;
			SampleX = math::halton(Index, 2) - 0.5f;
			SampleY = math::halton(Index, 3) - 0.5f;
		}

		_aa_data = math::vec4(float(currentSubpixelIndex), float(temporalAASamples), SampleX, SampleY);

		float fWidth = static_cast<float>(viewportSize.width);
		float fHeight = static_cast<float>(viewportSize.height);
		_aa_data.z *= (2.0f / fWidth);
		_aa_data.w *= (2.0f / fHeight);

	}
	else
	{
		_aa_data = math::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	_projection_dirty = true;
}

void Camera::touch()
{
	// All modifications require projection matrix and
	// frustum to be updated.
	_view_dirty = true;
	_projection_dirty = true;
	_frustum_dirty = true;
}