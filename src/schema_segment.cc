#include "moderndbs/schema.h"
#include "moderndbs/segment.h"
#include "moderndbs/error.h"
#include <limits>
#include <cstring>
#include <sstream>

using Segment = moderndbs::Segment;
using SchemaSegment = moderndbs::SchemaSegment;
using Schema = moderndbs::schema::Schema;
using Type = moderndbs::schema::Type;
using Table = moderndbs::schema::Table;
using Column = moderndbs::schema::Column;

SchemaSegment::SchemaSegment(uint16_t segment_id, BufferManager& buffer_manager)
    : Segment(segment_id, buffer_manager) {
}

void SchemaSegment::set_schema(std::unique_ptr<Schema> new_schema) {
    // TODO: add your implementation here
}

Schema *SchemaSegment::get_schema() {
    // TODO: add your implementation here
}

uint64_t SchemaSegment::get_sp_count() {
    // TODO: add your implementation here
    return 0;
}

void SchemaSegment::set_fsi_segment(uint16_t segment) {
    // TODO: add your implementation here
}

uint16_t SchemaSegment::get_fsi_segment() {
    // TODO: add your implementation here
    return 0;
}

void SchemaSegment::set_sp_segment(uint16_t segment) {
    // TODO: add your implementation here
}

uint16_t SchemaSegment::get_sp_segment() {
    // TODO: add your implementation here
    return 0;
}

void SchemaSegment::read() {
    // TODO: add your implementation here
}

void SchemaSegment::write() {
    // TODO: add your implementation here
}
