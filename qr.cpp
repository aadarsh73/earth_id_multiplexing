#include <iostream>
#include <fstream>
#include <cstring>
#include <qrencode.h>
#include <png.h>
#include <vector>

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

void generateQRCodePNG(const char* final_data[], const char* filename) {
   std::vector<std::vector<int>> qr_module;
   for (int i = 0; i < 3; ++i) {
        std::vector<int> module_values;
       QRcode* qr = QRcode_encodeString(final_data[i], 0, QR_ECLEVEL_L, QR_MODE_8, 1);

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
        qr_module.push_back(module_values);
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
