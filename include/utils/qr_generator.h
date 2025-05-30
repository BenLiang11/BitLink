#ifndef QR_GENERATOR_H
#define QR_GENERATOR_H

#include <vector>
#include <string>
#include <cstdint>

/**
 * @brief QR code generation utilities
 * 
 * Wrapper around libqrencode for generating QR codes
 * as PNG images with customizable size and error correction.
 */
class QRCodeGenerator {
public:
    /**
     * @brief Error correction levels for QR codes
     */
    enum class ErrorCorrection {
        LOW = 0,     // ~7% error correction
        MEDIUM = 1,  // ~15% error correction  
        QUARTILE = 2, // ~25% error correction
        HIGH = 3     // ~30% error correction
    };
    
    /**
     * @brief Generate QR code as PNG image data
     * @param data Text data to encode
     * @param size Image size in pixels (default: 200)
     * @param error_correction Error correction level
     * @return PNG image data as byte vector
     */
    static std::vector<uint8_t> generate_png(const std::string& data, 
                                            int size = 200,
                                            ErrorCorrection error_correction = ErrorCorrection::MEDIUM);
    
    /**
     * @brief Save QR code directly to file
     * @param data Text data to encode
     * @param file_path Output file path
     * @param size Image size in pixels
     * @param error_correction Error correction level
     * @return true if successful, false otherwise
     */
    static bool save_qr_code(const std::string& data, 
                            const std::string& file_path, 
                            int size = 200,
                            ErrorCorrection error_correction = ErrorCorrection::MEDIUM);
    
    /**
     * @brief Generate QR code as base64-encoded data URL
     * @param data Text data to encode
     * @param size Image size in pixels
     * @param error_correction Error correction level
     * @return base64 data URL string
     */
    static std::string generate_data_url(const std::string& data,
                                        int size = 200,
                                        ErrorCorrection error_correction = ErrorCorrection::MEDIUM);

private:
    /**
     * @brief Convert QR code bitmap to PNG format
     * @param qr_data Raw QR code data
     * @param qr_size QR code dimensions (width/height)
     * @param image_size Desired output image size in pixels
     * @return PNG image data as byte vector
     */
    static std::vector<uint8_t> qr_to_png(const uint8_t* qr_data, int qr_size, int image_size);
    
    /**
     * @brief Create PNG from bitmap data
     * @param bitmap Grayscale bitmap data
     * @param size Image dimensions (width/height)
     * @return PNG image data as byte vector
     */
    static std::vector<uint8_t> create_simple_png(const std::vector<uint8_t>& bitmap, int size);
    
    /**
     * @brief Write 32-bit unsigned integer in big-endian format
     * @param data Output vector to append to
     * @param value Value to write
     */
    static void write_uint32_be(std::vector<uint8_t>& data, uint32_t value);
    
    /**
     * @brief Calculate CRC32 checksum
     * @param data Input data
     * @return CRC32 checksum
     */
    static uint32_t crc32(const std::vector<uint8_t>& data);
    
    /**
     * @brief Initialize CRC32 lookup table
     */
    static void make_crc_table();
    
    /**
     * @brief Write PNG chunk with length, type, data, and CRC
     * @param png_data Output PNG data vector
     * @param type Chunk type (4 characters)
     * @param data Chunk data
     */
    static void write_png_chunk(std::vector<uint8_t>& png_data, 
                               const std::string& type, 
                               const std::vector<uint8_t>& data);
    
    /**
     * @brief Simple deflate compression for PNG
     * @param data Input data to compress
     * @return Compressed data
     */
    static std::vector<uint8_t> simple_deflate(const std::vector<uint8_t>& data);
    
    /**
     * @brief Calculate Adler-32 checksum
     * @param data Input data
     * @return Adler-32 checksum
     */
    static uint32_t adler32(const std::vector<uint8_t>& data);
    
    /**
     * @brief Encode binary data to base64 string
     * @param data Binary data to encode
     * @return Base64 encoded string
     */
    static std::string base64_encode(const std::vector<uint8_t>& data);
    
    // CRC32 lookup table and initialization flag
    static uint32_t crc32_table[256];
    static bool crc32_table_computed;
};

#endif // QR_GENERATOR_H