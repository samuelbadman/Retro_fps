#pragma once

#include "BinaryBuffer.h"

namespace Binary
{
	bool ReadBinaryIntoBuffer(const std::string& filepath, BinaryBuffer& buffer);
}