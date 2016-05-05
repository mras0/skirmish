#include <skirmish/util/stream.h>
#include <skirmish/util/zip_internals.h>
#include "catch.hpp"
#include <vector>
#include <iostream>
#include <cassert>

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