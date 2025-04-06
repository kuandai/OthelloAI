// tools/embed_model.cpp
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: embed_model <input.bin> <output.hpp>\n";
        return 1;
    }

    std::ifstream in(argv[1], std::ios::binary);
    std::ofstream out(argv[2]);

    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(in), {});

    out << "#pragma once\n";
    out << "inline unsigned char model_data[] = {";

    for (size_t i = 0; i < buffer.size(); ++i) {
        if (i % 12 == 0) out << "\n    ";
        out << "0x" << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(buffer[i]) << ", ";
    }

    out << "\n};\n";
    out << "inline size_t model_size = sizeof(model_data);\n";

    return 0;
}

