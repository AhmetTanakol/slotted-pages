#ifndef INCLUDE_MODERNDBS_SEGMENT_H_
#define INCLUDE_MODERNDBS_SEGMENT_H_

#include <atomic>
#include "moderndbs/buffer_manager.h"
#include "moderndbs/slotted_page.h"
#include "moderndbs/schema.h"

namespace moderndbs {

class Segment {
    public:
    /// Constructor
    /// @param[in] segment_id       Id of the segment.
    /// @param[in] buffer_manager   The buffer manager that should be used by the segment.
    Segment(uint16_t segment_id, BufferManager& buffer_manager)
        : segment_id(segment_id), buffer_manager(buffer_manager) {}

    protected:
    /// The segment id
    uint16_t segment_id;
    /// The buffer manager
    BufferManager& buffer_manager;
};

class SchemaSegment: public Segment {
    friend class SPSegment;
    friend class FSISegment;

    public:
    /// Constructor
    /// @param[in] segment_id       Id of the segment that the schema is stored in.
    /// @param[in] buffer_manager   The buffer manager that should be used by the schema segment.
    SchemaSegment(uint16_t segment_id, BufferManager& buffer_manager);

    /// Set the schema of the schema segment
    void set_schema(std::unique_ptr<schema::Schema> new_schema);

    /// Get the schema of the schema segment
    schema::Schema *get_schema();

    /// Set the segment id of the free-space inventory.
    /// @param[in] segment_id   Id of the fsi that is associated with the schema.
    void set_fsi_segment(uint16_t segment_id);

    /// Get the segment id of the free-space inventory associated with the schema.
    uint16_t get_fsi_segment();

    /// Set the segment id of the slotted pages.
    /// @param[in] segment_id   Id of the slotted pages that are associated with the schema.
    void set_sp_segment(uint16_t segment_id);

    /// Get the segment id of the slotted pages associated with the schema.
    uint16_t get_sp_segment();

    /// Get the number of slotted pages.
    uint64_t get_sp_count();

    /// Read the schema from disk.
    /// The schema segment should be structured as follows:
    ///   1) The segment id of the slotted pages segment
    ///   2) The segment id of the free-space inventory segment
    ///   3) The size of the slotted pages segment (in #pages)
    ///   4) The length of the serialized schema (in #bytes)
    ///   5) The serialized schema
    ///
    /// Note that the serialized schema *could* be larger than 1 page.
    void read();
    /// Write the schema to disk.
    /// Note that we need to track the number of slotted pages in the schema segment.
    /// For this assignment, you can simply write out the schema segment whenever you allocate a slotted page.
    void write();
};

class FSISegment: public Segment {
    public:
    /// Constructor
    /// @param[in] segment_id       Id of the segment that the fsi is stored in.
    /// @param[in] buffer_manager   The buffer manager that should be used by the fsi segment.
    /// @param[in] schema           The schema segment that the fsi belongs to.
    FSISegment(uint16_t segment_id, BufferManager &buffer_manager, SchemaSegment &schema);

    /// Update a the free space of a page.
    /// The free space inventory encodes the free space of a target page in 4 bits.
    /// It is left up to you whether you want to implement completely linear free space entries
    /// or half logarithmic ones. (cf. lecture slides)
    /// @param[in] target_page      The (slotted) page number.
    /// @param[in] free_space       The new free space on that page.
    void update(uint64_t target_page, uint32_t free_space);

    /// Find a page that has enough free space.
    /// @param[in] free_space       The required space.
    std::pair<bool, uint64_t> find(uint32_t required_space);
};

class SPSegment: public moderndbs::Segment {
    public:
    /// Constructor
    /// @param[in] segment_id       Id of the segment that the schema is stored in.
    /// @param[in] buffer_manager   The buffer manager that should be used by the slotted pages segment.
    /// @param[in] schema           The schema segment that the fsi belongs to.
    /// @param[in] fsi              The free-space inventory that is associated with the schema.
    SPSegment(uint16_t segment_id, BufferManager &buffer_manager, SchemaSegment &schema, FSISegment &fsi);

    /// Allocate a new record.
    /// Returns a TID that stores the page as well as the slot of the allocated record.
    /// The allocate method should use the free-space inventory to find a suitable page quickly.
    /// @param[in] size         The size that should be allocated.
    TID allocate(uint32_t size) ;

    /// Read the data of the record into a buffer.
    /// @param[in] tid          The TID that identifies the record.
    /// @param[in] record       The buffer that is read into.
    /// @param[in] capacity     The capacity of the buffer that is read into.
    uint32_t read(TID tid, std::byte *record, uint32_t capacity) const;

    /// Write a record.
    /// @param[in] tid          The TID that identifies the record.
    /// @param[in] record       The buffer that is written.
    /// @param[in] record_size  The capacity of the buffer that is written.
    uint32_t write(TID tid, std::byte *record, uint32_t record_size);

    /// Resize a record.
    /// Resize should first check whether the new size still fits on the page.
    /// If not, it yould create a redirect record.
    /// @param[in] tid          The TID that identifies the record.
    /// @param[in] new_length   The new length of the record.
    void resize(TID tid, uint32_t new_length);

    /// Removes the record from the slotted page
    /// @param[in] tid          The TID that identifies the record.
    void erase(TID tid);

    protected:
    /// Schema segment
    SchemaSegment &schema;
    /// Free space inventory
    FSISegment &fsi;
};

}  // namespace moderndbs

#endif // INCLUDE_MODERNDBS_SEGMENT_H_
