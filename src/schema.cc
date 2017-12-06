// ---------------------------------------------------------------------------------------------------
// MODERNDBS
// ---------------------------------------------------------------------------------------------------

#include "moderndbs/schema.h"
#include <memory>
#include <sstream>
#include <unordered_set>
#include "moderndbs/error.h"

using Table = moderndbs::schema::Table;
using Schema = moderndbs::schema::Schema;
using Type = moderndbs::schema::Type;

Type Type::Integer()    {
    Type t;
    t.tclass = kInteger;
    t.length = 0;
    t.precision = 0;
    return t; }
Type Type::Timestamp()  {
    Type t;
    t.tclass = kTimestamp;
    t.length = 0;
    t.precision = 0;
    return t;
}
Type Type::Numeric(unsigned length, unsigned precision) {
    Type t;
    t.tclass = kNumeric;
    t.length = length;
    t.precision = precision;
    return t;
}
Type Type::Char(unsigned length) {
    Type t;
    t.tclass = kChar;
    t.length = length;
    t.precision = 0;
    return t;
}
Type Type::Varchar(unsigned length) {
    Type t;
    t.tclass = kVarchar;
    t.length = length;
    t.precision = 0;
    return t;
}

const char *Type::name() const {
    switch (tclass) {
        case kInteger:      return "integer";
        case kTimestamp:    return "timestamp";
        case kNumeric:      return "numeric";
        case kChar:         return "char";
        case kVarchar:      return "varchar";
        default:            return "unknown";
    }
}
