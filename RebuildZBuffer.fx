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

//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Optional: Write color or texture.
float4 Color;

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

VSOutput VS(VSInput input)
{
  VSOutput output = (VSOutput)0;
    
  output.Position = input.Position;
  output.TexCoord = input.TexCoord;
  return output;
}


float4 PS(float2 texCoord : TEXCOORD) : COLOR0
{
  // Write color or texture to render target.
  return Color;
}
