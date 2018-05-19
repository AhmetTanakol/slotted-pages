#include "moderndbs/schema.h"
#include "moderndbs/segment.h"
#include "moderndbs/error.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "moderndbs/buffer_manager.h"
#include "moderndbs/file.h"
#include <limits>
#include <cstring>
#include <sstream>
#include <string>
#include <iostream>

using Segment = moderndbs::Segment;
using SchemaSegment = moderndbs::SchemaSegment;
using Schema = moderndbs::schema::Schema;
using Type = moderndbs::schema::Type;
using Table = moderndbs::schema::Table;
using Column = moderndbs::schema::Column;
using BufferFrame = moderndbs::BufferFrame;

SchemaSegment::SchemaSegment(uint16_t segment_id, BufferManager& buffer_manager)
    : Segment(segment_id, buffer_manager) {
}

void SchemaSegment::set_schema(std::unique_ptr<Schema> new_schema) {
    this->schema = std::move(new_schema);
}

Schema *SchemaSegment::get_schema() {
    return this->schema.get();
}

uint64_t SchemaSegment::get_sp_count() {
    return this->number_of_sp;
}

void SchemaSegment::set_fsi_segment(uint16_t segment) {
    this->fsi_segment_id = segment;
}

uint16_t SchemaSegment::get_fsi_segment() {
    return this->fsi_segment_id;
}

void SchemaSegment::set_sp_segment(uint16_t segment) {
    this->sp_segment_id = segment;
}

uint16_t SchemaSegment::get_sp_segment() {
    return this->sp_segment_id;
}

void SchemaSegment::increase_sp_count() {
    this->number_of_sp += 1;
}

