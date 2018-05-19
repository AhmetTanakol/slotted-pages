#include "moderndbs/slotted_page.h"
#include "moderndbs/segment.h"
#include "moderndbs/file.h"
#include <cassert>
#include <cstring>
#include <algorithm>

using moderndbs::SPSegment;
using moderndbs::Segment;
using moderndbs::TID;
using moderndbs::SlottedPage;

SPSegment::SPSegment(uint16_t segment_id, BufferManager& buffer_manager, SchemaSegment &schema, FSISegment &fsi)
    : Segment(segment_id, buffer_manager), schema(schema), fsi(fsi) {
    schema.set_sp_segment(segment_id);
}

TID SPSegment::allocate(uint32_t size) {
    std::pair<bool, uint64_t> result = fsi.find(size);
    if (!result.first) {
        uint64_t page_id = schema.get_sp_count();
        auto& page = buffer_manager.fix_page(page_id, false);
        auto slottedPage = new(page.get_data()) SlottedPage(buffer_manager.get_page_size());
        auto offSet = static_cast<int>(slottedPage->header.data_start);
        uint64_t value = (255 * 1ULL) << 56 | (0*1ULL) << 48 | ((offSet * 1ULL) << 24);
        uint64_t slotId = slottedPage->addSlot(value);
        slottedPage->header.data_start -= static_cast<int>(size);
        slottedPage->header.free_space -= (static_cast<int>(size) + 8);
        fsi.update(page_id, slottedPage->header.free_space);
        schema.increase_sp_count();
        schema.write();
        return TID(page_id, slotId);
    } else {
        auto& page = buffer_manager.fix_page(result.second, false);
        auto slottedPage = reinterpret_cast<SlottedPage*>(page.get_data());
        /// unfortunately I couldn't implement this, I might misunderstand how deletion should work
        /// I set offset and data length to 0 in slot value, but I couldn't find a proper mechanism
        /// to shift empty data items.
        //slottedPage->compactify(buffer_manager.get_page_size());
        auto offSet = static_cast<int>(slottedPage->header.data_start);
        uint64_t value = (255 * 1ULL << 56) | (0*1ULL) << 48 | ((offSet * 1ULL) << 24);
        uint64_t slotId = slottedPage->addSlot(value);
        slottedPage->header.data_start -= static_cast<int>(size);
        slottedPage->header.free_space -= (static_cast<int>(size) + 8);

        fsi.update(result.second, slottedPage->header.free_space);
        return TID(result.second, slotId);
    }
}

uint32_t SPSegment::read(TID tid, std::byte *record, uint32_t capacity) const {
    uint64_t page_id = tid.value >> 16;
    uint16_t slot_id = tid.value & ((1ull << 16) - 1);
    auto& page = buffer_manager.fix_page(page_id, false);
    auto slottedPage = reinterpret_cast<SlottedPage*>(page.get_data());
    auto& slot = *(slottedPage->slots.data() + slot_id);
    uint8_t t = slot.value >> 56;
    /// check t
    if (t == 255) {
        /// the item was not redirected
        if (((slot.value >> 48) & ((1ull << 8)-1)) == 0) {
            auto offSet = static_cast<int>((slot.value >> 24) & ((1ull << 24) - 1));
            for(auto i = 0; i < static_cast<int>(capacity); ++i){
                record[i] = slottedPage->data[offSet - i];
            }
        }
    } else {
        /// we need to find the new page since the item was redirected
        uint64_t redirected_page_id = slot.value >> 16;
        uint16_t redirected_slot_id = slot.value & ((1ull << 16) - 1);
        auto& redirected_page = buffer_manager.fix_page(redirected_page_id, false);
        auto redirected_slottedPage = reinterpret_cast<SlottedPage*>(redirected_page.get_data());
        auto& redirected_slot = *(redirected_slottedPage->slots.data() + redirected_slot_id);
        /// write the data into page
        auto offSet = static_cast<int>((redirected_slot.value >> 24) & ((1ull << 24) - 1));
        for(auto i = 0; i < static_cast<int>(capacity); ++i){
            record[i] = *(redirected_slottedPage->data.begin() + (offSet - i));
        }
    }

    return 0;
}

