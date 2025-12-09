#include <rdf4cpp/parser/XMLParserStateCollector.hpp>


namespace rdf4cpp::parser {
    IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::XMLOutputQueue(state_type *state) : state_(state) {
        if (state_ == nullptr) {
            state_ = new state_type();
            state_is_owned_ = true;
        }
    }

    IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::~XMLOutputQueue() {
        if (state_is_owned_) {
            delete state_;
        }
    }

    bool IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::empty() const {
        return result_queue_.empty();
    }

    std::optional<IStreamQuadIterator::value_type> IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::next() {
        if (result_queue_.empty()) {
            return std::nullopt;
        }
        auto r = result_queue_.front();
        result_queue_.pop_front();
        return r;
    }

    std::string_view IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::current_base_iri() const {
        return state_->iri_factory.get_base();
    }

    void IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::add_error(ParsingError::Type ty, std::string msg, Info const &i) {
        result_queue_.emplace_back(nonstd::unexpect, ty, i.line, i.column, std::move(msg));
    }

    void IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::add_statement(Node subject, IRI predicate, Node object, IRI reify) {
        if (subject.null() || predicate.null() || object.null()) {
            return;
        }
        result_queue_.emplace_back(Quad(subject, predicate, object));
        if (!reify.null()) {
            result_queue_.emplace_back(Quad(reify, make_hardcoded_iri(reify_subject), subject));
            result_queue_.emplace_back(Quad(reify, make_hardcoded_iri(reify_predicate), predicate));
            result_queue_.emplace_back(Quad(reify, make_hardcoded_iri(reify_object), object));
            result_queue_.emplace_back(Quad(reify, make_type_iri(), make_hardcoded_iri(reify_type)));
        }
    }

    IRI IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::make_hardcoded_iri(std::string_view const iri) const {
        return IRI::make_unchecked(iri, state_->node_storage);
    }

    IRI IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::make_type_iri() const {
        return IRI::rdf_type(state_->node_storage);
    }

    template<typename NT>
    NT IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::inspect_node(NT node, Info const &i) {
        try {
            state_->inspect_node_func(node);
            return node;
        } catch (std::exception &e) {
            add_error(ParsingError::Type::BadSyntax, std::format("Triple explicitly skipped by inspect function: {}", e.what()), i);
        } catch (...) {
            add_error(ParsingError::Type::BadSyntax, "Triple explicitly skipped by inspect function", i);
        }
        return NT::make_null();
    }

    IRI IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::make_iri(std::string_view const iri, std::string_view const base, Info const &i) {
        if (base.empty()) {
            state_->iri_factory.set_base_unchecked(i.base);
        } else {
            state_->iri_factory.set_base_unchecked(base);
        }
        auto exp = state_->iri_factory.from_maybe_relative(iri, state_->node_storage);
        if (exp.has_value()) {
            return inspect_node(*exp, i);
        } else {
            add_error(ParsingError::Type::BadIri, std::format("{}: {}", iri, exp.error()), i);
            return IRI::make_null();
        }
    }

    IRI IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::make_iri(std::string_view const uri, std::string_view const local_name, std::string_view const base, Info const &i) {
        std::string iri{uri};
        iri.append(local_name);
        return make_iri(iri, base, i);
    }

    IRI IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::make_id(std::string_view const local_name, std::string_view const base, Info const &i) {
        std::string local = "#";
        local.append(local_name);
        auto iri = make_iri(local, base, i);
        if (reserved_ids_.contains(iri.backend_handle().id())) {
            add_error(ParsingError::Type::BadIri, std::format("{}: is already used as a rdf:ID", iri), i);
            return IRI::make_null();
        }
        reserved_ids_.insert(iri.backend_handle().id());
        return iri;
    }

    Node IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::make_bn(std::optional<std::string_view> name, Info const &i) {
        std::string n = "";
        if (!name.has_value()) {
            n = std::format("bn_{}", next_bn_index_++);
            name = n;
        }
        try {
            if (state_->blank_node_scope_manager == nullptr) {
                return inspect_node(BlankNode::make(*name, state_->node_storage), i);
            } else {
                return inspect_node(state_->blank_node_scope_manager.scope("").get_or_generate_node(*name, state_->node_storage), i);
            }
        } catch (InvalidNode const &e) {
            add_error(ParsingError::Type::BadBlankNode, e.what(), i);
            return BlankNode::make_null();
        } catch (...) {
            add_error(ParsingError::Type::BadBlankNode, "unknown error", i);
            return BlankNode::make_null();
        }
    }

    Literal IStreamQuadIterator::ImplXMLStateCollector::XMLOutputQueue::make_literal(std::string_view value, std::optional<IRI> datatype, std::optional<std::string_view> lang_tag, Info const &i) {
        Literal l = Literal::make_null();
        try {
            if (datatype.has_value()) {
                l = Literal::make_typed(value, *datatype, state_->node_storage);
            } else {
                if (!lang_tag.has_value() || lang_tag->empty()) {
                    lang_tag = i.lang_tag;
                }
                if (lang_tag.has_value() && !lang_tag->empty()) {
                    l = Literal::make_lang_tagged(value, *lang_tag, state_->node_storage);
                } else {
                    l = Literal::make_simple(value, state_->node_storage);
                }
            }
        } catch (InvalidNode const &e) {
            add_error(ParsingError::Type::BadLiteral, e.what(), i);
        } catch (...) {
            add_error(ParsingError::Type::BadLiteral, "unknown error", i);
        }
        return inspect_node(l, i);
    }

    std::string_view IStreamQuadIterator::ImplXMLStateCollector::trim_left(std::string_view v) {
        auto s = v.find_first_not_of(" \t\r\n");
        if (s == std::string_view::npos) {
            return "";
        }
        v.remove_prefix(s);
        // ReSharper disable once CppDFALocalValueEscapesFunction
        return v;
    }

    bool IStreamQuadIterator::ImplXMLStateCollector::iri_equal_pieces(std::string_view const full_iri, std::string_view const uri, std::string_view const local_name) {
        if (full_iri.size() != local_name.size() + uri.size()) {
            return false;
        }
        return full_iri.starts_with(uri) && full_iri.ends_with(local_name);
    }

}  // namespace rdf4cpp::parser