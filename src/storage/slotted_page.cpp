#include "storage/slotted_page.h"
#include <cstring>
#include <cassert>

namespace minidb {

SlottedPage::SlottedPage(char* buffer, bool init) : data_(buffer) {
  // First is page header and then slots array.
  header_ = reinterpret_cast<SlottedPageHeader*>(buffer);
  slots_ = reinterpret_cast<Slot*>(buffer + sizeof(SlottedPageHeader));
  
  if (init) {
    std::memset(buffer, 0, PAGE_SIZE);
    header_->page_type = PageType::Data;
    header_->free_space_pointer = PAGE_SIZE;
    header_->num_slots = 0;
    header_->next_page_id = INVALID_PAGE_ID;
    header_->prev_page_id = INVALID_PAGE_ID;
	header_->tuple_count = 0;
  }
}

size_t SlottedPage::get_free_space() const {
  size_t headers_size = sizeof(SlottedPageHeader) + (header_->num_slots * sizeof(Slot));
  if (headers_size > header_->free_space_pointer) {
      // Should not happen unless corrupted
      return 0;
  }
  return header_->free_space_pointer - headers_size;
}

uint16_t SlottedPage::get_num_slots() const {
  return header_->num_slots;
}

uint16_t SlottedPage::get_tuple_count() const {
  return header_->tuple_count;
}

int32_t SlottedPage::insert_tuple(const char* tuple_data, uint32_t tuple_size) {

  //First search for a free slot
  auto target_slot_id = -1;
  for (auto i = 0; i < header_->num_slots; ++i) {
      if (slots_[i].length == 0) {
          target_slot_id = i;
          break;
      }
  }

  size_t needed_space = tuple_size;
  if (target_slot_id == -1) {
      // No free slot found, need to allocate new one
      needed_space += sizeof(Slot);
  }
  
  if (get_free_space() < needed_space) {
    return -1; // Not enough space
  }

  // Insert data
  header_->free_space_pointer -= tuple_size;
  // Copy data to the new location
  std::memcpy(data_ + header_->free_space_pointer, tuple_data, tuple_size);

  if (target_slot_id != -1) {
      // Reuse existing slot
      slots_[target_slot_id].offset = header_->free_space_pointer;
      slots_[target_slot_id].length = tuple_size;
      header_->tuple_count++;
      return target_slot_id;
  }
  
  // Set up the slot
  // Note: slots_ points to the beginning of slots array.
  // Accessing slots_[i] works as expected.
  slots_[header_->num_slots].offset = header_->free_space_pointer;
  slots_[header_->num_slots].length = tuple_size;
  
  header_->tuple_count++;
  return header_->num_slots++;
}

char* SlottedPage::get_tuple(uint16_t slot_id, uint32_t* size) {
  if (slot_id >= header_->num_slots) {
    return nullptr;
  }
  
  Slot& slot = slots_[slot_id];
  if (slot.length == 0) {
    return nullptr; // Deleted
  }

  assert(slot.offset + slot.length <= PAGE_SIZE);
  
  if (size) *size = slot.length;
  return data_ + slot.offset;
}

bool SlottedPage::delete_tuple(uint16_t slot_id) {
  if (slot_id >= header_->num_slots) {
    return false;
  }
  
  // Logical delete: just set length to 0
  // Note: We do NOT reclaim space immediately (needs compaction)
  slots_[slot_id].length = 0;
  header_->tuple_count--;
  return true;
}

} // namespace minidb
