#ifndef INCLUDE_MODERNDBS_SLOTTED_PAGE_H_
#define INCLUDE_MODERNDBS_SLOTTED_PAGE_H_

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <iostream>

namespace moderndbs {

struct TID {
    /// Constructor
    explicit TID(uint64_t raw_value);
    /// Constructor
    TID(uint64_t page, uint16_t slot);

    /// The TID value
    /// The TID could, for instance, look like the following:
    /// - 48 bit page id
    /// - 16 bit slot id
    uint64_t value;
};

struct SlottedPage {
    struct Header {
        // Constructor
        explicit Header(uint32_t page_size);

        /// Number of currently used slots
        uint16_t slot_count;
        /// To speed up the search for a free slot
        uint16_t first_free_slot;
        /// Lower end of the data
        uint32_t data_start;
        /// Space that would be available after compactification
        uint32_t free_space;
    };

    struct Slot {
        /// Constructor
        Slot();

        /// The slot value
        /// c.f. chapter 3 page 13
        uint64_t value;
    };

    /// Constructor.
    /// @param[in] page_size    The size of a buffer frame.
    explicit SlottedPage(uint32_t page_size);

    /// Compact the page.
    /// @param[in] page_size    The size of a buffer frame.
    void compactify(uint32_t page_size);

    /// The header.
    /// Note that the slotted page itself should reside on the buffer frame!
    /// DO NOT allocate heap objects for a slotted page but instead reinterpret_cast BufferFrame.get_data()!
    /// This is also the reason why the constructor and compactify require the actual page size as argument.
    /// (The slotted page itself does not know how large it is)
    Header header;
};

}  // namespace moderndbs

#endif // INCLUDE_MODERNDBS_SLOTTED_PAGE_H_
