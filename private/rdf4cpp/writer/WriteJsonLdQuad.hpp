#ifndef RDF4CPP_PRIVATE_WRITEJSONLDQUAD_HPP
#define RDF4CPP_PRIVATE_WRITEJSONLDQUAD_HPP

#include <rdf4cpp/writer/TryWrite.hpp>
#include <rdf4cpp/writer/SerializationState.hpp>

#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/pointer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace rdf4cpp::writer {

struct RapidJsonOStreamWrapper {
    using Ch = char;

private:
    writer::BufWriterParts writer_;

public:
    RapidJsonOStreamWrapper(writer::BufWriterParts writer) noexcept : writer_{writer} {
    }

    Ch *PutBegin() {
        assert(false);
        return nullptr;
    }

    void Put(Ch c) {
        write_str(std::string_view{&c, 1}, writer_);
    }

    void Flush() {
        writer_.flush(writer_.buffer, writer_.write_area, writer_.write_area_size, 0);
    }

    size_t PutEnd(Ch* begin) {
        assert(false);
        return 0;
    }
};

bool write_quad(Quad const &s, writer::BufWriterParts const writer, writer::SerializationState *const state) noexcept {
    RapidJsonOStreamWrapper json_writer{writer};
    rapidjson::Writer<RapidJsonOStreamWrapper> json{json_writer};

    json.StartObject();

    if (auto subject_iri = s.subject().as_iri(); !subject_iri.null()) {
        json.Key("@id");

        auto const identifier = subject_iri.identifier();
        json.String(identifier.data(), identifier.size());
    } else if (auto subject_bnode = s.subject().as_blank_node(); !subject_bnode.null()) {
        json.Key("@id");

        auto const identifier = static_cast<std::string>(subject_bnode); // TODO
        json.String(identifier.data(), identifier.size());
    } else {
        assert(false);
    }

    auto predicate = s.predicate().as_iri();
    assert(!predicate.null());

    if (predicate.is_rdf_type()) {
        json.Key("@type");
    } else {
        auto const identifier = predicate.identifier();
        json.Key(identifier.data(), identifier.size());
    }

    json.StartArray();

    if (auto object_iri = s.object().as_iri(); !object_iri.null()) {
        auto const identifier = object_iri.identifier();
        json.Key("@id");
        json.String(identifier.data(), identifier.size());
    } else if (auto object_bnode = s.object().as_blank_node(); !object_bnode.null()) {
        json.Key("@id");

        auto const identifier = static_cast<std::string>(object_bnode); // TODO
        json.String(identifier.data(), identifier.size());
    } else if (auto object_literal = s.object().as_literal(); !object_literal.null()) {

        if (object_literal.template datatype_eq<datatypes::xsd::String>()) {
            json.Key("@value");

            auto const lex = object_literal.lexical_form(); // TODO
            json.String(lex.data(), lex.size());
        } else if (object_literal.template datatype_eq<datatypes::rdf::LangString>()) {
            json.Key("@language");
            auto const lang_tag = object_literal.language_tag();
            json.String(lang_tag.data(), lang_tag.size());

            json.Key("@value");
            auto const lex = object_literal.lexical_form(); // TODO
            json.String(lex.data(), lex.size());
        } else if (object_literal.template datatype_eq<datatypes::xsd::Integer>()) {
            json.Key("@value");
            json.Int64(static_cast<uint64_t>(object_literal.template value<datatypes::xsd::Integer>())); // TODO
        } else if (object_literal.template datatype_eq<datatypes::xsd::Double>()) {
            json.Key("@value");
            json.Double(object_literal.template value<datatypes::xsd::Double>());
        }
    }

    json.EndArray();


    json.EndObject();

    return true;
}

} // namespace rdf4cpp::writer

#endif // RDF4CPP_PRIVATE_WRITEJSONLDQUAD_HPP
