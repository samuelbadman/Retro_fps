#include "Pch.h"
#include "BinaryBuffer.h"

const uint8_t* BinaryBuffer::GetBufferPointer() const
{
    return Binary.data();
}

uint8_t* BinaryBuffer::GetBufferPointer()
{
    return Binary.data();
}

size_t BinaryBuffer::GetBufferLength() const
{
    return Binary.size();
}

void BinaryBuffer::Resize(size_t newSize)
{
    Binary.resize(newSize);
}