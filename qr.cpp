//
// Created by Aadarsh on 11/02/24.
//

#include <iostream>
#include <fstream>
#include <cstring>
#include <qrencode.h>
//#include <png.h>
#include "/opt/homebrew/Cellar/libpng/1.6.42/include/png.h"
#include <vector>
#include <set>
#include <map>
#include <sstream>


// Define a simple JSON parser function
#include <sstream>
#include <zlib.h>

std::string serializeIntegerVector(const std::vector<int>& vec) {
    std::string serialized;
    for (int num : vec) {
        serialized += std::to_string(num) + ",";
    }
    // Remove the last comma
    if (!serialized.empty()) {
        serialized.pop_back();
    }
    return serialized;
}

std::vector<int> deserializeIntegerVector(const std::string& serialized) {
    std::vector<int> vec;
    std::string numStr;
    for (char ch : serialized) {
        if (ch == ',') {
            vec.push_back(std::stoi(numStr));
            numStr.clear();
        } else {
            numStr += ch;
        }
    }
    // Add the last number
    if (!numStr.empty()) {
        vec.push_back(std::stoi(numStr));
    }
    return vec;
}

std::string compressString(const std::string& input) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (deflateInit(&zs, Z_BEST_COMPRESSION) != Z_OK) {
        return "";
    }

    zs.next_in = (Bytef*)input.data();
    zs.avail_in = input.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // Retrieve the compressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            // Append the block to the output string
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        // An error occurred; return an empty string
        return "";
    }

    return outstring;
}

