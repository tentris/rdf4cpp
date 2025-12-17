#ifndef RDF4CPP_XMLPARSER_H
#define RDF4CPP_XMLPARSER_H

#include <rdf4cpp/Expected.hpp>
#include <rdf4cpp/Quad.hpp>
#include <rdf4cpp/IRIFactory.hpp>
#include <rdf4cpp/parser/IStreamQuadIterator.hpp>
#include <rdf4cpp/parser/XMLParserUtility.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserBaseState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserInitialState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserRDFState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserDescriptionState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserPredicateState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserTypedLiteralPredicateState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserXMLLiteralState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserCollectionState.hpp>
#include <rdf4cpp/parser/XMLStates/XMLParserEmptyElement.hpp>

#include <dice/sparse-map/sparse_set.hpp>
#include <dice/template-library/inplace_polymorphic.hpp>

#include <libxml/parser.h>

#include <memory>
#include <vector>

namespace rdf4cpp::parser {
    struct IStreamQuadIterator::ImplXML final : Impl {
    private:
        xmlSAXHandler handler_;
        // workaround for gcc-14 bug, erroneously warns on unsing a lambda here
        // see https://github.com/NVIDIA/stdexec/issues/1143
        struct XmlParserCtxtDtorLambda {
            void operator()(xmlParserCtxt *c) const {
                xmlFreeParserCtxt(c);
            }
        };
        std::unique_ptr<xmlParserCtxt, XmlParserCtxtDtorLambda> context_;
        void *reader_obj_;
        ReadFunc read_func_;
        ErrorFunc error_func_;
        EOFFunc eof_func_;
        XMLOutputQueue output_;

        using State = dice::template_library::inplace_polymorphic<xml_states::BaseState,
                                                                  xml_states::InitialState, xml_states::RDFState,
                                                                  xml_states::DescriptionState, xml_states::PredicateState,
                                                                  xml_states::TypedLiteralPredicateState, xml_states::EmptyElement,
                                                                  xml_states::XMLLiteralState, xml_states::CollectionState>;

        std::vector<State> state_stack_; // Note: we use a vector because std::stack does not have .reserve()

        [[nodiscard]] xml_states::BaseState const &current_state() const noexcept {
            return *state_stack_.back();
        }

        [[nodiscard]] xml_states::BaseState &current_state() noexcept {
            return *state_stack_.back();
        }

        static xmlSAXHandler make_sax_handler();

        void handle_state_transition(StateTransition transition);

        static void on_error(void *th, char const *msg, ...);

        [[nodiscard]] XMLStateInfo make_info() const;

    public:
        ImplXML(void *obj, ReadFunc read, ErrorFunc err, EOFFunc eof, state_type *state);

        ImplXML(ImplXML const &) = delete;
        ImplXML &operator=(ImplXML const &) = delete;
        ImplXML(ImplXML &&) = delete;
        ImplXML &operator=(ImplXML &&) = delete;
        ~ImplXML() override = default;

        [[nodiscard]] std::optional<value_type> next() override;

        [[nodiscard]] uint64_t current_line() const noexcept override;
        [[nodiscard]] uint64_t current_column() const noexcept override;
    };

    struct StateTransition {
        using ModifyStateStack = std::variant<NoStateChange, PopState,
                                              xml_states::RDFState, xml_states::DescriptionState,
                                              xml_states::PredicateState, xml_states::TypedLiteralPredicateState,
                                              xml_states::XMLLiteralState, xml_states::CollectionState, xml_states::EmptyElement>;

        ModifyStateStack modify_state;

        template<typename ...Args>
        explicit StateTransition(Args &&...args) : modify_state(std::forward<Args>(args)...) {
        }

        StateTransition() noexcept : StateTransition(std::in_place_type_t<NoStateChange>{}) {
        }
    };
}  // namespace rdf4cpp::parser

#endif  //RDF4CPP_XMLPARSER_H
