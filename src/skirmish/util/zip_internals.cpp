#include "zip_internals.h"
#include <ostream>

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


void read(util::in_stream& in, end_of_central_directory_record& r)
{
    read(in, r.signature);
    read(in, r.disk_number);
    read(in, r.central_disk);
    read(in, r.central_directory_records_this_disk);
    read(in, r.central_directory_records_this_total);
    read(in, r.central_directory_size_bytes);
    read(in, r.central_directory_offset);
    read(in, r.comment_length);
}

std::ostream& operator<<(std::ostream& os, compression_methods cm)
{
    switch (cm) {
    case compression_methods::stored: return os << "stored";
    case compression_methods::deflated: return os << "deflated";
    }
    return os << "compression_methods{" << static_cast<int>(cm) << "}";
}

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

// TODO: This can be probably be done more cleverly
uint64_t find_end_of_central_directory_record(util::in_stream& in, end_of_central_directory_record& r)
{
    const auto file_size = in.stream_size();
    if (file_size < end_of_central_directory_record::min_size_bytes) {
        return invalid_file_pos;
    }

    for (uint64_t pos = file_size - end_of_central_directory_record::min_size_bytes + 1; !in.error() && pos-- && pos + 0xffff >= file_size;) {
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