std::string decompressString(const std::string& input) {
    z_stream zs;
    memset(&zs, 0, sizeof(zs));

    if (inflateInit(&zs) != Z_OK) {
        return "";
    }

    zs.next_in = (Bytef*)input.data();
    zs.avail_in = input.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // Retrieve the decompressed bytes blockwise
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = inflate(&zs, 0);

        if (outstring.size() < zs.total_out) {
            // Append the block to the output string
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {
        // An error occurred; return an empty string
        return "";
    }

    return outstring;
}

void printMatrix(const std::vector<std::vector<int> >& matrix) {
    for (const auto& row : matrix) {
        for (int value : row) {
            std::cout << value << " ";
        }
        std::cout << std::endl;
    }
}

std::string convertToBase36(const std::string& input) {
    static const std::string charset = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string result;

    // Convert each character in the input string to base36 and concatenate
    for (char ch : input) {
        std::string base36;
        int num = static_cast<int>(ch);
        do {
            base36.push_back(charset[num % 36]);
            num /= 36;
        } while (num > 0);

        // Reverse the string and append to the result
        std::reverse(base36.begin(), base36.end());
        result += base36;
    }

    return result;
}

std::string base16ToString(int num) {
    std::stringstream ss;
    ss << std::hex << num;
    return ss.str();
}

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
    std::vector<std::vector<int> > qr_modules;
    std::map<std::string, std::string> sequence_colors;
    std::vector<std::string> sequences;

    // read individual json files and generate qr codes
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
                row_pointers[y][3 * x] = qr->data[offset] & 1 ? 0 : 255;
                row_pointers[y][3 * x + 1] = qr->data[offset] & 1 ? 0 : 255;
                row_pointers[y][3 * x + 2] = qr->data[offset] & 1 ? 0 : 255;
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
    // print qr modules:
    std::set<std::string> unique_sequences;

    std::map<std::string, int> sequence_numbers;
    std::cout << "QR modules : " << std::endl;

    // iterate over qr modules and generate sequences

    for (const auto& qr_module : qr_modules) {
        for (int i = 0; i < qr_module.size(); ++i) {
            std::stringstream sequence;
            for (int j = 0; j < qr_modules.size(); ++j) {
                sequence << qr_modules[j][i];
            }
            unique_sequences.insert(sequence.str());
            sequences.push_back(sequence.str());
        }
    }

    // compare sequence values from index 0 - 440 and index 441 - 881
    bool same = true;
    int last;
    for (int i = 0; i < 441; ++i) {
        std::cout << sequences[i] << " " << sequences[i+441] << std::endl;
        last = i;
        if (sequences[i] != sequences[i+2*441]) {
            same = false;
            break;
        }
    }
    std::cout << "Same: " << same <<"\t"<< last << std::endl;

    // generate a new array which contains only the 1st 441 elements to sequence
    std::vector<std::string> actual_sequences;
    for (int i = 0; i < 441; ++i) {
        actual_sequences.push_back(sequences[i]);
    }

    std::vector<int> decimal_values;

    // Iterate over each binary string in the set
    for (const std::string& binary : actual_sequences) {
        // Convert binary string to decimal and store in the vector
        int decimal = std::stoi(binary, 0, 2); // base 2 conversion
        decimal_values.push_back(decimal);
    }

    // Print the decimal values
    std::cout << "Decimal values:" << std::endl;
    for (int decimal : decimal_values) {
        std::cout << "decimal" << decimal << std::endl;
    }

    std::cout<< "Decimal values size: " << decimal_values.size() << std::endl;
    std::cout << std::endl << "Generated QR matrix : " << std::endl << std::endl;
    for (int i=0; i<441; i++){
        int n = decimal_values[i];
        if (i%21==0) {
            std::cout << std::endl;
        }
        else if (n>9) {
            std::cout << n << " " ;
        }
        else {
            std::cout <<0<< n << " ";
        }
        }

    std::cout << std::endl << std::endl << std::endl;

    // Serialize the integer vector

    std::string serialized = serializeIntegerVector(decimal_values);

    // Compress the serialized string
    std::string compressed = compressString(serialized);

    std::cout << "Original String: " << serialized << std::endl;
    std::cout << "Compressed String: " << compressed << std::endl;

    // Decompress the compressed string
    std::string decompressed = decompressString(compressed);

    // Deserialize the string back into an integer vector
    std::vector<int> restored = deserializeIntegerVector(decompressed);

    // Output the restored integer vector
    std::cout << "Restored Integer Vector: ";
    for (int num : restored) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    std::string separator = ","; // Separator between values

    // Convert decimal values to string with separator
    std::string data;
    for (size_t i = 0; i < decimal_values.size(); ++i) {
        data += std::to_string(decimal_values[i]);
        if (i != decimal_values.size() - 1) {
            data += separator;
        }
    }


    std::cout << "Data: " << data << std::endl;
    std::cout << "Data size: " << data.size() << std::endl;

//    std::string base36 = convertToBase36(data);
//
//    // Output the base36 number
//    std::cout << "Base36 number: " << base36 << std::endl;


    QRcode* qr_final = QRcode_encodeString(data.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
    std::cout << "Number of modules in QR code " << ": " << qr_final->width << std::endl;
    // Create a PNG file
    std::stringstream pngFilename;
    pngFilename << "final" <<".png";
    FILE* pngFile = fopen(pngFilename.str().c_str(), "wb");
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info = png_create_info_struct(png);

    // Set up error handling
    if (setjmp(png_jmpbuf(png))) {
        fclose(pngFile);
        png_destroy_write_struct(&png, &info);
        QRcode_free(qr_final);
        std::cerr << "Error writing PNG file." << std::endl;
        return;
    }
    png_init_io(png, pngFile);

    // Set image properties
    png_set_IHDR(png, info, qr_final->width, qr_final->width, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Write header information
    png_write_info(png, info);
    // Allocate memory for image data
    png_bytep row_pointers[qr_final->width];
    for (int y = 0; y < qr_final->width; ++y) {
        row_pointers[y] = new png_byte[3 * qr_final->width];
        for (int x = 0; x < qr_final->width; ++x) {
            int offset = y * qr_final->width + x;
            row_pointers[y][3 * x] = qr_final->data[offset] & 1 ? 0 : 255;
            row_pointers[y][3 * x + 1] = qr_final->data[offset] & 1 ? 0 : 255;
            row_pointers[y][3 * x + 2] = qr_final->data[offset] & 1 ? 0 : 255;
        }
        png_write_row(png, row_pointers[y]);
    }
    // Finalize writing
    png_write_end(png, nullptr);

    // Clean up
    for (int y = 0; y < qr_final->width; ++y) {
        delete[] row_pointers[y];
    }
    fclose(pngFile);
    png_destroy_write_struct(&png, &info);
    QRcode_free(qr_final);

    std::cout << "QR code " << " saved as " << pngFilename.str() << std::endl;


}

int main() {
    const char* jsonFilename = "input.json";
    const char* jsonFilename1 = "input1.json";
    const char* jsonFilename2 = "input2.json";
    const char* jsonFilename3 = "input3.json";
    std::string jsonData, jsonData1, jsonData2, jsonData3;

    // Read JSON data from file
    if (!(parseJsonFile(jsonFilename, jsonData) && parseJsonFile(jsonFilename1, jsonData1) && (parseJsonFile(jsonFilename2, jsonData2)) && (parseJsonFile(jsonFilename3, jsonData3))))
    {
        return 1;
    }
    const char* data = jsonData.c_str();
    const char* data1 = jsonData1.c_str();
    const char* data2 = jsonData2.c_str();
    const char* data3 = jsonData3.c_str();
    const char* filename = "qrcode.png";

    const char* final_data[] = {data, data1, data2, data3};

    // Generate and save the QR code as a PNG file
    generateQRCodePNG(final_data, filename);

    return 0;
}
