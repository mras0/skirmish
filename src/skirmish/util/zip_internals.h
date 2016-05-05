#ifndef SKIRMISH_UTIL_ZIP_INTERNALS_H
#define SKIRMISH_UTIL_ZIP_INTERNALS_H

#include "stream.h"

namespace skirmish { namespace zip {

// End of central directory record (EOCD)
struct end_of_central_directory_record {
    static constexpr uint32_t signature_magic = 0x06054b50;
    static constexpr uint32_t min_size_bytes  = 22;

    // Offset	Bytes	Description
    uint32_t signature;                             // 0	    4	    End of central directory signature = 0x06054b50
    uint16_t disk_number;                           // 4	    2	    Number of this disk
    uint16_t central_disk;                          // 6	    2	    Disk where central directory starts
    uint16_t central_directory_records_this_disk;   // 8	    2	    Number of central directory records on this disk
    uint16_t central_directory_records_this_total;  // 10	    2	    Total number of central directory records
    uint32_t central_directory_size_bytes;          // 12	    4	    Size of central directory (bytes)
    uint32_t central_directory_offset;              // 16	    4	    Offset of start of central directory, relative to start of archive
    uint16_t comment_length;                        // 20	    2	    Comment length (n)
                                                    // 22	    n	    Comment
};

void read(util::in_stream& in, end_of_central_directory_record& r);

enum class compression_methods : uint16_t { stored = 0, deflated = 8 };
std::ostream& operator<<(std::ostream& os, compression_methods cm);

// Central directory file header
struct central_directory_file_header {
    static constexpr uint32_t signature_magic = 0x02014b50;
    static constexpr uint32_t min_size_bytes  = 46;
    // Offset	Bytes	Description
    uint32_t signature;                     // 0	    4	    Central directory file header signature = 0x02014b50
    uint16_t version;                       // 4	    2	    Version made by
    uint16_t min_version;                   // 6	    2	    Version needed to extract (minimum)
    uint16_t flags;                         // 8	    2	    General purpose bit flag
    compression_methods compression_method; // 10	    2	    Compression method
    uint16_t last_modified_time;            // 12	    2	    File last modification time
    uint16_t last_modified_date;            // 14	    2	    File last modification date
    uint32_t crc32;                         // 16	    4	    CRC-32
    uint32_t compressed_size;               // 20	    4	    Compressed size
    uint32_t uncompressed_size;             // 24	    4	    Uncompressed size
    uint16_t filename_length;               // 28	    2	    File name length (n)
    uint16_t extra_field_length;            // 30	    2	    Extra field length (m)
    uint16_t file_comment_length;           // 32	    2	    File comment length (k)
    uint16_t disk;                          // 34	    2	    Disk number where file starts
    uint16_t internal_file_attributes;      // 36	    2	    Internal file attributes
    uint32_t external_file_attributes;      // 38	    4	    External file attributes
    uint32_t local_file_header_offset;      // 42	    4	    Relative offset of local file header. This is the number of bytes between the start of the first disk on which the file occurs, and the start of the local file header. This allows software reading the central directory to locate the position of the file inside the .ZIP file.
                                            // 46	    n	    File name
                                            // 46+n	    m	    Extra field
                                            // 46+n+m	k	    File comment
};

void read(util::in_stream& in, central_directory_file_header& r);

//Local file header
struct local_file_header {
    static constexpr uint32_t signature_magic = 0x04034b50;
    static constexpr uint32_t min_size_bytes  = 30;
    // Offset	Bytes	Description
    uint32_t signature;                     // 0	    4	    Local file header signature = 0x04034b50 (read as a little-endian number)
    uint16_t min_version;                   // 4	    2	    Version needed to extract (minimum)
    uint16_t flags;                         // 6	    2	    General purpose bit flag
    compression_methods compression_method; // 8	    2	    Compression method
    uint16_t last_modified_time;            // 10	    2	    File last modification time
    uint16_t last_modified_date;            // 12	    2	    File last modification date
    uint32_t crc32;                         // 14	    4	    CRC-32
    uint32_t compressed_size;               // 18	    4	    Compressed size
    uint32_t uncompressed_size;             // 22	    4	    Uncompressed size
    uint16_t filename_length;               // 26	    2	    File name length (n)
    uint16_t extra_field_length;            // 28	    2	    Extra field length (m)
                                            // 30	    n	    File name
                                            // 30+n	    m	    Extra field
};

void read(util::in_stream& in, local_file_header& r);

static constexpr auto invalid_file_pos = util::invalid_stream_size;

// TODO: This can be probably be done more cleverly
uint64_t find_end_of_central_directory_record(util::in_stream& in, end_of_central_directory_record& r);

} } // namespace skirmish::zip

#endif