void SchemaSegment::read() {
    size_t pageSize = buffer_manager.get_page_size();
    //std::cout << "segment id: " << segment_id << std::endl;
    std::string file_name = std::to_string(segment_id);
    //std::cout << "file_name: " << file_name << std::endl;
    const char* filename = file_name.c_str();
    auto file = File::open_file(filename, File::WRITE);
    size_t offSet = 0;
    uint64_t segment_page = 0;
    std::string serializedSchema;
    uint64_t stringSize = 0;
    size_t headerSize = 0;
    size_t fileSize = file->size();
    if (fileSize < pageSize) {
        uint64_t page_id = (static_cast<uint64_t>(segment_id) << 48) | segment_page;
        auto& page = this->buffer_manager.fix_page(page_id, false);
        uint64_t segmentPageId = this->buffer_manager.get_segment_page_id(page_id);

        if (offSet == 0) {
            file->read_block((segmentPageId * pageSize) + headerSize, sizeof(uint16_t), reinterpret_cast<char *>(&this->sp_segment_id));
            headerSize += sizeof(uint16_t);
            file->read_block((segmentPageId * pageSize) + headerSize, sizeof(uint16_t), reinterpret_cast<char *>(&this->fsi_segment_id));
            headerSize += sizeof(uint16_t);
            file->read_block((segmentPageId * pageSize) + headerSize, sizeof(uint64_t), reinterpret_cast<char *>(&this->number_of_sp));
            headerSize += sizeof(uint64_t);
            file->read_block((segmentPageId * pageSize) + headerSize, sizeof(uint64_t), reinterpret_cast<char *>(&stringSize));
            serializedSchema.resize(stringSize);
            headerSize += sizeof(uint64_t);
        }

        file->read_block((segmentPageId * pageSize) + headerSize, (fileSize - headerSize), page.get_data());
        memcpy(serializedSchema.data(), page.get_data(), (fileSize - headerSize));

    } else {
        while (offSet + pageSize <= fileSize) {
            uint64_t page_id = (static_cast<uint64_t>(segment_id) << 48) | segment_page;
            auto& page = this->buffer_manager.fix_page(page_id, false);
            uint64_t segmentPageId = this->buffer_manager.get_segment_page_id(page_id);

            if (offSet == 0) {
                file->read_block((segmentPageId * pageSize) + headerSize, sizeof(uint16_t), reinterpret_cast<char *>(&this->sp_segment_id));
                headerSize += sizeof(uint16_t);
                file->read_block((segmentPageId * pageSize) + headerSize, sizeof(uint16_t), reinterpret_cast<char *>(&this->fsi_segment_id));
                headerSize += sizeof(uint16_t);
                file->read_block((segmentPageId * pageSize) + headerSize, sizeof(uint64_t), reinterpret_cast<char *>(&this->number_of_sp));
                headerSize += sizeof(uint64_t);
                file->read_block((segmentPageId * pageSize) + headerSize, sizeof(uint64_t), reinterpret_cast<char *>(&stringSize));
                serializedSchema.resize(stringSize);
                headerSize += sizeof(uint64_t);
            }

            file->read_block((segmentPageId * pageSize) + headerSize, pageSize, page.get_data());
            memcpy(serializedSchema.data(), page.get_data() + offSet, pageSize);
            ++segment_page;
            offSet += pageSize;
        }

        if (offSet < fileSize) {
            uint64_t page_id = (static_cast<uint64_t>(segment_id) << 48) | segment_page;
            auto& page = this->buffer_manager.fix_page(page_id, false);
            uint64_t segmentPageId = this->buffer_manager.get_segment_page_id(page_id);
            file->read_block((segmentPageId * pageSize) + headerSize, (file->size() - offSet), page.get_data());
            memcpy(serializedSchema.data() + offSet, page.get_data(), (file->size() - offSet));
        }
    }

    std::vector<Table> tables;
    if (serializedSchema != "") {
        rapidjson::Document document;
        document.Parse<rapidjson::kParseStopWhenDoneFlag>(serializedSchema.c_str());
        if (document.HasParseError()) {
            std::cout << "parse error:" << rapidjson::GetParseError_En(document.GetParseError()) << " offset:" << document.GetErrorOffset() << std::endl;
        }
        const auto& doc = document.GetObject();
        for(const auto& t : doc["tables"].GetArray()){
            const auto& table = t.GetObject();
            std::string tableId = table["id"].GetString();

            std::vector<Column> columns;
            for(const auto& c : table["columns"].GetArray()){
                const auto& column = c.GetObject();

                std::string typeId = column["id"].GetString();

                const auto& type = column["type"].GetObject();
                std::string tclass = type["tclass"].GetString();
                uint32_t length = type["length"].GetUint();
                uint32_t precision = type["precision"].GetUint();
                Type t;
                if (tclass == "integer") {
                    t = Type::Integer();
                } else if (tclass == "timestamp") {
                    t = Type::Timestamp();
                } else if (tclass == "numeric") {
                    t = Type::Numeric(length, precision);
                } else if (tclass == "char") {
                    t = Type::Char(length);
                } else if (tclass == "varchar") {
                    t = Type::Varchar(length);
                } else {
                    t = Type::Integer();
                }

                columns.push_back(Column(typeId, t));
            }

            std::vector<std::string> primary_key;
            for(const auto& p : table["primary_key"].GetArray()){
                primary_key.push_back(p.GetString());
            }

            tables.push_back(Table(tableId, columns, primary_key));
        }
        this->schema = std::make_unique<schema::Schema>(std::move(tables));
    } else {
        this->schema = std::make_unique<schema::Schema>(std::move(tables));
    }
}

