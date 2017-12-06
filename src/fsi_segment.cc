#include <limits>
#include "moderndbs/segment.h"

using Segment = moderndbs::Segment;
using FSISegment = moderndbs::FSISegment;

FSISegment::FSISegment(uint16_t segment_id, BufferManager& buffer_manager, SchemaSegment &schema)
    : Segment(segment_id, buffer_manager) {
}

void FSISegment::update(uint64_t target_page, uint32_t free_space) {
    // TODO: add your implementation here
}

std::pair<bool, uint64_t> FSISegment::find(uint32_t required_space) {
    // TODO: add your implementation here
    return { false, 0 };
}
