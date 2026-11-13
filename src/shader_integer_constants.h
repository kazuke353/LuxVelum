#pragma once

#include <d3d9.h>

HRESULT CreatePixelShaderWithIntegerConstants(IDirect3DDevice9* dev, const DWORD* code, IDirect3DPixelShader9** out);
HRESULT CreatePixelShaderWithIntegerConstants(IDirect3DDevice9* dev, const BYTE* code, IDirect3DPixelShader9** out);
void SetPixelShaderWithIntegerConstants(IDirect3DDevice9* dev, IDirect3DPixelShader9* shader);
void ClearPixelShaderIntegerConstants(IDirect3DDevice9* dev);
