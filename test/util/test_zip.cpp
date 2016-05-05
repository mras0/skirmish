#include <skirmish/util/stream.h>
#include "catch.hpp"
#include <vector>
#include <iostream>
#include <cassert>

namespace skirmish { namespace zip {

void read(util::in_stream& in, uint16_t& value)
{
    value = in.get_u16_le();
}

void read(util::in_stream& in, uint32_t& value)
{
    value = in.get_u32_le();
}

template<typename E>
void read(util::in_stream& in, E& value)
{
    std::underlying_type_t<E> repr;
    read(in, repr);
    value = static_cast<E>(repr);
}

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
    uint32_t central_directory_central_offset;      // 16	    4	    Offset of start of central directory, relative to start of archive
    uint16_t comment_length;                        // 20	    2	    Comment length (n)
                                                    // 22	    n	    Comment
};

void read(util::in_stream& in, end_of_central_directory_record& r)
{
    read(in, r.signature);
    read(in, r.disk_number);
    read(in, r.central_disk);
    read(in, r.central_directory_records_this_disk);
    read(in, r.central_directory_records_this_total);
    read(in, r.central_directory_size_bytes);
    read(in, r.central_directory_central_offset);
    read(in, r.comment_length);
}

enum class compression_methods : uint16_t { stored = 0, deflated = 8 };
std::ostream& operator<<(std::ostream& os, compression_methods cm)
{
    switch (cm) {
    case compression_methods::stored: return os << "stored";
    case compression_methods::deflated: return os << "deflated";
    }
    return os << "compression_methods{" << static_cast<int>(cm) << "}";
}

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

void read(util::in_stream& in, central_directory_file_header& r)
{
    read(in, r.signature);
    read(in, r.version);
    read(in, r.min_version);
    read(in, r.flags);
    read(in, r.compression_method);
    read(in, r.last_modified_time);
    read(in, r.last_modified_date);
    read(in, r.crc32);
    read(in, r.compressed_size);
    read(in, r.uncompressed_size);
    read(in, r.filename_length);
    read(in, r.extra_field_length);
    read(in, r.file_comment_length);
    read(in, r.disk);
    read(in, r.internal_file_attributes);
    read(in, r.external_file_attributes);
    read(in, r.local_file_header_offset);
}

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

void read(util::in_stream& in, local_file_header& r)
{
    read(in, r.signature);
    read(in, r.min_version);
    read(in, r.flags);
    read(in, r.compression_method);
    read(in, r.last_modified_time);
    read(in, r.last_modified_date);
    read(in, r.crc32);
    read(in, r.compressed_size);
    read(in, r.uncompressed_size);
    read(in, r.filename_length);
    read(in, r.extra_field_length);
}

static constexpr auto invalid_file_pos = util::invalid_stream_size;

// TODO: This can be probably be done more cleverly
uint64_t find_end_of_central_directory_record(util::in_stream& in, end_of_central_directory_record& r)
{
    const auto file_size = in.stream_size();
    if (file_size < end_of_central_directory_record::min_size_bytes) {
        return invalid_file_pos;
    }

    for (uint64_t pos = file_size - end_of_central_directory_record::min_size_bytes + 1; !in.error() && pos--;) {
        in.seek(pos, util::seekdir::beg);
        read(in, r);
        assert(in.tell() == pos + end_of_central_directory_record::min_size_bytes);
        if (r.signature == end_of_central_directory_record::signature_magic &&
            pos + r.comment_length + end_of_central_directory_record::min_size_bytes == file_size) {
            return pos;
        }
    }

    return invalid_file_pos;
}

} } // namespace skirmish::zip

using namespace skirmish::zip;
using namespace skirmish::util;

TEST_CASE("empty.zip") {
    in_file_stream empty_zip{(std::string{TEST_DATA_DIR} + "/" + "empty.zip").c_str()};
    REQUIRE(empty_zip.stream_size() == end_of_central_directory_record::min_size_bytes);

    end_of_central_directory_record r;
    REQUIRE(find_end_of_central_directory_record(empty_zip, r) == 0);

    REQUIRE(r.signature == end_of_central_directory_record::signature_magic);
    REQUIRE(r.disk_number == 0);
    REQUIRE(r.central_disk == 0);
    REQUIRE(r.central_directory_records_this_disk == 0);
    REQUIRE(r.central_directory_records_this_total == 0);
    REQUIRE(r.central_directory_size_bytes == 0);
    REQUIRE(r.central_directory_central_offset == 0);
    REQUIRE(r.comment_length == 0);
}

