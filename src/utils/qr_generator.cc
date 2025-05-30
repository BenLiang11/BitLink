#include "utils/qr_generator.h"
#include "qrencode.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring>

using namespace std;

// Base64 encoding table
static const string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

vector<uint8_t> QRCodeGenerator::generate_png(const string& data, 
                                             int size,
                                             ErrorCorrection error_correction) {
    if (data.empty()) {
        cerr << "QRCodeGenerator: Empty data provided" << endl;
        return {};
    }
    
    if (size <= 0) {
        cerr << "QRCodeGenerator: Invalid size: " << size << endl;
        return {};
    }
    
    // Convert error correction level
    QRecLevel qr_error_level;
    switch (error_correction) {
        case ErrorCorrection::LOW:
            qr_error_level = QR_ECLEVEL_L;
            break;
        case ErrorCorrection::MEDIUM:
            qr_error_level = QR_ECLEVEL_M;
            break;
        case ErrorCorrection::QUARTILE:
            qr_error_level = QR_ECLEVEL_Q;
            break;
        case ErrorCorrection::HIGH:
            qr_error_level = QR_ECLEVEL_H;
            break;
        default:
            qr_error_level = QR_ECLEVEL_M;
            break;
    }
    
    // Generate QR code
    QRcode* qr_code = QRcode_encodeString(data.c_str(), 0, qr_error_level, QR_MODE_8, 1);
    if (!qr_code) {
        cerr << "QRCodeGenerator: Failed to generate QR code" << endl;
        return {};
    }
    
    // Convert to PNG
    vector<uint8_t> png_data = qr_to_png(qr_code->data, qr_code->width, size);
    
    // Clean up
    QRcode_free(qr_code);
    
    return png_data;
}

bool QRCodeGenerator::save_qr_code(const string& data, 
                                  const string& file_path, 
                                  int size,
                                  ErrorCorrection error_correction) {
    if (file_path.empty()) {
        cerr << "QRCodeGenerator: Empty file path provided" << endl;
        return false;
    }
    
    vector<uint8_t> png_data = generate_png(data, size, error_correction);
    if (png_data.empty()) {
        return false;
    }
    
    // Write to file
    ofstream file(file_path, ios::binary);
    if (!file) {
        cerr << "QRCodeGenerator: Failed to open file for writing: " << file_path << endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(png_data.data()), png_data.size());
    if (!file) {
        cerr << "QRCodeGenerator: Failed to write PNG data to file" << endl;
        return false;
    }
    
    return true;
}

string QRCodeGenerator::generate_data_url(const string& data,
                                         int size,
                                         ErrorCorrection error_correction) {
    vector<uint8_t> png_data = generate_png(data, size, error_correction);
    if (png_data.empty()) {
        return "";
    }
    
    string base64_data = base64_encode(png_data);
    return "data:image/png;base64," + base64_data;
}

vector<uint8_t> QRCodeGenerator::qr_to_png(const uint8_t* qr_data, int qr_size, int image_size) {
    // Simple PNG implementation without external dependencies
    // Create a basic PNG structure manually
    
    if (!qr_data || qr_size <= 0 || image_size <= 0) {
        return {};
    }
    
    // Calculate scale factor
    int scale = max(1, image_size / qr_size);
    int actual_size = qr_size * scale;
    
    // Add border (4 modules on each side as per QR code spec)
    int border = scale * 4;
    int total_size = actual_size + (border * 2);
    
    // Create bitmap data (1 byte per pixel, 0 = black, 255 = white)
    vector<uint8_t> bitmap(total_size * total_size, 255); // Initialize to white
    
    // Fill QR code data
    for (int y = 0; y < qr_size; y++) {
        for (int x = 0; x < qr_size; x++) {
            uint8_t module = qr_data[y * qr_size + x];
            uint8_t color = (module & 1) ? 0 : 255; // Black if module is set, white otherwise
            
            // Scale up the module
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    int bitmap_x = border + (x * scale) + sx;
                    int bitmap_y = border + (y * scale) + sy;
                    bitmap[bitmap_y * total_size + bitmap_x] = color;
                }
            }
        }
    }
    
    // Convert to simple PNG format
    return create_simple_png(bitmap, total_size);
}

