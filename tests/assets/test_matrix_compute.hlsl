cbuffer cbConstants : register(b0)
{
    float fMatrixSize;
}

StructuredBuffer<float> matrix1: register(t0);
StructuredBuffer<float> matrix2: register(t1);
RWStructuredBuffer<float> matrixOut: register(u0);

[numthreads(32, 32, 1)]
void matrixCalc(int3 iDispatchThreadID : SV_DispatchThreadID)
{
    int iMatrixSize = fMatrixSize;

    if (iDispatchThreadID.x >= iMatrixSize || iDispatchThreadID.y >= iMatrixSize)
    {
        // a few threads will be out of bounds
        return;
    }

    float fResult = 0.0f;

    for (int i = 0; i < iMatrixSize; i++)
    {
        fResult += matrix1[iDispatchThreadID.x * iMatrixSize + i] * matrix2[iMatrixSize * i + iDispatchThreadID.y];
    }

    matrixOut[iDispatchThreadID.x * iMatrixSize + iDispatchThreadID.y] = fResult;
}