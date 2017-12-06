
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <exception>
#include <utility>
#include <random>
#include <vector>
#include <gtest/gtest.h>
#include "moderndbs/segment.h"
#include "moderndbs/file.h"
#include "moderndbs/buffer_manager.h"

using BufferManager = moderndbs::BufferManager;
using FSISegment = moderndbs::FSISegment;
using SPSegment = moderndbs::SPSegment;
using SchemaSegment = moderndbs::SchemaSegment;

namespace schema = moderndbs::schema;

namespace {

std::unique_ptr<schema::Schema> getTPCHSchemaLight() {
    std::vector<schema::Table> tables {
        schema::Table(
            "customer",
            {
                schema::Column("c_custkey", schema::Type::Integer()),
                schema::Column("c_name", schema::Type::Varchar(25)),
                schema::Column("c_address", schema::Type::Varchar(40)),
                schema::Column("c_nationkey", schema::Type::Integer()),
                schema::Column("c_phone", schema::Type::Char(15)),
                schema::Column("c_acctbal", schema::Type::Numeric(12, 2)),
                schema::Column("c_mktsegment", schema::Type::Char(10)),
                schema::Column("c_comment", schema::Type::Varchar(117)),
            },
            {
                "c_custkey"
            }
        ),
        schema::Table(
            "nation",
            {
                schema::Column("n_nationkey", schema::Type::Integer()),
                schema::Column("n_name", schema::Type::Varchar(25)),
                schema::Column("n_regionkey", schema::Type::Integer()),
                schema::Column("n_comment", schema::Type::Varchar(152)),
            },
            {
                "n_nationkey"
            }
        ),
        schema::Table(
            "region",
            {
                schema::Column("r_regionkey", schema::Type::Integer()),
                schema::Column("r_name", schema::Type::Char(25)),
                schema::Column("r_comment", schema::Type::Varchar(152)),
            },
            {
                "r_regionkey"
            }
        ),
    };
    auto schema = std::make_unique<schema::Schema>(std::move(tables));
    return schema;
}

// NOLINTNEXTLINE
TEST(SegmentTest, SchemaSetter) {
    BufferManager buffer_manager(1024, 10);
    SchemaSegment schema_segment(100, buffer_manager);
    EXPECT_EQ(nullptr, schema_segment.get_schema());
    auto schema = getTPCHSchemaLight();
    auto schema_ptr = schema.get();
    schema_segment.set_schema(std::move(schema));
    EXPECT_EQ(schema_ptr, schema_segment.get_schema());
}

// NOLINTNEXTLINE
TEST(SegmentTest, SchemaSerialiseEmptySchema) {
    BufferManager buffer_manager(1024, 10);
    {
        SchemaSegment schema_segment_1(101, buffer_manager);
        std::vector<schema::Table> no_tables;
        auto schema = std::make_unique<schema::Schema>(no_tables);
        schema_segment_1.set_schema(std::move(schema));
        schema_segment_1.write();
    }
    SchemaSegment schema_segment_2(102, buffer_manager);
    schema_segment_2.read();
    ASSERT_NE(nullptr, schema_segment_2.get_schema());
    EXPECT_EQ(0, schema_segment_2.get_schema()->tables.size());
}

// NOLINTNEXTLINE
TEST(SegmentTest, SchemaSerialiseTPCHLight) {
    BufferManager buffer_manager(1024, 10);
    {
        SchemaSegment schema_segment_1(103, buffer_manager);
        auto schema_1 = getTPCHSchemaLight();
        schema_segment_1.set_schema(std::move(schema_1));
        schema_segment_1.write();
    }
    SchemaSegment schema_segment_2(104, buffer_manager);
    schema_segment_2.read();
    ASSERT_NE(nullptr, schema_segment_2.get_schema());
    auto schema_2 = schema_segment_2.get_schema();
    ASSERT_EQ(schema_2->tables.size(), 3);
    EXPECT_EQ(schema_2->tables[0].id, "customer");
    ASSERT_EQ(schema_2->tables[0].primary_key.size(), 1);
    EXPECT_EQ(schema_2->tables[0].primary_key[0], "c_custkey");
    ASSERT_EQ(schema_2->tables[0].columns.size(), 8);
    EXPECT_EQ(schema_2->tables[0].columns[0].id, "c_custkey");
    EXPECT_EQ(schema_2->tables[0].columns[0].type.tclass, schema::Type::Class::kInteger);
    EXPECT_EQ(schema_2->tables[0].columns[1].id, "c_name");
    EXPECT_EQ(schema_2->tables[0].columns[1].type.tclass, schema::Type::Class::kVarchar);
    EXPECT_EQ(schema_2->tables[0].columns[1].type.length, 25);
    EXPECT_EQ(schema_2->tables[0].columns[2].id, "c_address");
    EXPECT_EQ(schema_2->tables[0].columns[2].type.tclass, schema::Type::Class::kVarchar);
    EXPECT_EQ(schema_2->tables[0].columns[2].type.length, 40);
    EXPECT_EQ(schema_2->tables[0].columns[3].id, "c_nationkey");
    EXPECT_EQ(schema_2->tables[0].columns[3].type.tclass, schema::Type::Class::kInteger);
    EXPECT_EQ(schema_2->tables[0].columns[4].id, "c_phone");
    EXPECT_EQ(schema_2->tables[0].columns[4].type.tclass, schema::Type::Class::kChar);
    EXPECT_EQ(schema_2->tables[0].columns[4].type.length, 15);
    EXPECT_EQ(schema_2->tables[0].columns[5].id, "c_acctbal");
    EXPECT_EQ(schema_2->tables[0].columns[5].type.tclass, schema::Type::Class::kNumeric);
    EXPECT_EQ(schema_2->tables[0].columns[5].type.length, 12);
    EXPECT_EQ(schema_2->tables[0].columns[5].type.precision, 2);
    EXPECT_EQ(schema_2->tables[0].columns[6].id, "c_mktsegment");
    EXPECT_EQ(schema_2->tables[0].columns[6].type.tclass, schema::Type::Class::kChar);
    EXPECT_EQ(schema_2->tables[0].columns[6].type.length, 10);
    EXPECT_EQ(schema_2->tables[0].columns[7].id, "c_comment");
    EXPECT_EQ(schema_2->tables[0].columns[7].type.length, 117);
    EXPECT_EQ(schema_2->tables[0].columns[7].type.tclass, schema::Type::Class::kVarchar);
    EXPECT_EQ(schema_2->tables[1].id, "nation");
    ASSERT_EQ(schema_2->tables[1].columns.size(), 4);
    EXPECT_EQ(schema_2->tables[1].columns[0].id, "n_nationkey");
    EXPECT_EQ(schema_2->tables[1].columns[0].type.tclass, schema::Type::Class::kInteger);
    EXPECT_EQ(schema_2->tables[1].columns[1].id, "n_name");
    EXPECT_EQ(schema_2->tables[1].columns[1].type.tclass, schema::Type::Class::kVarchar);
    EXPECT_EQ(schema_2->tables[1].columns[1].type.length, 25);
    EXPECT_EQ(schema_2->tables[1].columns[2].id, "n_regionkey");
    EXPECT_EQ(schema_2->tables[1].columns[2].type.tclass, schema::Type::Class::kInteger);
    EXPECT_EQ(schema_2->tables[1].columns[3].id, "n_comment");
    EXPECT_EQ(schema_2->tables[1].columns[3].type.tclass, schema::Type::Class::kVarchar);
    EXPECT_EQ(schema_2->tables[1].columns[3].type.length, 152);
    ASSERT_EQ(schema_2->tables[1].primary_key.size(), 1);
    EXPECT_EQ(schema_2->tables[1].primary_key[0], "n_nationkey");
    EXPECT_EQ(schema_2->tables[2].id, "region");
    ASSERT_EQ(schema_2->tables[2].columns.size(), 3);
    EXPECT_EQ(schema_2->tables[2].columns[0].id, "r_regionkey");
    EXPECT_EQ(schema_2->tables[2].columns[0].type.tclass, schema::Type::Class::kInteger);
    EXPECT_EQ(schema_2->tables[2].columns[1].id, "r_name");
    EXPECT_EQ(schema_2->tables[2].columns[1].type.tclass, schema::Type::Class::kChar);
    EXPECT_EQ(schema_2->tables[2].columns[1].type.length, 25);
    EXPECT_EQ(schema_2->tables[2].columns[2].id, "r_comment");
    EXPECT_EQ(schema_2->tables[2].columns[2].type.tclass, schema::Type::Class::kVarchar);
    EXPECT_EQ(schema_2->tables[2].columns[2].type.length, 152);
    ASSERT_EQ(schema_2->tables[2].primary_key.size(), 1);
    EXPECT_EQ(schema_2->tables[2].primary_key[0], "r_regionkey");
}

// NOLINTNEXTLINE
TEST(SegmentTest, SPRecordSingleAllocations) {
    auto schema = getTPCHSchemaLight();
    BufferManager buffer_manager(1024, 10);
    SchemaSegment schema_segment(105, buffer_manager);
    schema_segment.set_schema(std::move(schema));
    FSISegment fsi_segment(106, buffer_manager, schema_segment);
    SPSegment sp_segment(107, buffer_manager, schema_segment, fsi_segment);
    int sizes[] = { 1, 42, 111, 333, 444, 888 };
    for (int i = 0; i < 100; ++i) {
        sp_segment.allocate(sizes[i % 3]);
    }
    EXPECT_NE(0, schema_segment.get_sp_count());
}

// NOLINTNEXTLINE
TEST(SegmentTest, SPRecordRead) {
    auto schema = getTPCHSchemaLight();
    BufferManager buffer_manager(1024, 10);
    SchemaSegment schema_segment(108, buffer_manager);
    schema_segment.set_schema(std::move(schema));
    FSISegment fsi_segment(109, buffer_manager, schema_segment);
    SPSegment sp_segment(110, buffer_manager, schema_segment, fsi_segment);

    // Allocate slot
    auto tid = sp_segment.allocate(42);

    // Write buffer
    std::vector<char> buffer1;
    buffer1.resize(42, 0xFF);
    sp_segment.write(tid, reinterpret_cast<std::byte*>(buffer1.data()), 42);

    // Read into buffer
    std::vector<char> buffer2;
    buffer2.resize(42, 0x00);
    sp_segment.read(tid, reinterpret_cast<std::byte*>(buffer2.data()), 42);

    auto buffer2_equals = std::equal(buffer2.begin(), buffer2.end(), buffer1.begin());
    EXPECT_TRUE(buffer2_equals);
}

// NOLINTNEXTLINE
TEST(SegmentTest, SPRecordResizeOnPage) {
    auto schema = getTPCHSchemaLight();
    BufferManager buffer_manager(1024, 10);
    SchemaSegment schema_segment(111, buffer_manager);
    schema_segment.set_schema(std::move(schema));
    FSISegment fsi_segment(112, buffer_manager, schema_segment);
    SPSegment sp_segment(113, buffer_manager, schema_segment, fsi_segment);

    // Allocate slot
    auto tid = sp_segment.allocate(42);

    // Write buffer
    std::vector<char> buffer1;
    buffer1.resize(42, 0xFF);
    sp_segment.write(tid, reinterpret_cast<std::byte*>(buffer1.data()), 42);

    // Resize the record
    sp_segment.resize(tid, 84);

    // Read into buffer
    std::vector<char> buffer2;
    buffer2.resize(84, 0x00);
    sp_segment.read(tid, reinterpret_cast<std::byte*>(buffer2.data()), 84);

    auto buffer2_equals = std::equal(buffer2.begin(), buffer2.begin() + 42, buffer1.begin());
    EXPECT_TRUE(buffer2_equals);
}

// NOLINTNEXTLINE
TEST(SegmentTest, SPRecordResizeFirstRedirect) {
    auto schema = getTPCHSchemaLight();
    BufferManager buffer_manager(1024, 10);
    SchemaSegment schema_segment(114, buffer_manager);
    schema_segment.set_schema(std::move(schema));
    FSISegment fsi_segment(115, buffer_manager, schema_segment);
    SPSegment sp_segment(116, buffer_manager, schema_segment, fsi_segment);

    // Allocate slot
    auto tid = sp_segment.allocate(42);

    // Fill the first page
    for (int i = 0; i < 100; ++i) {
        sp_segment.allocate(42);
    }

    // Write buffer
    std::vector<char> buffer1;
    buffer1.resize(42, 0xFF);
    sp_segment.write(tid, reinterpret_cast<std::byte*>(buffer1.data()), 42);

    // Resize the record
    sp_segment.resize(tid, 84);

    // Read into buffer
    std::vector<char> buffer2;
    buffer2.resize(84, 0x00);
    sp_segment.read(tid, reinterpret_cast<std::byte*>(buffer2.data()), 84);

    auto buffer2_equals = std::equal(buffer2.begin(), buffer2.begin() + 42, buffer1.begin());
    ASSERT_TRUE(buffer2_equals);

    // Fill the next page
    for (int i = 0; i < 100; ++i) {
        sp_segment.allocate(42);
    }

    // Resize the record again
    sp_segment.resize(tid, 120);

    // Read into buffer
    std::vector<char> buffer3;
    buffer2.resize(120, 0x00);
    sp_segment.read(tid, reinterpret_cast<std::byte*>(buffer3.data()), 120);

    auto buffer3_equals = std::equal(buffer3.begin(), buffer3.begin() + 42, buffer1.begin());
    ASSERT_TRUE(buffer3_equals);
}

}  // namespace
