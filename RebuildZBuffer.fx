//-----------------------------------------------------------------------------
// DigitalRune Engine - Copyright (C) DigitalRune GmbH
// This file is subject to the terms and conditions defined in
// file 'LICENSE.TXT', which is part of this source code package.
//-----------------------------------------------------------------------------
//
/// \file RebuildZBuffer.fx
/// Reconstructs the Z-buffer from the G-buffer and optionally copies a texture
/// into the primary render target. (Because of different precision of the depth
/// values in the G-buffer, the resulting Z-buffer is only an approximation of
/// the original Z-buffer.)
//
//-----------------------------------------------------------------------------

/// Declares the uniform const for a G-buffer texture + sampler.
/// \param[in] name   The name of the texture constant.
/// \param[in] index  The index of the G-buffer.
/// \remarks
/// Example: To declare GBuffer0 and GBuffer0Sampler call
///   DECLARE_UNIFORM_GBUFFER(GBuffer0, 0);
/// Usually you will use
///  DECLARE_UNIFORM_GBUFFER(GBuffer0, 0);
///  DECLARE_UNIFORM_GBUFFER(GBuffer1, 1);
#define DECLARE_UNIFORM_GBUFFER(name, index) \
texture name : GBUFFER##index; \
sampler name##Sampler = sampler_state \
{ \
  Texture = <name>; \
  AddressU  = CLAMP; \
  AddressV  = CLAMP; \
  MinFilter = POINT; \
  MagFilter = POINT; \
  MipFilter = NONE; \
}

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

float2 ViewportSize : VIEWPORTSIZE;
float4x4 Projection : PROJECTION;
float CameraFar : CAMERAFAR;

// Declare texture GBuffer0 and sampler GBuffer0Sampler.
DECLARE_UNIFORM_GBUFFER(GBuffer0, 0);

// Optional: Write color or texture.
float4 Color;
texture SourceTexture;
sampler SourceSampler : register(s1) = sampler_state
{
  Texture = <SourceTexture>;
};


//-----------------------------------------------------------------------------
// Structures
//-----------------------------------------------------------------------------

struct VSInput
{
  float4 Position : POSITION;
  float2 TexCoord : TEXCOORD;
};

struct VSOutput
{
  float2 TexCoord : TEXCOORD;
  float4 Position : SV_Position;
};


//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

/// Gets the linear depth in the range [0,1] from a G-buffer 0 sample.
/// \param[in] gBuffer0    The G-buffer 0 value.
/// \return The linear depth in the range [0, 1].
float GetGBufferDepth(float4 gBuffer0)
{
  return abs(gBuffer0.x);
}


/// Transforms a screen space position to standard projection space.
/// The half pixel offset for correct texture alignment is applied.
/// (Note: This half pixel offset is only necessary in DirectX 9.)
/// \param[in] position     The screen space position in "pixels".
/// \param[in] viewportSize The viewport size in pixels, e.g. (1280, 720).
/// \return The position in projection space.
float2 ScreenToProjection(float2 position, float2 viewportSize)
{
#if FIX_HALF_PIXEL
  // Subtract a half pixel so that the edge of the primitive is between screen pixels.
  // Thus, the first texel lies exactly on the first pixel.
  // See also http://drilian.com/2008/11/25/understanding-half-pixel-and-half-texel-offsets/
  // for a good description of this DirectX 9 problem.
  position -= 0.5;
#endif
  
  // Now transform screen space coordinate into projection space.
  // Screen space: Left top = (0, 0), right bottom = (ScreenSize.x - 1, ScreenSize.y - 1).
  // Projection space: Left top = (-1, 1), right bottom = (1, -1).
  
  // Transform into the range [0, 1] x [0, 1].
  position /= viewportSize;
  // Transform into the range [0, 2] x [-2, 0]
  position *= float2(2, -2);
  // Transform into the range [-1, 1] x [1, -1].
  position -= float2(1, -1);
  
  return position;
}


/// Transforms a screen space position to standard projection space.
/// The half pixel offset for correct texture alignment is applied.
/// (Note: This half pixel offset is only necessary in DirectX 9.)
/// \param[in] position     The screen space position in "pixels".
/// \param[in] viewportSize The viewport size in pixels, e.g. (1280, 720).
/// \return The position in projection space.
float4 ScreenToProjection(float4 position, float2 viewportSize)
{
  position.xy = ScreenToProjection(position.xy, viewportSize);
  return position;
}

VSOutput VS(VSInput input)
{
  VSOutput output = (VSOutput)0;
  output.Position = ScreenToProjection(input.Position, ViewportSize);
  output.TexCoord = input.TexCoord;
  return output;
}


void PS(float2 texCoord : TEXCOORD,
        out float4 color : COLOR,
        out float depth : DEPTH)
{
  // Write color or texture to render target.
  color = Color;
  
  float4 gBuffer0 = tex2D(GBuffer0Sampler, texCoord);
  float linearZ = GetGBufferDepth(gBuffer0) * CameraFar;
  
  // This is what we want to do:
  //float4 positionClip = mul(float4(0, 0, -linearZ, 1), Projection);
  //depth = positionClip.z / positionClip.w;
  
  // Optimized versions:
  // Perspective projection:
  // Since the Projection matrix has 0 elements, we only need z, and w is
  // equal to linearZ:
  depth = saturate((-linearZ * Projection._m22 + Projection._m32) / linearZ);
    
  // Effect compiler bug: Compiler removes saturate and pixel with a depth which is slightly above 1,
  // fail the depth test (even if DepthBufferFunction is Always)! --> We have to reformulate the equation.
  //depth = (-linearZ * Projection._m22 + Projection._m32) / linearZ;
  //depth = max(0, min(1, depth));
  // --> We can ignore this fix if we assume that the target z buffer is cleared to 1 anyway.
}