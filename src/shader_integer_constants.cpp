#include "shader_integer_constants.h"

#include <mutex>
#include <unordered_map>
#include <vector>

namespace
{
constexpr DWORD kEndToken = 0x0000FFFF;
constexpr DWORD kOpcodeMask = 0x0000FFFF;
constexpr DWORD kCommentOpcode = 0xFFFE;
constexpr DWORD kDefineIntegerOpcode = 0x30;
constexpr int kCommentLengthShift = 16;
constexpr DWORD kCommentLengthMask = 0x7FFF;
constexpr int kInstructionLengthShift = 24;
constexpr DWORD kInstructionLengthMask = 0xF;
constexpr DWORD kRegisterNumberMask = 0x7FF;
constexpr UINT kIntegerRegisters = 16;
constexpr size_t kMaxShaderTokens = 1u << 20;

struct IntegerConstant
{
    UINT reg;
    int values[4];
};

std::mutex g_lock;
std::unordered_map<IDirect3DPixelShader9*, std::vector<IntegerConstant>> g_constants;

std::vector<IntegerConstant> DefinedIntegerConstants(const DWORD* code)
{
    std::vector<IntegerConstant> found;
    for (size_t i = 1; i < kMaxShaderTokens && code[i] != kEndToken;)
    {
        const DWORD token = code[i];
        const DWORD opcode = token & kOpcodeMask;
        if (opcode == kCommentOpcode)
        {
            i += 1 + ((token >> kCommentLengthShift) & kCommentLengthMask);
            continue;
        }
        const DWORD length = (token >> kInstructionLengthShift) & kInstructionLengthMask;
        if (opcode == kDefineIntegerOpcode && length >= 5)
        {
            IntegerConstant constant = {code[i + 1] & kRegisterNumberMask, {}};
            for (int c = 0; c < 4; ++c)
                constant.values[c] = static_cast<int>(code[i + 2 + c]);
            if (constant.reg < kIntegerRegisters)
                found.push_back(constant);
        }
        i += 1 + length;
    }
    return found;
}
}

HRESULT CreatePixelShaderWithIntegerConstants(IDirect3DDevice9* dev, const DWORD* code, IDirect3DPixelShader9** out)
{
    const HRESULT result = dev->CreatePixelShader(code, out);
    if (SUCCEEDED(result) && *out)
    {
        std::vector<IntegerConstant> constants = DefinedIntegerConstants(code);
        const std::lock_guard<std::mutex> guard(g_lock);
        if (constants.empty())
            g_constants.erase(*out);
        else
            g_constants[*out] = std::move(constants);
    }
    return result;
}

HRESULT CreatePixelShaderWithIntegerConstants(IDirect3DDevice9* dev, const BYTE* code, IDirect3DPixelShader9** out)
{
    return CreatePixelShaderWithIntegerConstants(dev, reinterpret_cast<const DWORD*>(code), out);
}

void SetPixelShaderWithIntegerConstants(IDirect3DDevice9* dev, IDirect3DPixelShader9* shader)
{
    dev->SetPixelShader(shader);
    if (!shader)
        return;
    const std::lock_guard<std::mutex> guard(g_lock);
    const auto found = g_constants.find(shader);
    if (found == g_constants.end())
        return;
    for (const IntegerConstant& constant : found->second)
        dev->SetPixelShaderConstantI(constant.reg, constant.values, 1);
}

void ClearPixelShaderIntegerConstants(IDirect3DDevice9* dev)
{
    const int zeros[kIntegerRegisters * 4] = {};
    dev->SetPixelShaderConstantI(0, zeros, kIntegerRegisters);
}