void SchemaSegment::write() {
    std::string serializedSchema;
    if (!this->schema->tables.empty()) {
        rapidjson::Document jsonDoc;
        jsonDoc.SetObject();
        rapidjson::Value tables(rapidjson::Type::kArrayType);
        rapidjson::Document::AllocatorType &allocator = jsonDoc.GetAllocator();

        for (Table t : this->schema.get()->tables) {
            rapidjson::Value table(rapidjson::Type::kObjectType);
            table.AddMember("id", rapidjson::Value(t.id.c_str(), allocator), allocator);

            rapidjson::Value columns(rapidjson::Type::kArrayType);
            for (Column c : t.columns) {
                rapidjson::Value column(rapidjson::Type::kObjectType);
                column.AddMember("id", rapidjson::Value(c.id.c_str(), allocator), allocator);
                rapidjson::Value type(rapidjson::Type::kObjectType);
                type.AddMember("tclass", rapidjson::Value(c.type.name(), allocator), allocator);
                rapidjson::Value length(rapidjson::Type::kNumberType);
                length.SetUint(c.type.length);
                type.AddMember("length", length, allocator);
                rapidjson::Value precision(rapidjson::Type::kNumberType);
                precision.SetUint(c.type.precision);
                type.AddMember("precision", precision, allocator);
                column.AddMember("type", type, allocator);
                columns.PushBack(column, allocator);
            }
            table.AddMember("columns", columns, allocator);

            rapidjson::Value primary_key(rapidjson::Type::kArrayType);
            for (std::string k : t.primary_key) {
                primary_key.PushBack(rapidjson::Value(k.c_str(), allocator), allocator);
            }
            table.AddMember("primary_key", primary_key, allocator);
            tables.PushBack(table, allocator);
        }

        jsonDoc.AddMember("tables", tables, allocator);
        rapidjson::StringBuffer strbuf;
        rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
        jsonDoc.Accept(writer);

        serializedSchema = std::string(strbuf.GetString());
    }

    size_t offSet = 0;
    size_t pageSize = buffer_manager.get_page_size();
    size_t stringSize = serializedSchema.size();

    uint64_t segment_page = 0;
    std::string file_name = std::to_string(this->segment_id );
    const char* filename = file_name.c_str();
    auto file = File::open_file(filename, File::WRITE);
    size_t headerSize = 0;

    file->write_block(reinterpret_cast<const char *>(&sp_segment_id), 0, sizeof(uint16_t));
    headerSize += sizeof(uint16_t);
    file->write_block(reinterpret_cast<char *>(&fsi_segment_id), headerSize, sizeof(uint16_t));
    headerSize += sizeof(uint16_t);
    file->write_block(reinterpret_cast<char *>(&number_of_sp), headerSize, sizeof(uint64_t));
    headerSize += sizeof(uint64_t);
    file->write_block(reinterpret_cast<char *>(&stringSize), headerSize, sizeof(uint64_t));
    headerSize += sizeof(uint64_t);

    if (stringSize != 0) {
        if (stringSize < pageSize) {
            uint64_t page_id = (static_cast<uint64_t>(segment_id) << 48) | segment_page;
            auto& page = this->buffer_manager.fix_page(page_id, false);
            uint64_t segmentPageId = this->buffer_manager.get_segment_page_id(page_id);

            std::memcpy(page.get_data(), serializedSchema.data() + offSet, stringSize);
            file->write_block(page.get_data(), (segmentPageId * pageSize) + headerSize, stringSize);

            ++segment_page;
            offSet += pageSize;
        } else {
            while (offSet + pageSize <= stringSize) {
                uint64_t page_id = (static_cast<uint64_t>(segment_id) << 48) | segment_page;
                auto& page = this->buffer_manager.fix_page(page_id, false);
                uint64_t segmentPageId = this->buffer_manager.get_segment_page_id(page_id);

                std::memcpy(page.get_data(), serializedSchema.data() + offSet, pageSize);
                file->write_block(page.get_data(), (segmentPageId * pageSize) + headerSize, pageSize);

                ++segment_page;
                offSet += pageSize;
            }

            if (offSet < stringSize) {
                uint64_t page_id = (static_cast<uint64_t>(segment_id) << 48) | segment_page;
                auto& page = this->buffer_manager.fix_page(page_id, false);
                std::memcpy(page.get_data(), serializedSchema.data() + offSet, (stringSize - offSet));

                uint16_t segmentId = this->buffer_manager.get_segment_id(page_id);
                uint64_t segmentPageId = this->buffer_manager.get_segment_page_id(page_id);

                std::string file_name = std::to_string(segmentId);
                const char* filename = file_name.c_str();
                auto file = File::open_file(filename, File::WRITE);
                file->write_block(page.get_data(), (segmentPageId * pageSize) + headerSize, (stringSize - offSet));
            }
        }
    }
}
