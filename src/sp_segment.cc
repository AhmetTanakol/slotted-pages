#include "moderndbs/slotted_page.h"
#include "moderndbs/segment.h"
#include <cassert>
#include <cstring>
#include <algorithm>

using moderndbs::SPSegment;
using moderndbs::Segment;
using moderndbs::TID;

SPSegment::SPSegment(uint16_t segment_id, BufferManager& buffer_manager, SchemaSegment &schema, FSISegment &fsi)
    : Segment(segment_id, buffer_manager), schema(schema), fsi(fsi) {}

TID SPSegment::allocate(uint32_t size) {
    // TODO: add your implementation here
    return TID(0, 0);
}

uint32_t SPSegment::read(TID tid, std::byte *record, uint32_t capacity) const {
    // TODO: add your implementation here
    return 0;
}

uint32_t SPSegment::write(TID tid, std::byte *record, uint32_t record_size) {
    // TODO: add your implementation here
    return 0;
}

void SPSegment::resize(TID tid, uint32_t new_size) {
    // TODO: add your implementation here
}

void SPSegment::erase(TID tid) {
    // TODO: add your implementation here
}
