//-----------------------------------------------------------------------------
// DigitalRune Engine - Copyright (C) DigitalRune GmbH
// This file is subject to the terms and conditions defined in
// file 'LICENSE.TXT', which is part of this source code package.
//-----------------------------------------------------------------------------
//
/// \file CameraMotionBlur.fx
/// Blurs the image when the camera moves.
//
// see "GPU Gems 3: Motion Blur as a Post-Processing Effect"
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// The viewport size in pixels.
float2 ViewportSize;

float3 FrustumCorners[4];

// The inverse view matrix.
float4x4 ViewInverse;

// The view projection matrix of the last frame.
float4x4 ViewProjOld;

// The blur strength in the range [0, infinity[.
float Strength = 0.6f;

// The number of samples for the blur.
int NumberOfSamples = 8;

// The input texture.
texture SourceTexture;
sampler SourceSampler : register(s0) = sampler_state
{
    Texture = <SourceTexture>;
};

// The normalized linear planar depth (range [0, 1]).
texture GBuffer0;
sampler GBuffer0Sampler = sampler_state
{
    Texture = <GBuffer0>;
    AddressU = CLAMP;
    AddressV = CLAMP;
    MagFilter = POINT;
    MinFilter = POINT;
    MipFilter = NONE;
};

struct VSFrustumRayInput
{
    float4 Position : POSITION0;
    float2 TexCoord : TEXCOORD0; // The texture coordinate of one of the texture corners.
                                  // Allowed values are (0, 0), (1, 0), (0, 1), and (1, 1).
};

struct VSFrustumRayOutput
{
    float2 TexCoord : TEXCOORD0; // The texture coordinates of the vertex.
    float3 FrustumRay : TEXCOORD1;
    float4 Position : SV_Position;
};

/// Gets the linear depth in the range [0,1] from a G-buffer 0 sample.
/// \param[in] gBuffer0    The G-buffer 0 value.
/// \return The linear depth in the range [0, 1].
float GetGBufferDepth(float4 gBuffer0)
{
    return abs(gBuffer0.x);
}

/// Gets the index of the given texture corner.
/// \param[in] texCoord The texture coordinate of one of the texture corners.
///                     Allowed values are (0, 0), (1, 0), (0, 1), and (1, 1).
/// \return The index of the texture corner.
/// \retval 0   left, top
/// \retval 1   right, top
/// \retval 2   left, bottom
/// \retval 3   right, bottom
float GetCornerIndex(in float2 texCoord)
{
    return texCoord.x + (texCoord.y * 2);
}

/// A vertex shader that also converts the position from screen space for clip space and computes
/// the frustum ray for this vertex.
/// \param[in] input            The vertex data (see VSFrustumRayInput).
/// \param[in] viewportSize     The viewport size in pixels.
/// \param[in] frustumCorners   See constant FrustumCorners above.
VSFrustumRayOutput VSFrustumRay(VSFrustumRayInput input,
                                uniform const float2 viewportSize,
                                uniform const float3 frustumCorners[4])
{
    float4 position = input.Position;
    float2 texCoord = input.TexCoord;
  
    position.xy /= viewportSize;
  
    texCoord.xy = position.xy;
  
  // Instead of subtracting the 0.5 pixel offset from the position, we add
  // it to the texture coordinates - because frustumRay is associated with
  // the position output.
#if FIX_HALF_PIXEL
  texCoord.xy += 0.5f / viewportSize;
#endif
  
    position.xy = position.xy * float2(2, -2) - float2(1, -1);
  
    VSFrustumRayOutput output = (VSFrustumRayOutput) 0;
    output.Position = position;
    output.TexCoord = texCoord;
    output.FrustumRay = frustumCorners[GetCornerIndex(input.TexCoord)];
  
    return output;
}


//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

VSFrustumRayOutput VS(VSFrustumRayInput input)
{
    return VSFrustumRay(input, ViewportSize, FrustumCorners);
}


float4 PS(float2 texCoord : TEXCOORD0, float3 frustumRay : TEXCOORD1) : COLOR0
{
  // Get depth.
    float4 gBuffer0Sample = tex2Dlod(GBuffer0Sampler, float4(texCoord, 0, 0));
    float depth = GetGBufferDepth(gBuffer0Sample);
  
  // Reconstruct the view space position.
    float4 positionView = float4(frustumRay * depth, 1);
  
  // Get the xy position on the near plane in projection space. This is in the range [-1, 1].
    float2 positionProj = float2(texCoord.x * 2 - 1, (1 - texCoord.y) * 2 - 1);
  
  // Get world space position.
    float4 positionWorld = mul(positionView, ViewInverse);
  
  // Compute positionProj of the last frame.
    float4 positionProjOld = mul(positionWorld, ViewProjOld);
  
  // Perspective divide.
    positionProjOld /= positionProjOld.w;
  
  // Get screen space velocity.
  // Divide by 2 to convert from homogenous clip space [-1, 1] to texture space [0, 1].
    float2 velocity = -(positionProj - positionProjOld.xy) / 2 / NumberOfSamples * Strength;
    velocity.y = -velocity.y;
    texCoord -= velocity * NumberOfSamples / 2;
  
  // Blur in velocity direction.
    float4 color = float4(0, 0, 0, 0);
    float weightSum = 0;
    float weightDelta = 1 / (float) NumberOfSamples * 0.5;
    for (int i = 0; i < NumberOfSamples; i++)
    {
        float weight = 1 - i * weightDelta;
        color += tex2D(SourceSampler, texCoord) * weight;
        texCoord += velocity;
        weightSum += weight;
    }
  
    color /= weightSum;
    return color;
}
