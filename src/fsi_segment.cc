#include <limits>
#include <vector>
#include "moderndbs/segment.h"

using Segment = moderndbs::Segment;
using FSISegment = moderndbs::FSISegment;

FSISegment::FSISegment(uint16_t segment_id, BufferManager& buffer_manager, SchemaSegment &schema)
    : Segment(segment_id, buffer_manager) {
    schema.set_fsi_segment(segment_id);
}


void FSISegment::update(uint64_t target_page, uint32_t free_space) {
    auto sizeForEachBlock = static_cast<int>(buffer_manager.get_page_size() / 16);
    auto fsi_entries = static_cast<int>(free_space / sizeForEachBlock);
    /// each vector element contains 2 page ids
    int vector_position = target_page / 2;
    if (this->bitmap.size() == 0) {
        this->bitmap.push_back((15<<4));
    }
    if (this->bitmap.size() < target_page) {
        this->bitmap.push_back((0<<4));
    }
    auto& page_pair = *(this->bitmap.data() + vector_position);
    if (target_page % 2 == 0) {
        page_pair = ((fsi_entries*1ULL)<<4) | (page_pair & ((1ull << 4) - 1));
    } else {
        auto previousPageEmptySpace = static_cast<int>(page_pair>>4);
        page_pair = (previousPageEmptySpace*1ULL)<<4 | ((fsi_entries*1ULL) & ((1ull << 4) - 1));
    }
}

std::pair<bool, uint64_t> FSISegment::find(uint32_t required_space) {
    int pageNumber = 0;
    auto sizeForEachBlock = static_cast<int>(buffer_manager.get_page_size() / 16);
    int required_fsi_entries = 1 + ((required_space - 1) / sizeForEachBlock);
    for (uint8_t page_pair : this->bitmap) {
        auto currentPageEmptySpace = static_cast<int>(page_pair>>4);
        if (currentPageEmptySpace >= required_fsi_entries) {
            return { true, pageNumber };
        }
        auto nextPageEmptySpace = static_cast<int>(page_pair & ((1ull << 4) - 1));
        pageNumber++;
        if (nextPageEmptySpace >= required_fsi_entries) {
            return { true, pageNumber };
        }
        pageNumber++;
    }
    return { false, 0 };
}