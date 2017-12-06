#ifndef INCLUDE_MODERNDBS_SCHEMA_H_
#define INCLUDE_MODERNDBS_SCHEMA_H_

#include <map>
#include <memory>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>

namespace moderndbs {
namespace schema {

class SchemaParser;
struct SchemaCompiler;

struct Type {
    /// Type class
    enum Class: uint8_t {
        kInteger,
        kTimestamp,
        kNumeric,
        kChar,
        kVarchar,
    };
    /// The type class
    Class tclass;
    /// The type argument (if any)
    union {
        struct {
            uint32_t length;
            uint32_t precision;
        };
    };

    /// Get type name
    const char *name() const;

    /// Static methods to construct a type
    static Type Integer();
    static Type Timestamp();
    static Type Numeric(unsigned length, unsigned precision);
    static Type Char(unsigned length);
    static Type Varchar(unsigned length);
};

struct Column {
    /// Name of the column
    const std::string id;
    /// Type of the column
    const Type type;

    /// Constructor
    explicit Column(std::string id, Type type = Type::Integer())
        : id(std::move(id)), type(type) {}
};

struct Table {
    /// Name of the table
    const std::string id;
    /// Columns
    const std::vector<Column> columns;
    /// Primary key
    const std::vector<std::string> primary_key;

    /// Constructor
    Table(std::string id, std::vector<Column> columns, std::vector<std::string> primary_key)
        : id(std::move(id)), columns(std::move(columns)), primary_key(std::move(primary_key)) {}
};

struct Schema {
    /// Tables
    const std::vector<Table> tables;

    /// Constructor
    explicit Schema(std::vector<Table> tables)
        : tables(std::move(tables)) {
    }
};

}  // namespace schema
}  // namespace moderndbs

#endif  // INCLUDE_MODERNDBS_SCHEMA_H_
