#include "Pch.h"
#include "Binary.h"
#include "BinaryBuffer.h"

static uintmax_t FileSize(const std::string& filepath)
{
    std::filesystem::path path(filepath);

    if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
    {
        return std::filesystem::file_size(path);
    }

    return 0;
}

bool Binary::ReadBinaryIntoBuffer(const std::string& filepath, BinaryBuffer& buffer)
{
    std::ifstream fs;
    fs.open(filepath.c_str(), std::ifstream::in | std::ifstream::binary);

    if (!fs.good())
    {
        return false;
    }

    auto size = FileSize(filepath);
    buffer.Resize(static_cast<size_t>(size));
    fs.seekg(0, std::ios::beg);
    fs.read(reinterpret_cast<char*>(buffer.GetBufferPointer()), size);
    fs.close();

    return true;
}
