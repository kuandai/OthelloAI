#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <map>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: embed_kernels <kernels_dir> <output.hpp>\n";
        return 1;
    }

    std::string dir = argv[1];
    std::string outpath = argv[2];
    std::ofstream out(outpath);
    out << "#pragma once\n\n";
    out << "#include <string>\n";
    out << "#include <unordered_map>\n\n";
    out << "inline std::unordered_map<std::string, std::string> kernel_sources = {\n";

    for (const auto& file : std::filesystem::directory_iterator(dir)) {
        if (file.path().extension() != ".cl") continue;

        std::ifstream in(file.path());
        std::ostringstream content;
        content << in.rdbuf();

        out << "  {\"" << file.path().filename().string() << "\", R\"(" << content.str() << ")\"},\n";
    }

    out << "};\n";
    return 0;
}
