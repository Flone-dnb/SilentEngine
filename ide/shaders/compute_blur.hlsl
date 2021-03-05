// ******************************************************************
// This file is part of the Silent Engine.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

cbuffer cbSettings : register(b0)
{
	// a list of 32 bit values:

	// Blur weights.
	float fW0;
	float fW1;
	float fW2;
	float fW3;
	float fW4;
	float fW5;
	float fW6;
	float fW7;
	float fW8;
	float fW9;
	float fW10;
};


Texture2D inputTexture            : register(t0);
RWTexture2D<float4> outputTexture : register(u0);


#define THREADS_IN_GROUP 256
#define BLUR_RADIUS 5
#define CACHE_SIZE (THREADS_IN_GROUP + 2 * BLUR_RADIUS)


groupshared float4 vGroupCache[CACHE_SIZE];



[numthreads(THREADS_IN_GROUP, 1, 1)]
void horzBlurCS(int3 iGroupThreadID : SV_GroupThreadID, int3 iDispatchThreadID : SV_DispatchThreadID)
{
	float vWeights[11] = { fW0, fW1, fW2, fW3, fW4, fW5, fW6, fW7, fW8, fW9, fW10 };
	
	// Each group needs (THREADS_IN_GROUP + 2 * BLUR_RADIUS) pixels for blur.
	// Fill local thread storage to reduce bandwidth.
	// Each group has THREADS_IN_GROUP threads, but we also need (2 * BLUR_RADIUS) more pixels.
	// To get these (2 * BLUR_RADIUS) pixels first BLUR_RADIUS threads will fetch 2 pixels and
	// BLUR_RADIUS last threads will also fetch 2 pixels, others will fetch only 1 pixel.
	
	float dimx = 0.0f;
	float dimy = 0.0f;
	inputTexture.GetDimensions(dimx, dimy);
	float2 textureDimentions = length(float2(dimx, dimy));

	if(iGroupThreadID.x < BLUR_RADIUS)
	{
		// Clamp left border.
		int iX = max(iDispatchThreadID.x - BLUR_RADIUS, 0);
		vGroupCache[iGroupThreadID.x] = inputTexture[int2(iX, iDispatchThreadID.y)];
	}
	if (iGroupThreadID.x >= THREADS_IN_GROUP - BLUR_RADIUS)
	{
		// Clamp right border.
		

		int iX = min(iDispatchThreadID.x + BLUR_RADIUS, textureDimentions.x - 1);
		vGroupCache[iGroupThreadID.x + 2 * BLUR_RADIUS] = inputTexture[int2(iX, iDispatchThreadID.y)];
	}
	
	// Clamp borders.
	vGroupCache[iGroupThreadID.x + BLUR_RADIUS] = inputTexture[min(iDispatchThreadID.xy, textureDimentions.xy - 1)];
	
	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();
	
	
	// Now blur each pixel.
	
	float4 vBlurColor = float4(0, 0, 0, 0);
	
	for(int i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++)
	{
		int iCacheIndex = iGroupThreadID.x + BLUR_RADIUS + i;
		
		vBlurColor += vWeights[i + BLUR_RADIUS] * vGroupCache[iCacheIndex];
	}
	
	outputTexture[iDispatchThreadID.xy] = vBlurColor;
}


[numthreads(1, THREADS_IN_GROUP, 1)]
void vertBlurCS(int3 iGroupThreadID : SV_GroupThreadID, int3 iDispatchThreadID : SV_DispatchThreadID)
{
	float vWeights[11] = { fW0, fW1, fW2, fW3, fW4, fW5, fW6, fW7, fW8, fW9, fW10 };
	
	// Each group needs (THREADS_IN_GROUP + 2 * BLUR_RADIUS) pixels for blur.
	// Fill local thread storage to reduce bandwidth.
	// Each group has THREADS_IN_GROUP threads, but we also need (2 * BLUR_RADIUS) more pixels.
	// To get these (2 * BLUR_RADIUS) pixels first BLUR_RADIUS threads will fetch 2 pixels and
	// BLUR_RADIUS last threads will also fetch 2 pixels, others will fetch only 1 pixel.

	float dimx = 0.0f;
	float dimy = 0.0f;
	inputTexture.GetDimensions(dimx, dimy);
	float2 textureDimentions = length(float2(dimx, dimy));
	
	if(iGroupThreadID.y < BLUR_RADIUS)
	{
		// Clamp left border.
		int iY = max(iDispatchThreadID.y - BLUR_RADIUS, 0);
		vGroupCache[iGroupThreadID.y] = inputTexture[int2(iDispatchThreadID.x, iY)];
	}
	if (iGroupThreadID.y >= THREADS_IN_GROUP - BLUR_RADIUS)
	{
		// Clamp right border.
		int iY = min(iDispatchThreadID.y + BLUR_RADIUS, textureDimentions.y - 1);
		vGroupCache[iGroupThreadID.y + 2 * BLUR_RADIUS] = inputTexture[int2(iDispatchThreadID.x, iY)];
	}
	
	// Clamp borders.
	vGroupCache[iGroupThreadID.y + BLUR_RADIUS] = inputTexture[min(iDispatchThreadID.xy, textureDimentions.xy - 1)];
	
	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();
	
	
	// Now blur each pixel.
	
	float4 vBlurColor = float4(0, 0, 0, 0);
	
	for(int i = -BLUR_RADIUS; i <= BLUR_RADIUS; i++)
	{
		int iCacheIndex = iGroupThreadID.y + BLUR_RADIUS + i;
		
		vBlurColor += vWeights[i + BLUR_RADIUS] * vGroupCache[iCacheIndex];
	}
	
	outputTexture[iDispatchThreadID.xy] = vBlurColor;
}