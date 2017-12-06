#include "moderndbs/slotted_page.h"
#include <cassert>
#include <cstring>
#include <vector>
#include <algorithm>

using SlottedPage = moderndbs::SlottedPage;
using TID = moderndbs::TID;

TID::TID(uint64_t raw_value) {
    // TODO: add your implementation here
}

TID::TID(uint64_t page, uint16_t slot) {
    // TODO: add your implementation here
}

SlottedPage::Header::Header(uint32_t page_size) {
    // TODO: add your implementation here
}

SlottedPage::Slot::Slot() {
    // TODO: add your implementation here
}

SlottedPage::SlottedPage(uint32_t page_size) : header(page_size) {
    // TODO: add your implementation here
}

void SlottedPage::compactify(uint32_t page_size) {
    // TODO: add your implementation here
}
