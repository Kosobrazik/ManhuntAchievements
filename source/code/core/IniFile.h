#pragma once
#include <filesystem>
#include <fstream>
#include <vector>

// Windows' INI reader does not skip a UTF-8 byte order mark, so a file saved
// with one loses its first section: every setting quietly falls back to its
// default, logging included, and nothing says why. Editors and shells add the
// mark on their own, so take it back off rather than let that happen unnoticed.
inline void RemoveIniByteOrderMark(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return;
    char mark[3] = {};
    if (!file.read(mark, sizeof(mark)) ||
        static_cast<unsigned char>(mark[0]) != 0xEF ||
        static_cast<unsigned char>(mark[1]) != 0xBB ||
        static_cast<unsigned char>(mark[2]) != 0xBF)
        return;
    const std::vector<char> rest((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    file.close();
    std::ofstream rewritten(path, std::ios::binary | std::ios::trunc);
    if (rewritten && !rest.empty())
        rewritten.write(rest.data(), static_cast<std::streamsize>(rest.size()));
}
