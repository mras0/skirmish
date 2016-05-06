#include <skirmish/util/stream.h>
#include <skirmish/util/zip_internals.h>
#include <skirmish/util/zip.h>
#include "catch.hpp"
#include <vector>
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
    REQUIRE(r.central_directory_offset == 0);
    REQUIRE(r.comment_length == 0);

    in_zip_archive za{empty_zip};
    REQUIRE(za.filenames() == (std::vector<std::string>{}));
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
    REQUIRE(r.central_directory_offset == 51);

    const std::string expected_comment = "This is a comment!";
    REQUIRE(r.comment_length == expected_comment.length());
    REQUIRE(test_zip.tell() == expected_end_of_central_directory_record_pos+22);
    std::string comment(r.comment_length, '\0');
    test_zip.read(&comment[0], r.comment_length);
    REQUIRE(test_zip.error() == std::error_code());
    REQUIRE(comment == expected_comment);

    // Central directory file header
    central_directory_file_header cdfh;
    test_zip.seek(r.central_directory_offset, seekdir::beg);
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

    in_zip_archive za{test_zip};
    REQUIRE(za.filenames() == (std::vector<std::string>{"test.txt"}));
    auto file_stream = za.get_file_stream("test.txt");
    REQUIRE(file_stream->stream_size() == 14);
    char buffer[14];
    file_stream->read(buffer, sizeof(buffer));
    REQUIRE(file_stream->tell() == 14);
    REQUIRE(file_stream->error() == std::error_code());
    REQUIRE(std::string(buffer, buffer+sizeof(buffer)) == "Line 1\nLine 2\n");
}

TEST_CASE("find_end_of_central_directory_record doesn't seek too far") {
    std::vector<uint8_t> zeros(0x20000);
    in_mem_stream zip{make_array_view(zeros)};
    end_of_central_directory_record dir_end;
    REQUIRE(find_end_of_central_directory_record(zip, dir_end) == invalid_file_pos);
    REQUIRE(zip.tell() >= 0x10000);
}

TEST_CASE("test_data.zip") {
    in_file_stream zip{(std::string{TEST_DATA_DIR} + "/" + "test_data.zip").c_str()};

    end_of_central_directory_record dir_end;
    REQUIRE(find_end_of_central_directory_record(zip, dir_end) != invalid_file_pos);
    REQUIRE(dir_end.disk_number == 0);
    REQUIRE(dir_end.central_disk == 0);
    REQUIRE(dir_end.central_directory_size_bytes >= central_directory_file_header::min_size_bytes);

    std::vector<std::string> filenames;
    zip.seek(dir_end.central_directory_offset, seekdir::beg);
    while (zip.tell() < dir_end.central_directory_offset + dir_end.central_directory_size_bytes) {
        central_directory_file_header cdfh;
        read(zip, cdfh);

        REQUIRE(cdfh.signature == central_directory_file_header::signature_magic);
        REQUIRE((cdfh.version>>8) == 0);
        REQUIRE(cdfh.min_version <= 20);
        auto supported_compression_method = [](compression_methods compression_method) { return compression_method == compression_methods::deflated || compression_method == compression_methods::stored; };
        REQUIRE(supported_compression_method(cdfh.compression_method));
        REQUIRE(cdfh.disk == 0);
 
        std::string filename(cdfh.filename_length, '\0');
        zip.read(&filename[0], filename.size());
        zip.seek(cdfh.extra_field_length, seekdir::cur);
        zip.seek(cdfh.file_comment_length, seekdir::cur);
        const bool is_dir = (cdfh.external_file_attributes & 0x10) != 0;
        if (!is_dir) {
            filenames.push_back(filename);
        }
    }
    const auto expected_filenames = std::vector<std::string>{"test_data/empty.zip", "test_data/test.txt", "test_data/test.zip"};
    REQUIRE(filenames == expected_filenames);

    in_zip_archive za{zip};
    REQUIRE(za.filenames() == expected_filenames);

    auto file_stream = za.get_file_stream("test_data/test.txt");
    REQUIRE(file_stream->stream_size() == 14);
    char buffer[14];
    file_stream->read(buffer, sizeof(buffer));
    REQUIRE(file_stream->tell() == 14);
    REQUIRE(file_stream->error() == std::error_code());
    REQUIRE(std::string(buffer, buffer+sizeof(buffer)) == "Line 1\nLine 2\n");
    file_stream->get();
    REQUIRE(file_stream->error() != std::error_code());
}

#if 0
#include <iostream>
#include <fstream>

std::string get_line(in_stream& in)
{
    std::string s;
    while (!in.error()) {
        char c = in.get();
        if (c == '\n') break;
        s += c;
    }
    return s;
}

void check(in_stream& in)
{
    const int N = (1<<18);
    /*
    std::ofstream o("big.txt", std::ofstream::binary);
    for (int i = 0; i < N; ++i) {
    o << i << '\n';
    }
    */
    for (int i = 0; i < N; ++i) {
        REQUIRE(get_line(in) == std::to_string(i));
    }
}

TEST_CASE("bigger zip file") {
    //in_file_stream in{std::string(TEST_DATA_DIR) + "/" + "big.txt"};
    //check(in);

    in_file_stream zip{std::string(TEST_DATA_DIR) + "/" + "big.zip"};
    in_zip_archive za{zip};
    auto fs = za.get_file_stream("big.txt");
    check(*fs);
}
#endif