uint32_t SPSegment::write(TID tid, std::byte *record, uint32_t record_size) {
    uint64_t page_id = tid.value >> 16;
    uint16_t slot_id = tid.value & ((1ull << 16) - 1);
    auto& page = buffer_manager.fix_page(page_id, false);
    auto slottedPage = reinterpret_cast<SlottedPage*>(page.get_data());
    auto& slot = *(slottedPage->slots.data() + slot_id);
    uint8_t t = (slot.value >> 56);
    if (t != 255) {
        uint64_t redirected_page_id = slot.value >> 16;
        uint16_t redirected_slot_id = slot.value & ((1ull << 16) - 1);
        auto& redirected_page = buffer_manager.fix_page(redirected_page_id, false);
        auto redirected_slottedPage = reinterpret_cast<SlottedPage*>(redirected_page.get_data());
        auto& redirected_slot = *(redirected_slottedPage->slots.data() + redirected_slot_id);
        auto offSet = static_cast<int>((redirected_slot.value >> 24) & ((1ull << 24) - 1));
        for(auto i = 0; i < static_cast<int>(record_size); ++i){
            slottedPage->data[offSet - i] = record[i];
        }
    } else {
        auto offSet = static_cast<int>((slot.value >> 24) & ((1ull << 24) - 1));
        for(auto i = 0; i < static_cast<int>(record_size); ++i){
            slottedPage->data[offSet - i] = record[i];
        }
        slot.value = (255*1ULL)<<56 | (offSet*1ULL)<<24 | (record_size*1ULL);
    }
    return 0;
}

