#include "moderndbs/slotted_page.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <algorithm>

using SlottedPage = moderndbs::SlottedPage;
using TID = moderndbs::TID;

TID::TID(uint64_t raw_value) {
    this->value = raw_value;
}

TID::TID(uint64_t page, uint16_t slot) {
    this->value = (page<<16) | slot;
}

SlottedPage::Header::Header(uint32_t page_size) {
    /// 12 bytes for header data
    this->slot_count = 0;
    this->data_start = page_size - 12;
    this->free_space = page_size - 12;
}

SlottedPage::SlottedPage(uint32_t page_size) : header(page_size) {
    /// 12 bytes for header data
    this->data.resize(page_size - 12);
}

void SlottedPage::compactify(uint32_t page_size) {
    bool isFound = false;
    std::vector<std::byte> tempDataVector(page_size - 12);
    int temp_vector_offset = page_size - 12;
    for (int i = 0;i < static_cast<int>(this->slots.size());++i) {
        auto offSet = static_cast<int>((this->slots[i].value >> 24) & ((1ull << 24) - 1));
        auto length = static_cast<int>(this->slots[i].value & ((1ull << 24)-1));
        if (offSet == 0 && length == 0) {
            /// set the first empty slot id and increase the free space
            if (!isFound) {
                this->header.first_free_slot = static_cast<uint16_t>(i);
                this->header.free_space += length;
                isFound = true;
            }
        } else {
            for (int j=0;j < length;++j) {
                tempDataVector[temp_vector_offset - j] = this->data[offSet-j];
            }
            /// I am not sure how I should handle shifting data items and set a correct offset
            /// for each slot
            /// set new offset of data
            auto t = static_cast<int>(this->slots[i].value >> 56);
            auto s = static_cast<int>((this->slots[i].value >> 48) & ((1ull << 8)-1));
            this->slots[i].value = ((t*1ULL)<<56) | ((s*1ULL)<<48) | (temp_vector_offset*1ULL)<<24 | (length*1ULL);
            temp_vector_offset -= offSet;
        }
    }
    this->header.data_start = temp_vector_offset;
    std::memcpy(this->data.data(), tempDataVector.data(), tempDataVector.size());

}

uint16_t SlottedPage::addSlot(uint64_t value) {
    Slot slot;
    slot.value = value;
    int index = this->header.slot_count;
    slots.push_back(slot);
    this->header.slot_count += 1;
    return index;
}