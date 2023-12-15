#include <iostream>
#include <fstream>
#include <cstring>
#include <qrencode.h>
#include <png.h>
#include <vector>
#include <set>
#include <map>
#include <sstream>

// Define a simple JSON parser function
#include <sstream>

bool parseJsonFile(const char* filename, std::string& jsonData) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening JSON file: " << filename << std::endl;
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    jsonData = buffer.str();

    return true;
}

std::set<std::string> used_colors;

std::string getRandomColor() {
   std::string color;
   while (true) {
       color = std::to_string(rand() % 256) + "," +
               std::to_string(rand() % 256) + "," +
               std::to_string(rand() % 256);
       if (used_colors.find(color) == used_colors.end()) {
           used_colors.insert(color);
           break;
       }
   }
   return color;
}

void generateQRCodePNG(const char* final_data[], const char* filename) {
   std::vector<std::vector<int>> qr_modules;
   for (int i = 0; i < 3; ++i) {
        std::vector<int> module_values;
       QRcode* qr = QRcode_encodeString(final_data[i], 0, QR_ECLEVEL_L, QR_MODE_8, 1);
        std::cout << "Number of modules in QR code " << i+1 << ": " << qr->width << std::endl;
       // Create a PNG file
       std::stringstream pngFilename;
       pngFilename << filename << "_" << i << ".png";
       FILE* pngFile = fopen(pngFilename.str().c_str(), "wb");
       png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
       png_infop info = png_create_info_struct(png);

       // Set up error handling
       if (setjmp(png_jmpbuf(png))) {
           fclose(pngFile);
           png_destroy_write_struct(&png, &info);
           QRcode_free(qr);
           std::cerr << "Error writing PNG file." << std::endl;
           return;
       }

       png_init_io(png, pngFile);

       // Set image properties
       png_set_IHDR(png, info, qr->width, qr->width, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                   PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

       // Write header information
       png_write_info(png, info);
       // Allocate memory for image data
       png_bytep row_pointers[qr->width];
       for (int y = 0; y < qr->width; ++y) {
           row_pointers[y] = new png_byte[3 * qr->width];
           for (int x = 0; x < qr->width; ++x) {
               int offset = y * qr->width + x;
               module_values.push_back(qr->data[offset] & 1? 1:0);
               row_pointers[y][3 * x] = qr->data[offset] & 1 ? 100 : 255;
               row_pointers[y][3 * x + 1] = qr->data[offset] & 1 ? 100 : 255;
               row_pointers[y][3 * x + 2] = qr->data[offset] & 1 ? 100 : 255;
           }
           png_write_row(png, row_pointers[y]);
       }
        qr_modules.push_back(module_values);
       // Finalize writing
       png_write_end(png, nullptr);

       // Clean up
       for (int y = 0; y < qr->width; ++y) {
           delete[] row_pointers[y];
       }
       fclose(pngFile);
       png_destroy_write_struct(&png, &info);
       QRcode_free(qr);

       std::cout << "QR code " << i+1 << " saved as " << pngFilename.str() << std::endl;
   }
    std::set<std::string> unique_sequences;
    std::map<std::string, int> sequence_numbers;
    std::map<std::string, std::string> sequence_colors;

    for (const auto& qr_module : qr_modules) {
    for (int i = 0; i < qr_module.size(); ++i) {
        std::stringstream sequence;
        for (int j = 0; j < qr_modules.size(); ++j) {
            sequence << qr_modules[j][i];
        }
        unique_sequences.insert(sequence.str());
    }
    }

    int number = 0;
    for (const auto& sequence : unique_sequences) {
    sequence_numbers[sequence] = number++;
    std::string color = getRandomColor();
    sequence_colors[sequence] = color;
    }
    for (const auto& pair : sequence_numbers) {
    std::cout << "Sequence: " << pair.first << ", Number: " << pair.second << std::endl;
    }
    std::vector<std::vector<std::string>> new_qr_module;

    for (int i = 0; i < qr_modules[0].size(); ++i) {
    std::string sequence;
    for (const auto& qr_module : qr_modules) {
        sequence += std::to_string(qr_module[i]);
    }
    std::vector<std::string> color_sequence;
    for (int j = 0; j < qr_modules.size(); ++j) {
        color_sequence.push_back(sequence_colors[sequence]);
    }
    new_qr_module.push_back(color_sequence);
    }

    std::string new_qr_string;
for (const auto& color_sequence : new_qr_module) {
 for (const auto& color : color_sequence) {
   new_qr_string += color;
 }
}

}

int main() {
    const char* jsonFilename = "input.json";
    const char* jsonFilename1 = "input1.json";
    const char* jsonFilename2 = "input2.json";
    std::string jsonData, jsonData1, jsonData2;

    // Read JSON data from file
    if (!(parseJsonFile(jsonFilename, jsonData) && parseJsonFile(jsonFilename1, jsonData1) && (parseJsonFile(jsonFilename2, jsonData2)))) 
    {
        return 1;
    }
    const char* data = jsonData.c_str();
    const char* data1 = jsonData1.c_str();
    const char* data2 = jsonData2.c_str();
    const char* filename = "qrcode.png";

    const char* final_data[] = {data, data1, data2};

    // Generate and save the QR code as a PNG file
    generateQRCodePNG(final_data, filename);

    return 0;
}