void SPSegment::resize(TID tid, uint32_t new_size) {
    uint64_t page_id = (tid.value*1ULL) >> 16;
    uint16_t slot_id = tid.value & ((1ull << 16) - 1);
    auto& page = buffer_manager.fix_page(page_id, false);
    auto slottedPage = reinterpret_cast<SlottedPage*>(page.get_data());
    auto& slot = *(slottedPage->slots.data() + slot_id);
    auto tValue =  static_cast<int>(slot.value >> 56);
    uint64_t redirected_page_id;
    uint16_t redirected_slot_id;
    if (tValue != 255) {
        redirected_page_id = (slot.value*1ULL) >> 16;
        redirected_slot_id = slot.value & ((1ull << 16) - 1);
        page = buffer_manager.fix_page(redirected_page_id, false);
        slottedPage = reinterpret_cast<SlottedPage*>(page.get_data());
        slot = *(slottedPage->slots.data() + redirected_slot_id);
    }
    /// unfortunately I couldn't implement this, I might misunderstand how deletion should work
    /// I set offset and data length to 0 in slot value, but I couldn't find a proper mechanism
    /// to shift empty data items.
    //slottedPage->compactify(buffer_manager.get_page_size());
    uint32_t emptySpace = slottedPage->header.free_space;
    if (new_size <= emptySpace) {
        auto initialSize = static_cast<int>(slot.value & ((1ull << 24)-1));
        slottedPage->header.free_space -= (static_cast<int>(new_size) - initialSize);
        fsi.update(page_id, slottedPage->header.free_space);
        /// update slot data length
        slot.value = slot.value | (new_size & ((1ull << 24) - 1));
    } else {

        /// first get the data that is going to be moved to another page
        auto dataOffSet = static_cast<int>((slot.value >> 24) & ((1ull << 24) - 1));
        auto dataLength = static_cast<int>(slot.value & ((1ull << 24)-1));
        std::vector<std::byte> tempDataVector;
        tempDataVector.reserve(dataLength);
        for(auto i = 0; i < dataLength; ++i){
            tempDataVector.push_back(slottedPage->data[dataOffSet - i]);
        }
        /// erase record on the current page, because we will move it to another page
        uint64_t tempTidValue = tid.value;
        erase(tid);
        std::pair<bool, uint64_t> result = fsi.find(new_size + 8);

        if (!result.first) {
            uint64_t new_page_id = schema.get_sp_count();
            auto& new_page = buffer_manager.fix_page(new_page_id, false);
            auto new_slottedPage = new(new_page.get_data()) SlottedPage(buffer_manager.get_page_size());
            /// 8 bytes for original TID
            new_slottedPage->header.free_space -= (new_size + 8);
            auto offSet = static_cast<int>(new_slottedPage->header.data_start);
            /// first write original TID before actual record
            for(auto i = 0; i < 8; ++i) {
                new_slottedPage->data[offSet - i] = static_cast<std::byte>((tempTidValue >> i) & 0xff);
            }
            /// skip 8 bytes, because we wrote original TID there
            offSet = static_cast<int>(new_slottedPage->header.data_start) - 8;
            /// write the new data on the new page
            for(auto i = 0; i < static_cast<int>(tempDataVector.size()); ++i){
                new_slottedPage->data[offSet - i] = tempDataVector[i];
            }
            /// set slot value of redirected item, t is not equal to 255, s is not equal to 0
            uint64_t value = (0*1ULL)<<56 | (255*1ULL)<<48 | (offSet*1ULL<<24) | (new_size*1ULL);
            uint64_t new_slotId = new_slottedPage->addSlot(value);
            /// set data starting point
            new_slottedPage->header.data_start -= (static_cast<int>(new_size) + 8);

            /// update bitmap with the free space
            fsi.update(new_page_id, new_slottedPage->header.free_space);
            schema.increase_sp_count();
            schema.write();
            /// write the new TID into slot value
            auto new_tid = TID(new_page_id, new_slotId);
            slot.value = new_tid.value;
        } else {
            auto& new_page = buffer_manager.fix_page(result.second, false);
            auto new_slottedPage = reinterpret_cast<SlottedPage*>(new_page.get_data());
            /// 8 bytes for original TID
            new_slottedPage->header.free_space -= static_cast<int>(new_size + 8);
            auto offSet =  static_cast<int>(new_slottedPage->header.data_start);
            /// first write original TID before actual record
            for(auto i = 0; i < 8; ++i) {
                new_slottedPage->data[offSet - i] = static_cast<std::byte>((tempTidValue >> (i * 8)) & 0xff);
            }
            /// skip 8 bytes, because we wrote original TID there
            offSet = static_cast<int>(new_slottedPage->header.data_start) - 8;
            /// write the new data on the new page
            for(auto i = 0; i < static_cast<int>(tempDataVector.size()); ++i){
                new_slottedPage->data[offSet - i] = tempDataVector[i];
            }

            uint64_t value = (255*1ULL)<<56 | (0*1ULL)<<48 | (offSet*1ULL)<<24 | (new_size*1ULL);
            uint64_t new_slotId = new_slottedPage->addSlot(value);
            /// set data starting point
            new_slottedPage->header.data_start -= (static_cast<int>(new_size) + 8);
            /// update bitmap with the free space
            fsi.update(result.second, new_slottedPage->header.free_space);
            /// write the new TID into slot value
            auto new_tid = TID(result.second, new_slotId);
            /// here I need to get the original page if the redirected item is redirected again.
            /// because slot is pointed to the slot on the redirected page
            if (tValue != 255) {
                auto& original_page = buffer_manager.fix_page(page_id, false);
                auto original_slottedPage = reinterpret_cast<SlottedPage*>(original_page.get_data());
                auto& original_slot = *(original_slottedPage->slots.data() + slot_id);
                original_slot.value = new_tid.value;
            } else {
                slot.value = new_tid.value;
            }
        }
    }
}

void SPSegment::erase(TID tid) {
    uint64_t page_id = (tid.value*1ULL) >> 16;
    uint16_t slot_id = tid.value & ((1ull << 16) - 1);
    auto& page = buffer_manager.fix_page(page_id, false);
    auto slottedPage = reinterpret_cast<SlottedPage*>(page.get_data());
    auto& slot = *(slottedPage->slots.data() + slot_id);
    auto offSet = static_cast<int>((slot.value >> 24) & ((1ull << 24) - 1));
    auto length = static_cast<int>(slot.value & ((1ull << 24)-1));
    for(auto i = 0; i < length; ++i){
        slottedPage->data[offSet - i] = static_cast<std::byte>(0);
    }
    /// set empty slot
    slot.value = (255*1ULL<<56);
}