TEST_CASE("test.zip") {
    in_file_stream test_zip{(std::string{TEST_DATA_DIR} + "/" + "test.zip").c_str()};

    // End of central directory record
    end_of_central_directory_record r;
    const uint64_t expected_end_of_central_directory_record_pos = 105;
    const auto end_of_central_directory_record_pos = find_end_of_central_directory_record(test_zip, r);
    REQUIRE(end_of_central_directory_record_pos == expected_end_of_central_directory_record_pos);
    REQUIRE(r.disk_number == 0);
    REQUIRE(r.central_disk == 0);
    REQUIRE(r.central_directory_records_this_disk == 1);
    REQUIRE(r.central_directory_records_this_total == 1);
    REQUIRE(r.central_directory_size_bytes == 54);
    REQUIRE(r.central_directory_central_offset == 51);

    const std::string expected_comment = "This is a comment!";
    REQUIRE(r.comment_length == expected_comment.length());
    REQUIRE(test_zip.tell() == expected_end_of_central_directory_record_pos+22);
    std::string comment(r.comment_length, '\0');
    test_zip.read(&comment[0], r.comment_length);
    REQUIRE(test_zip.error() == std::error_code());
    REQUIRE(comment == expected_comment);

    // Central directory file header
    central_directory_file_header cdfh;
    test_zip.seek(r.central_directory_central_offset, seekdir::beg);
    read(test_zip, cdfh);

    REQUIRE(cdfh.signature == central_directory_file_header::signature_magic);
    REQUIRE(cdfh.version == 20);
    REQUIRE(cdfh.min_version == 20);
    REQUIRE(cdfh.flags == 2);
    REQUIRE(cdfh.compression_method == compression_methods::deflated);
    REQUIRE(cdfh.last_modified_time == 25120);
    REQUIRE(cdfh.last_modified_date == 18597);
    REQUIRE(cdfh.crc32 == 0x87E4F545);
    REQUIRE(cdfh.compressed_size == 13);
    REQUIRE(cdfh.uncompressed_size == 14);
    REQUIRE(cdfh.filename_length == 8);
    REQUIRE(cdfh.extra_field_length == 0);
    REQUIRE(cdfh.file_comment_length == 0);
    REQUIRE(cdfh.disk == 0);
    REQUIRE(cdfh.internal_file_attributes == 1);
    REQUIRE(cdfh.external_file_attributes == 32);
    REQUIRE(cdfh.local_file_header_offset == 0);

    std::string filename(cdfh.filename_length, '\0');
    test_zip.read(&filename[0], filename.size());
    REQUIRE(filename == "test.txt");

    // Local file hader
    local_file_header lfh;
    test_zip.seek(cdfh.local_file_header_offset, seekdir::beg);
    read(test_zip, lfh);
    REQUIRE(lfh.signature == local_file_header::signature_magic);
    REQUIRE(lfh.min_version == 20);
    REQUIRE(lfh.flags == 2);
    REQUIRE(lfh.compression_method == compression_methods::deflated);
    REQUIRE(lfh.last_modified_time == 25120);
    REQUIRE(lfh.last_modified_date == 18597);
    REQUIRE(lfh.crc32 == 0x87E4F545);
    REQUIRE(lfh.compressed_size == 13);
    REQUIRE(lfh.uncompressed_size == 14);
    REQUIRE(lfh.filename_length == 8);
    REQUIRE(lfh.extra_field_length == 0);
    std::string filename2(lfh.filename_length, '\0');
    test_zip.read(&filename2[0], filename2.size());
    REQUIRE(filename2 == "test.txt");

    std::vector<uint8_t> file_data(lfh.compressed_size);
    test_zip.read(&file_data[0], file_data.size());
    REQUIRE(file_data == (std::vector<uint8_t>{0xf3, 0xc9, 0xcc, 0x4b, 0x55, 0x30, 0xe4, 0xf2, 0x01, 0x51, 0x46, 0x5c, 0x00}));
}