vector<uint8_t> QRCodeGenerator::create_simple_png(const vector<uint8_t>& bitmap, int size) {
    vector<uint8_t> png_data;
    
    // PNG signature
    png_data.insert(png_data.end(), {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A});
    
    // IHDR chunk
    vector<uint8_t> ihdr_data;
    write_uint32_be(ihdr_data, size);        // Width
    write_uint32_be(ihdr_data, size);        // Height
    ihdr_data.push_back(8);                  // Bit depth
    ihdr_data.push_back(0);                  // Color type (grayscale)
    ihdr_data.push_back(0);                  // Compression method
    ihdr_data.push_back(0);                  // Filter method
    ihdr_data.push_back(0);                  // Interlace method
    
    write_png_chunk(png_data, "IHDR", ihdr_data);
    
    // IDAT chunk (compressed image data)
    vector<uint8_t> raw_data;
    for (int y = 0; y < size; y++) {
        raw_data.push_back(0); // Filter type (None)
        for (int x = 0; x < size; x++) {
            raw_data.push_back(bitmap[y * size + x]);
        }
    }
    
    vector<uint8_t> compressed_data = simple_deflate(raw_data);
    write_png_chunk(png_data, "IDAT", compressed_data);
    
    // IEND chunk
    write_png_chunk(png_data, "IEND", {});
    
    return png_data;
}

void QRCodeGenerator::write_uint32_be(vector<uint8_t>& data, uint32_t value) {
    data.push_back((value >> 24) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back(value & 0xFF);
}

uint32_t QRCodeGenerator::crc32_table[256] = {0};
bool QRCodeGenerator::crc32_table_computed = false;

void QRCodeGenerator::make_crc_table() {
    if (crc32_table_computed) return;
    
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t c = n;
        for (int k = 0; k < 8; k++) {
            if (c & 1) {
                c = 0xedb88320L ^ (c >> 1);
            } else {
                c = c >> 1;
            }
        }
        crc32_table[n] = c;
    }
    crc32_table_computed = true;
}

uint32_t QRCodeGenerator::crc32(const vector<uint8_t>& data) {
    make_crc_table();
    uint32_t c = 0xffffffffL;
    
    for (uint8_t byte : data) {
        c = crc32_table[(c ^ byte) & 0xff] ^ (c >> 8);
    }
    
    return c ^ 0xffffffffL;
}

void QRCodeGenerator::write_png_chunk(vector<uint8_t>& png_data, 
                                     const string& type, 
                                     const vector<uint8_t>& data) {
    // Length
    write_uint32_be(png_data, data.size());
    
    // Type + Data
    vector<uint8_t> type_and_data;
    type_and_data.insert(type_and_data.end(), type.begin(), type.end());
    type_and_data.insert(type_and_data.end(), data.begin(), data.end());
    
    png_data.insert(png_data.end(), type_and_data.begin(), type_and_data.end());
    
    // CRC
    uint32_t crc = crc32(type_and_data);
    write_uint32_be(png_data, crc);
}

vector<uint8_t> QRCodeGenerator::simple_deflate(const vector<uint8_t>& data) {
    // Very simple deflate implementation (no compression, just store blocks)
    vector<uint8_t> result;
    
    // Zlib header (no compression)
    result.push_back(0x78); // CMF
    result.push_back(0x01); // FLG
    
    // Process data in blocks
    size_t pos = 0;
    while (pos < data.size()) {
        size_t block_size = min(size_t(65535), data.size() - pos);
        bool is_last = (pos + block_size >= data.size());
        
        // Block header
        result.push_back(is_last ? 0x01 : 0x00); // BFINAL and BTYPE
        
        // Block length (little-endian)
        result.push_back(block_size & 0xFF);
        result.push_back((block_size >> 8) & 0xFF);
        
        // One's complement of block length
        uint16_t nlen = ~block_size;
        result.push_back(nlen & 0xFF);
        result.push_back((nlen >> 8) & 0xFF);
        
        // Block data
        result.insert(result.end(), 
                     data.begin() + pos, 
                     data.begin() + pos + block_size);
        
        pos += block_size;
    }
    
    // Adler-32 checksum
    uint32_t adler = adler32(data);
    result.push_back((adler >> 24) & 0xFF);
    result.push_back((adler >> 16) & 0xFF);
    result.push_back((adler >> 8) & 0xFF);
    result.push_back(adler & 0xFF);
    
    return result;
}

uint32_t QRCodeGenerator::adler32(const vector<uint8_t>& data) {
    uint32_t a = 1, b = 0;
    const uint32_t MOD_ADLER = 65521;
    
    for (uint8_t byte : data) {
        a = (a + byte) % MOD_ADLER;
        b = (b + a) % MOD_ADLER;
    }
    
    return (b << 16) | a;
}

string QRCodeGenerator::base64_encode(const vector<uint8_t>& data) {
    string result;
    int val = 0, valb = -6;
    
    for (uint8_t c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (result.size() % 4) {
        result.push_back('=');
    }
    
    return result;
}   