// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

#include <DirectXMath.h>

//@@Class
/*
The class is used to store 3-4 values, which can represent a vector or a point in space.
*/
class SVector
{
public:
	//@@Function
	/*
	* desc: the constructor which initializes the vector with zeros.
	*/
	SVector();
	//@@Function
	/*
	* desc: the constructor which initializes the vector with the given values, 4th component of the vector is initialized as zero.
	*/
	SVector(float fX, float fY, float fZ);
	//@@Function
	/*
	* desc: the constructor which initializes the vector with the given values.
	*/
	SVector(float fX, float fY, float fZ, float fW);

	//@@Function
	/*
	* desc: sets the X component of the vector.
	*/
	void  setX(float fX);
	//@@Function
	/*
	* desc: sets the Y component of the vector.
	*/
	void  setY(float fY);
	//@@Function
	/*
	* desc: sets the Z component of the vector.
	*/
	void  setZ(float fZ);
	//@@Function
	/*
	* desc: sets the W component of the vector (4th component of the vector).
	*/
	void  setW(float fW);

	//@@Function
	/*
	* desc: returns the X component of the vector.
	*/
	float getX() const;
	//@@Function
	/*
	* desc: returns the Y component of the vector.
	*/
	float getY() const;
	//@@Function
	/*
	* desc: returns the Z component of the vector.
	*/
	float getZ() const;
	//@@Function
	/*
	* desc: returns the W component of the vector (4th component of the vector).
	*/
	float getW() const;

	//@@Function
	/*
	* desc: normalized the vector.
	*/
	void  normalizeVector     ();
	//@@Function
	/*
	* desc: returns the length of the vector.
	*/
	float length              ();

	//@@Function
	/*
	* desc: returns the result of a dot product between this vector and another one.
	*/
	float dotProduct                (const SVector& b);
	//@@Function
	/*
	* desc: saves the result of a cross product between this vector and another one in this vector.
	*/
	void  crossProduct              (const SVector& b);
	//@@Function
	/*
	* desc: returns the angle between this vector and the given vector in radians.
	* param "bVectorsNormalized": set to true if this vector and b vector are normalized,
	using this function on normalized vectors is faster.
	*/
	float angleBetweenVectorsInRad  (const SVector& b, bool bVectorsNormalized = false);
	//@@Function
	/*
	* desc: returns the angle between this vector and the given vector in degrees.
	* param "bVectorsNormalized": set to true if this vector and b vector are normalized,
	using this function on normalized vectors is faster.
	*/
	float angleBetweenVectorsInDeg  (const SVector& b, bool bVectorsNormalized = false);

	//@@Function
	/*
	* desc: rotates the vector around the given axis.
	*/
	void  rotateAroundAxis          (const SVector& axis, float fAngleInGrad);

	//@@Function
	/*
	* desc: converts the spherical coordinate system coordinates to the cartesian coordinate system coordinates.
	*/
	static SVector sphericalToCartesianCoords (float fRadius, float fTheta, float fPhi);


	SVector operator+(const SVector& b);
	SVector operator-(const SVector& b);
	SVector operator*(const SVector& b);
	SVector operator/(const SVector& b);

	SVector operator+(const float& b);
	SVector operator-(const float& b);
	SVector operator*(const float& b);
	SVector operator/(const float& b);

	bool   operator==(const SVector& b);

private:

	//@@Variable
	/* the 3D vector that holds the vector data. */
	DirectX::XMFLOAT3 vVector;
	//@@Variable
	/* the variable that stores the value of the 4th component of the vector. */
	float fW;
};

