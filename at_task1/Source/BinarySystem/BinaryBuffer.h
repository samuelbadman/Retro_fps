#pragma once

class BinaryBuffer
{
public:
	const uint8_t* GetBufferPointer() const;
	uint8_t* GetBufferPointer();
	size_t GetBufferLength() const;
	void Resize(size_t newSize);

private:
	std::vector<uint8_t> Binary;
};