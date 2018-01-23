// Copyright (C) 2018 Elviss Strazdins
// This file is part of the Ouzel engine.

cbuffer Constants: register(b0)
{
    float4x4 modelViewProj;
}

struct VSInput
{
    float3 position: POSITION;
    float4 color: COLOR;
};

struct VS2PS
{
    float4 position: SV_POSITION;
    float4 color: COLOR;
};

void main(in VSInput input, out VS2PS output)
{
    output.position = mul(modelViewProj, float4(input.position, 1));
    output.color = input.color;
}
