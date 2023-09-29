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
float3 FrustumCorners[4];

struct VSFrustumRayInput
{
  float4 Position : POSITION0;
  float2 TexCoord : TEXCOORD0;    // The texture coordinate of one of the texture corners.
                                  // Allowed values are (0, 0), (1, 0), (0, 1), and (1, 1).
};

struct VSFrustumRayOutput
{
  float2 TexCoord : TEXCOORD0;    // The texture coordinates of the vertex.
  float3 FrustumRay : TEXCOORD1;
  float4 Position : SV_Position;
};

// Gets the index of the given texture corner.
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

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

VSFrustumRayOutput VS(VSFrustumRayInput input)
{
    float4 position = input.Position;
    float2 texCoord = input.TexCoord;
  
    float2 viewportSize = float2(1600, 900);
    position.xy /= viewportSize;
  
    texCoord.xy = position.xy;
  
 
    position.xy = position.xy * float2(2, -2) - float2(1, -1);
  
    VSFrustumRayOutput output = (VSFrustumRayOutput) 0;
    output.Position = position;
    output.TexCoord = texCoord;
    output.FrustumRay = FrustumCorners[GetCornerIndex(input.TexCoord)];
  
    return output;
}


float4 PS(VSFrustumRayOutput input) : COLOR0
{
  return float4(1, 1, 1, 1);
}