/******************************************************************************
* esdf
* Copyright (C) 2015  Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <camera.h>
#include <graphics.h>
#include <constants.h>
#include <ui/ui.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/norm.hpp>

// Initialize
_Camera::_Camera(const glm::vec2 &Position, float Distance, float UpdateDivisor)
:	LastPosition(Position),
	Position(Position),
	TargetPosition(Position),
	Distance(Distance),
	TargetDistance(Distance),
	Fovy(CAMERA_FOVY),
	UpdateDivisor(UpdateDivisor) {

	// Set up frustum
	Near = CAMERA_NEAR;
	Far = CAMERA_FAR;
}

// Shutdown
_Camera::~_Camera() {
}

void _Camera::CalculateFrustum(float AspectRatio) {
	Frustum[1] = tan(Fovy / 360 * M_PI) * Near;
	Frustum[0] = Frustum[1] * AspectRatio;
	Projection = glm::frustum(-Frustum[0], Frustum[0], Frustum[1], -Frustum[1], Near, Far);

	Update(0);
}

// Set up 3d projection matrix
void _Camera::Set3DProjection(double BlendFactor) {
	glm::vec2 DrawPosition(Position * (float)BlendFactor + LastPosition * (1.0f - (float)BlendFactor));

	float Width = Distance * Graphics.AspectRatio;
	float Height = Distance;

	// Get AABB at z=0
	AABB[0] = -Width + DrawPosition.x;
	AABB[1] = -Height + DrawPosition.y;
	AABB[2] = Width + DrawPosition.x;
	AABB[3] = Height + DrawPosition.y;

	Transform = Projection * glm::translate(glm::mat4(1.0f), glm::vec3(-DrawPosition.x, -DrawPosition.y, -Distance));
}

// Converts screen space to world space
void _Camera::ConvertScreenToWorld(const glm::ivec2 &Point, glm::vec2 &WorldPosition) {
	WorldPosition.x = (Point.x / (float)(Graphics.ViewportSize.x) - 0.5f) * Distance * Graphics.AspectRatio * 2  + Position.x;
	WorldPosition.y = (Point.y / (float)(Graphics.ViewportSize.y) - 0.5f) * Distance * 2 + Position.y;
}

// Converts world space to screen space
void _Camera::ConvertWorldToScreen(const glm::vec2 &WorldPosition, glm::ivec2 &Point) {
	Point.x = Graphics.ViewportSize.x * (0.5f + ((WorldPosition.x - Position.x) / (Distance * Graphics.AspectRatio * 2)));
	Point.y = Graphics.ViewportSize.y * (0.5f + ((WorldPosition.y - Position.y) / (Distance * 2)));
}

// Update camera
void _Camera::Update(double FrameTime) {
	LastPosition = Position;
	if(TargetDistance <= 1.0f)
		TargetDistance = 1.0f;
	else if(TargetDistance >= Far)
		TargetDistance = Far;

	glm::vec2 Delta(TargetPosition - Position);
	if(std::abs(Delta.x) > 0.01f)
		Position.x += Delta.x / UpdateDivisor;
	if(std::abs(Delta.y) > 0.01f)
		Position.y += Delta.y / UpdateDivisor;

	float DeltaZ = TargetDistance - Distance;
	if(std::abs(DeltaZ) > 0.01f)
		Distance += DeltaZ / UpdateDivisor;

}

// Determines whether a circle is in view
bool _Camera::IsCircleInView(const glm::vec2 &Center, float Radius) const {

	// Get closest point on AABB
	glm::vec2 Point(Center);
	if(Point.x < AABB[0])
		Point.x = AABB[0];
	if(Point.y < AABB[1])
		Point.y = AABB[1];
	if(Point.x > AABB[2])
		Point.x = AABB[2];
	if(Point.y > AABB[3])
		Point.y = AABB[3];

	// Test circle collision with point
	float DistanceSquared = glm::distance2(Point, Center);
	bool Hit = DistanceSquared < Radius * Radius;

	return Hit;
}

// Determines whether an AABB is in view
bool _Camera::IsAABBInView(const float *Bounds) const {

	if(Bounds[2] < AABB[0] || Bounds[0] > AABB[2])
		return false;
	if(Bounds[3] < AABB[1] || Bounds[1] > AABB[3])
		return false;

	return true;
}