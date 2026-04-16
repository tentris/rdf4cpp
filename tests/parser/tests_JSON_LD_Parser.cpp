#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

#include <filesystem>
#include <curl/curl.h>

#include "parser_test_helpers.hpp"

using namespace rdf4cpp;
using namespace rdf4cpp::parser;

void jsonld_test_positive(std::string json_str, std::string nt_str, std::string_view base_iri, bool dedup = false) {
    parse_test_helpers::jsonld_test_positive(std::move(json_str), std::move(nt_str), base_iri, ParsingFlag::JsonLd, ParsingFlag::NQuads, dedup);
}

void jsonld_test_negative(std::string json_str, std::string_view base_iri) {
    parse_test_helpers::parser_test_negative(std::move(json_str), base_iri, ParsingFlag::JsonLd);
}

// adopted from https://stackoverflow.com/questions/9786150/save-curl-content-result-into-a-string-in-c/9786295#9786295
static size_t write_callback(void const *contents, size_t size, size_t nmemb, void *userp) {
    static_cast<std::string *>(userp)->append(static_cast<char const *>(contents), size * nmemb);
    return size * nmemb;
}

std::string remote_test_file_to_str(std::string const &file_name) {
    if (!std::filesystem::exists("./jsonld_test_cache")) { // this ends up in the cmake folder, next to the executable
        std::filesystem::create_directories("./jsonld_test_cache");
    }
    auto cache = std::format("./jsonld_test_cache/{}", file_name);
    if (std::filesystem::exists(cache)) {
        std::ifstream ifs(cache);
        return std::string{std::istreambuf_iterator{ifs}, {}};
    }
    CURL *curl = nullptr;
    CURLcode curl_res;
    auto const url = std::format("https://raw.githubusercontent.com/w3c/json-ld-streaming/refs/heads/main/tests/stream-toRdf/{}", file_name);
    std::string file_contents_as_str;
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file_contents_as_str);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // for https
        curl_res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    REQUIRE_EQ(curl_res, CURLE_OK);
    std::ofstream ofs(cache);
    ofs << file_contents_as_str;
    return file_contents_as_str;
}

TEST_CASE("test cases from rdf-tests") {
    // positive tests
    jsonld_test_positive(remote_test_file_to_str("0001-in.jsonld"), remote_test_file_to_str("0001-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0001");
    jsonld_test_positive(remote_test_file_to_str("0002-in.jsonld"), remote_test_file_to_str("0002-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0002");
    jsonld_test_positive(remote_test_file_to_str("0003-in.jsonld"), remote_test_file_to_str("0003-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0003");
    jsonld_test_positive(remote_test_file_to_str("0004-in.jsonld"), remote_test_file_to_str("0004-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0004");
    jsonld_test_positive(remote_test_file_to_str("0005-in.jsonld"), remote_test_file_to_str("0005-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0005");
    jsonld_test_positive(remote_test_file_to_str("0006-in.jsonld"), remote_test_file_to_str("0006-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0006");
    jsonld_test_positive(remote_test_file_to_str("0007-in.jsonld"), remote_test_file_to_str("0007-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0007");
    jsonld_test_positive(remote_test_file_to_str("0008-in.jsonld"), remote_test_file_to_str("0008-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0008");
    jsonld_test_positive(remote_test_file_to_str("0009-in.jsonld"), remote_test_file_to_str("0009-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0009");
    jsonld_test_positive(remote_test_file_to_str("0010-in.jsonld"), remote_test_file_to_str("0010-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0010");
    jsonld_test_positive(remote_test_file_to_str("0011-in.jsonld"), remote_test_file_to_str("0011-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0011");
    jsonld_test_positive(remote_test_file_to_str("0012-in.jsonld"), remote_test_file_to_str("0012-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0012");
    jsonld_test_positive(remote_test_file_to_str("0013-in.jsonld"), remote_test_file_to_str("0013-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0013");
    jsonld_test_positive(remote_test_file_to_str("0014-in.jsonld"), remote_test_file_to_str("0014-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0014");
    jsonld_test_positive(remote_test_file_to_str("0015-in.jsonld"), remote_test_file_to_str("0015-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0015");
    jsonld_test_positive(remote_test_file_to_str("0016-in.jsonld"), remote_test_file_to_str("0016-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/0016-in.jsonld");
    jsonld_test_positive(remote_test_file_to_str("0017-in.jsonld"), remote_test_file_to_str("0017-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/0017-in.jsonld");
    jsonld_test_positive(remote_test_file_to_str("0018-in.jsonld"), remote_test_file_to_str("0018-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/0018-in.jsonld");
    jsonld_test_positive(remote_test_file_to_str("0019-in.jsonld"), remote_test_file_to_str("0019-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0019");
    jsonld_test_positive(remote_test_file_to_str("0020-in.jsonld"), remote_test_file_to_str("0020-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0020");
    jsonld_test_positive(remote_test_file_to_str("0022-in.jsonld"), remote_test_file_to_str("0022-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0022");
    jsonld_test_positive(remote_test_file_to_str("0023-in.jsonld"), remote_test_file_to_str("0023-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0023");
    jsonld_test_positive(remote_test_file_to_str("0024-in.jsonld"), remote_test_file_to_str("0024-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0024");
    jsonld_test_positive(remote_test_file_to_str("0025-in.jsonld"), remote_test_file_to_str("0025-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0025");
    jsonld_test_positive(remote_test_file_to_str("0026-in.jsonld"), remote_test_file_to_str("0026-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0026");
    jsonld_test_positive(remote_test_file_to_str("0027-in.jsonld"), remote_test_file_to_str("0027-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0027");
    jsonld_test_positive(remote_test_file_to_str("0028-in.jsonld"), remote_test_file_to_str("0028-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0028");
    jsonld_test_positive(remote_test_file_to_str("0029-in.jsonld"), remote_test_file_to_str("0029-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0029");
    jsonld_test_positive(remote_test_file_to_str("0030-in.jsonld"), remote_test_file_to_str("0030-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0030");
    jsonld_test_positive(remote_test_file_to_str("0031-in.jsonld"), remote_test_file_to_str("0031-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0031");
    jsonld_test_positive(remote_test_file_to_str("0032-in.jsonld"), remote_test_file_to_str("0032-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0032");
    jsonld_test_positive(remote_test_file_to_str("0033-in.jsonld"), remote_test_file_to_str("0033-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0033");
    jsonld_test_positive(remote_test_file_to_str("0034-in.jsonld"), remote_test_file_to_str("0034-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0034");
    // 9.9 as int
    //jsonld_test_positive(remote_test_file_to_str("0035-in.jsonld"), remote_test_file_to_str("0035-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0035");
    parse_test_helpers::jsonld_test_positive(R"({
  "@context": {
    "double": {
      "@id": "http://example.com/double",
      "@type": "http://www.w3.org/2001/XMLSchema#double"
    },
    "integer": {
      "@id": "http://example.com/integer",
      "@type": "http://www.w3.org/2001/XMLSchema#integer"
    }
  },
  "double": [1, 2.2 ],
  "integer": [8, 9 ]
})", R"(_:b0 <http://example.com/double> "1.0E0"^^<http://www.w3.org/2001/XMLSchema#double> .
_:b0 <http://example.com/double> "2.2E0"^^<http://www.w3.org/2001/XMLSchema#double> .
_:b0 <http://example.com/integer> "8"^^<http://www.w3.org/2001/XMLSchema#integer> .
_:b0 <http://example.com/integer> "9"^^<http://www.w3.org/2001/XMLSchema#integer> .)", "https://w3c.github.io/json-ld-streaming/tests/t0035", ParsingFlag::JsonLd, ParsingFlag::NQuads);

    jsonld_test_positive(remote_test_file_to_str("0036-in.jsonld"), remote_test_file_to_str("0036-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0036");
    jsonld_test_positive(remote_test_file_to_str("0113-in.jsonld"), remote_test_file_to_str("0113-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0113");
    jsonld_test_positive(remote_test_file_to_str("0114-in.jsonld"), remote_test_file_to_str("0114-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0114");
    jsonld_test_positive(remote_test_file_to_str("0115-in.jsonld"), remote_test_file_to_str("0115-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0115");
    jsonld_test_positive(remote_test_file_to_str("0116-in.jsonld"), remote_test_file_to_str("0116-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0116");
    jsonld_test_positive(remote_test_file_to_str("0117-in.jsonld"), remote_test_file_to_str("0117-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0117");
    // bn as predicate
    //jsonld_test_positive(remote_test_file_to_str("0118-in.jsonld"), remote_test_file_to_str("0118-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0118");
    jsonld_test_positive(remote_test_file_to_str("0119-in.jsonld"), remote_test_file_to_str("0119-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0119");
    jsonld_test_positive(remote_test_file_to_str("0120-in.jsonld"), remote_test_file_to_str("0120-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0120");
    jsonld_test_positive(remote_test_file_to_str("0121-in.jsonld"), remote_test_file_to_str("0121-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0121");
    jsonld_test_positive(remote_test_file_to_str("0122-in.jsonld"), remote_test_file_to_str("0122-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0122");
    jsonld_test_positive(remote_test_file_to_str("0123-in.jsonld"), remote_test_file_to_str("0123-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0123");
    jsonld_test_positive(remote_test_file_to_str("0124-in.jsonld"), remote_test_file_to_str("0124-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0124");
    jsonld_test_positive(remote_test_file_to_str("e124-in.jsonld"), remote_test_file_to_str("e124-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te124");
    jsonld_test_positive(remote_test_file_to_str("0125-in.jsonld"), remote_test_file_to_str("0125-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0125");
    jsonld_test_positive(remote_test_file_to_str("e125-in.jsonld"), remote_test_file_to_str("e125-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te125");
    jsonld_test_positive(remote_test_file_to_str("0126-in.jsonld"), remote_test_file_to_str("0126-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0126");
    jsonld_test_positive(remote_test_file_to_str("0127-in.jsonld"), remote_test_file_to_str("0127-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0127");
    jsonld_test_positive(remote_test_file_to_str("0128-in.jsonld"), remote_test_file_to_str("0128-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0128");
    jsonld_test_positive(remote_test_file_to_str("0129-in.jsonld"), remote_test_file_to_str("0129-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0129");
    jsonld_test_positive(remote_test_file_to_str("0130-in.jsonld"), remote_test_file_to_str("0130-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0130");
    jsonld_test_positive(remote_test_file_to_str("0131-in.jsonld"), remote_test_file_to_str("0131-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0131");
    jsonld_test_positive(remote_test_file_to_str("0132-in.jsonld"), remote_test_file_to_str("0132-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/t0132");
    jsonld_test_positive(remote_test_file_to_str("c001-in.jsonld"), remote_test_file_to_str("c001-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc001");
    jsonld_test_positive(remote_test_file_to_str("c002-in.jsonld"), remote_test_file_to_str("c002-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc002");
    jsonld_test_positive(remote_test_file_to_str("c003-in.jsonld"), remote_test_file_to_str("c003-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc003");
    jsonld_test_positive(remote_test_file_to_str("c004-in.jsonld"), remote_test_file_to_str("c004-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc004");
    jsonld_test_positive(remote_test_file_to_str("c005-in.jsonld"), remote_test_file_to_str("c005-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc005");
    jsonld_test_positive(remote_test_file_to_str("c006-in.jsonld"), remote_test_file_to_str("c006-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc006");
    jsonld_test_positive(remote_test_file_to_str("c007-in.jsonld"), remote_test_file_to_str("c007-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc007");
    jsonld_test_positive(remote_test_file_to_str("c008-in.jsonld"), remote_test_file_to_str("c008-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc008");
    jsonld_test_positive(remote_test_file_to_str("c009-in.jsonld"), remote_test_file_to_str("c009-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc009");
    jsonld_test_positive(remote_test_file_to_str("c010-in.jsonld"), remote_test_file_to_str("c010-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc010");
    jsonld_test_positive(remote_test_file_to_str("c011-in.jsonld"), remote_test_file_to_str("c011-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc011");
    jsonld_test_positive(remote_test_file_to_str("c012-in.jsonld"), remote_test_file_to_str("c012-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc012");
    jsonld_test_positive(remote_test_file_to_str("c013-in.jsonld"), remote_test_file_to_str("c013-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc013");
    jsonld_test_positive(remote_test_file_to_str("c014-in.jsonld"), remote_test_file_to_str("c014-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc014");
    jsonld_test_positive(remote_test_file_to_str("c015-in.jsonld"), remote_test_file_to_str("c015-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc015");
    jsonld_test_positive(remote_test_file_to_str("c016-in.jsonld"), remote_test_file_to_str("c016-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc016");
    jsonld_test_positive(remote_test_file_to_str("c017-in.jsonld"), remote_test_file_to_str("c017-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc017");
    jsonld_test_positive(remote_test_file_to_str("c018-in.jsonld"), remote_test_file_to_str("c018-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc018");
    jsonld_test_positive(remote_test_file_to_str("c019-in.jsonld"), remote_test_file_to_str("c019-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc019");
    jsonld_test_positive(remote_test_file_to_str("c020-in.jsonld"), remote_test_file_to_str("c020-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc020");
    jsonld_test_positive(remote_test_file_to_str("c021-in.jsonld"), remote_test_file_to_str("c021-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc021");
    jsonld_test_positive(remote_test_file_to_str("c022-in.jsonld"), remote_test_file_to_str("c022-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc022");
    jsonld_test_positive(remote_test_file_to_str("c023-in.jsonld"), remote_test_file_to_str("c023-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc023");
    jsonld_test_positive(remote_test_file_to_str("c024-in.jsonld"), remote_test_file_to_str("c024-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc024");
    jsonld_test_positive(remote_test_file_to_str("c025-in.jsonld"), remote_test_file_to_str("c025-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc025");
    jsonld_test_positive(remote_test_file_to_str("c026-in.jsonld"), remote_test_file_to_str("c026-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc026");
    jsonld_test_positive(remote_test_file_to_str("c027-in.jsonld"), remote_test_file_to_str("c027-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc027");
    jsonld_test_positive(remote_test_file_to_str("c028-in.jsonld"), remote_test_file_to_str("c028-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc028");
    // remote context
    //jsonld_test_positive(remote_test_file_to_str("c031-in.jsonld"), remote_test_file_to_str("c031-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc031");
    //jsonld_test_positive(remote_test_file_to_str("c034-in.jsonld"), remote_test_file_to_str("c034-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc034");
    jsonld_test_positive(remote_test_file_to_str("c035-in.jsonld"), remote_test_file_to_str("c035-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tc035");
    jsonld_test_positive(remote_test_file_to_str("di01-in.jsonld"), remote_test_file_to_str("di01-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi01");
    jsonld_test_positive(remote_test_file_to_str("di02-in.jsonld"), remote_test_file_to_str("di02-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi02");
    jsonld_test_positive(remote_test_file_to_str("di03-in.jsonld"), remote_test_file_to_str("di03-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi03");
    jsonld_test_positive(remote_test_file_to_str("di04-in.jsonld"), remote_test_file_to_str("di04-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi04");
    jsonld_test_positive(remote_test_file_to_str("di05-in.jsonld"), remote_test_file_to_str("di05-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi05");
    jsonld_test_positive(remote_test_file_to_str("di06-in.jsonld"), remote_test_file_to_str("di06-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi06");
    jsonld_test_positive(remote_test_file_to_str("di07-in.jsonld"), remote_test_file_to_str("di07-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi07");
    // TODO direction option
    // jsonld_test_positive(remote_test_file_to_str("di09-in.jsonld"), remote_test_file_to_str("di09-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi09");
    // jsonld_test_positive(remote_test_file_to_str("di10-in.jsonld"), remote_test_file_to_str("di10-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi10");
    // jsonld_test_positive(remote_test_file_to_str("di11-in.jsonld"), remote_test_file_to_str("di11-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi11");
    // jsonld_test_positive(remote_test_file_to_str("di12-in.jsonld"), remote_test_file_to_str("di12-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tdi12");
    jsonld_test_positive(remote_test_file_to_str("e001-in.jsonld"), remote_test_file_to_str("e001-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te001");
    jsonld_test_positive(remote_test_file_to_str("e002-in.jsonld"), remote_test_file_to_str("e002-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te002");
    jsonld_test_positive(remote_test_file_to_str("e003-in.jsonld"), remote_test_file_to_str("e003-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te003");
    jsonld_test_positive(remote_test_file_to_str("e004-in.jsonld"), remote_test_file_to_str("e004-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te004");
    jsonld_test_positive(remote_test_file_to_str("e005-in.jsonld"), remote_test_file_to_str("e005-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/e005-in.jsonld");
    jsonld_test_positive(remote_test_file_to_str("e006-in.jsonld"), remote_test_file_to_str("e006-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te006");
    jsonld_test_positive(remote_test_file_to_str("e007-in.jsonld"), remote_test_file_to_str("e007-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te007");
    jsonld_test_positive(remote_test_file_to_str("e008-in.jsonld"), remote_test_file_to_str("e008-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te008");
    jsonld_test_positive(remote_test_file_to_str("e009-in.jsonld"), remote_test_file_to_str("e009-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te009");
    jsonld_test_positive(remote_test_file_to_str("e010-in.jsonld"), remote_test_file_to_str("e010-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te010");
    jsonld_test_positive(remote_test_file_to_str("e011-in.jsonld"), remote_test_file_to_str("e011-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te011");
    jsonld_test_positive(remote_test_file_to_str("e012-in.jsonld"), remote_test_file_to_str("e012-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te012");
    jsonld_test_positive(remote_test_file_to_str("e013-in.jsonld"), remote_test_file_to_str("e013-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te013");
    // test looks to be broken ???
    //jsonld_test_positive(remote_test_file_to_str("e014-in.jsonld"), remote_test_file_to_str("e014-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te014");
    jsonld_test_positive(remote_test_file_to_str("e015-in.jsonld"), remote_test_file_to_str("e015-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te015");
    jsonld_test_positive(remote_test_file_to_str("e016-in.jsonld"), remote_test_file_to_str("e016-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te016");
    jsonld_test_positive(remote_test_file_to_str("e017-in.jsonld"), remote_test_file_to_str("e017-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te017");
    jsonld_test_positive(remote_test_file_to_str("e018-in.jsonld"), remote_test_file_to_str("e018-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te018");
    jsonld_test_positive(remote_test_file_to_str("e019-in.jsonld"), remote_test_file_to_str("e019-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te019");
    jsonld_test_positive(remote_test_file_to_str("e020-in.jsonld"), remote_test_file_to_str("e020-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te020");
    jsonld_test_positive(remote_test_file_to_str("e021-in.jsonld"), remote_test_file_to_str("e021-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te021");
    jsonld_test_positive(remote_test_file_to_str("e022-in.jsonld"), remote_test_file_to_str("e022-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te022");
    jsonld_test_positive(remote_test_file_to_str("e023-in.jsonld"), remote_test_file_to_str("e023-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te023");
    jsonld_test_positive(remote_test_file_to_str("e024-in.jsonld"), remote_test_file_to_str("e024-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te024");
    jsonld_test_positive(remote_test_file_to_str("e025-in.jsonld"), remote_test_file_to_str("e025-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te025");
    // this is the same as er43 (a negative test)?
    // jsonld_test_positive(remote_test_file_to_str("e026-in.jsonld"), remote_test_file_to_str("e026-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te026");
    jsonld_test_positive(remote_test_file_to_str("e027-in.jsonld"), remote_test_file_to_str("e027-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te027", true);
    jsonld_test_positive(remote_test_file_to_str("e028-in.jsonld"), remote_test_file_to_str("e028-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/te028");
    jsonld_test_positive(remote_test_file_to_str("e029-in.jsonld"), remote_test_file_to_str("e029-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/e029-in.jsonld");
    jsonld_test_positive(remote_test_file_to_str("e030-in.jsonld"), remote_test_file_to_str("e030-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te030");
    jsonld_test_positive(remote_test_file_to_str("e031-in.jsonld"), remote_test_file_to_str("e031-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te031");
    jsonld_test_positive(remote_test_file_to_str("e032-in.jsonld"), remote_test_file_to_str("e032-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te032");
    jsonld_test_positive(remote_test_file_to_str("e033-in.jsonld"), remote_test_file_to_str("e033-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te033");
    jsonld_test_positive(remote_test_file_to_str("e034-in.jsonld"), remote_test_file_to_str("e034-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te034");
    jsonld_test_positive(remote_test_file_to_str("e035-in.jsonld"), remote_test_file_to_str("e035-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te035");
    jsonld_test_positive(remote_test_file_to_str("e036-in.jsonld"), remote_test_file_to_str("e036-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te036");
    jsonld_test_positive(remote_test_file_to_str("e037-in.jsonld"), remote_test_file_to_str("e037-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te037");
    jsonld_test_positive(remote_test_file_to_str("e038-in.jsonld"), remote_test_file_to_str("e038-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te038");
    jsonld_test_positive(remote_test_file_to_str("e039-in.jsonld"), remote_test_file_to_str("e039-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te039");
    jsonld_test_positive(remote_test_file_to_str("e040-in.jsonld"), remote_test_file_to_str("e040-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/te040");
    jsonld_test_positive(remote_test_file_to_str("e041-in.jsonld"), remote_test_file_to_str("e041-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te041");
    jsonld_test_positive(remote_test_file_to_str("e042-in.jsonld"), remote_test_file_to_str("e042-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te042");
    jsonld_test_positive(remote_test_file_to_str("e043-in.jsonld"), remote_test_file_to_str("e043-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te043");
    jsonld_test_positive(remote_test_file_to_str("e044-in.jsonld"), remote_test_file_to_str("e044-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te044");
    jsonld_test_positive(remote_test_file_to_str("e045-in.jsonld"), remote_test_file_to_str("e045-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te045");
    jsonld_test_positive(remote_test_file_to_str("e046-in.jsonld"), remote_test_file_to_str("e046-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te046");
    jsonld_test_positive(remote_test_file_to_str("e047-in.jsonld"), remote_test_file_to_str("e047-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te047");
    jsonld_test_positive(remote_test_file_to_str("e048-in.jsonld"), remote_test_file_to_str("e048-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/te048");
    jsonld_test_positive(remote_test_file_to_str("e049-in.jsonld"), remote_test_file_to_str("e049-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te049");
    jsonld_test_positive(remote_test_file_to_str("e050-in.jsonld"), remote_test_file_to_str("e050-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te050");
    jsonld_test_positive(remote_test_file_to_str("e051-in.jsonld"), remote_test_file_to_str("e051-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te051");
    jsonld_test_positive(remote_test_file_to_str("e052-in.jsonld"), remote_test_file_to_str("e052-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te052");
    jsonld_test_positive(remote_test_file_to_str("e053-in.jsonld"), remote_test_file_to_str("e053-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te053");
    jsonld_test_positive(remote_test_file_to_str("e054-in.jsonld"), remote_test_file_to_str("e054-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te054");
    jsonld_test_positive(remote_test_file_to_str("e055-in.jsonld"), remote_test_file_to_str("e055-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te055");
    jsonld_test_positive(remote_test_file_to_str("e056-in.jsonld"), remote_test_file_to_str("e056-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/te056");
    jsonld_test_positive(remote_test_file_to_str("e057-in.jsonld"), remote_test_file_to_str("e057-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/te057");
    jsonld_test_positive(remote_test_file_to_str("e058-in.jsonld"), remote_test_file_to_str("e058-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te058");
    jsonld_test_positive(remote_test_file_to_str("e059-in.jsonld"), remote_test_file_to_str("e059-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/te059");
    jsonld_test_positive(remote_test_file_to_str("e060-in.jsonld"), remote_test_file_to_str("e060-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/e060-in.jsonld", true);
    jsonld_test_positive(remote_test_file_to_str("e061-in.jsonld"), remote_test_file_to_str("e061-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te061");
    jsonld_test_positive(remote_test_file_to_str("e062-in.jsonld"), remote_test_file_to_str("e062-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te062");
    jsonld_test_positive(remote_test_file_to_str("e063-in.jsonld"), remote_test_file_to_str("e063-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te063");
    jsonld_test_positive(remote_test_file_to_str("e064-in.jsonld"), remote_test_file_to_str("e064-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te064");
    jsonld_test_positive(remote_test_file_to_str("e065-in.jsonld"), remote_test_file_to_str("e065-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te065");
    jsonld_test_positive(remote_test_file_to_str("e066-in.jsonld"), remote_test_file_to_str("e066-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/te066");
    jsonld_test_positive(remote_test_file_to_str("e067-in.jsonld"), remote_test_file_to_str("e067-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te067");
    jsonld_test_positive(remote_test_file_to_str("e068-in.jsonld"), remote_test_file_to_str("e068-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te068");
    jsonld_test_positive(remote_test_file_to_str("e069-in.jsonld"), remote_test_file_to_str("e069-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te069");
    jsonld_test_positive(remote_test_file_to_str("e070-in.jsonld"), remote_test_file_to_str("e070-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te070");
    // same as er44 (negative test)
    // jsonld_test_positive(remote_test_file_to_str("e071-in.jsonld"), remote_test_file_to_str("e071-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te071");
    jsonld_test_positive(remote_test_file_to_str("e072-in.jsonld"), remote_test_file_to_str("e072-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te072");
    jsonld_test_positive(remote_test_file_to_str("e073-in.jsonld"), remote_test_file_to_str("e073-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te073");
    jsonld_test_positive(remote_test_file_to_str("e074-in.jsonld"), remote_test_file_to_str("e074-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te074");
    // bn vocab is deprecated
    //jsonld_test_positive(remote_test_file_to_str("e075-in.jsonld"), remote_test_file_to_str("e075-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te075");
    jsonld_test_positive(remote_test_file_to_str("e076-in.jsonld"), remote_test_file_to_str("e076-out.nq"), "http://example/base/");
    // some sort of external context ...
    //jsonld_test_positive(remote_test_file_to_str("e077-in.jsonld"), remote_test_file_to_str("e077-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te077");
    jsonld_test_positive(remote_test_file_to_str("e078-in.jsonld"), remote_test_file_to_str("e078-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/stream-toRdf/e078-in.jsonld");
    jsonld_test_positive(remote_test_file_to_str("e079-in.jsonld"), remote_test_file_to_str("e079-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te079");
    jsonld_test_positive(remote_test_file_to_str("e080-in.jsonld"), remote_test_file_to_str("e080-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te080");
    jsonld_test_positive(remote_test_file_to_str("e081-in.jsonld"), remote_test_file_to_str("e081-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te081");
    jsonld_test_positive(remote_test_file_to_str("e082-in.jsonld"), remote_test_file_to_str("e082-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te082");
    jsonld_test_positive(remote_test_file_to_str("e083-in.jsonld"), remote_test_file_to_str("e083-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te083");
    jsonld_test_positive(remote_test_file_to_str("e084-in.jsonld"), remote_test_file_to_str("e084-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te084");
    jsonld_test_positive(remote_test_file_to_str("e085-in.jsonld"), remote_test_file_to_str("e085-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te085");
    jsonld_test_positive(remote_test_file_to_str("e086-in.jsonld"), remote_test_file_to_str("e086-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te086");
    jsonld_test_positive(remote_test_file_to_str("e087-in.jsonld"), remote_test_file_to_str("e087-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te087");
    jsonld_test_positive(remote_test_file_to_str("e088-in.jsonld"), remote_test_file_to_str("e088-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te088");
    jsonld_test_positive(remote_test_file_to_str("e089-in.jsonld"), remote_test_file_to_str("e089-out.nq"), "http://example/base/");
    jsonld_test_positive(remote_test_file_to_str("e090-in.jsonld"), remote_test_file_to_str("e090-out.nq"), "http://example/base/");
    jsonld_test_positive(remote_test_file_to_str("e091-in.jsonld"), remote_test_file_to_str("e091-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te091");
    jsonld_test_positive(remote_test_file_to_str("e092-in.jsonld"), remote_test_file_to_str("e092-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te092");
    jsonld_test_positive(remote_test_file_to_str("e093-in.jsonld"), remote_test_file_to_str("e093-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te093");
    jsonld_test_positive(remote_test_file_to_str("e094-in.jsonld"), remote_test_file_to_str("e094-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te094");
    jsonld_test_positive(remote_test_file_to_str("e095-in.jsonld"), remote_test_file_to_str("e095-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te095");
    jsonld_test_positive(remote_test_file_to_str("e096-in.jsonld"), remote_test_file_to_str("e096-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te096");
    jsonld_test_positive(remote_test_file_to_str("e097-in.jsonld"), remote_test_file_to_str("e097-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te097");
    jsonld_test_positive(remote_test_file_to_str("e098-in.jsonld"), remote_test_file_to_str("e098-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te098");
    jsonld_test_positive(remote_test_file_to_str("e099-in.jsonld"), remote_test_file_to_str("e099-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te099");
    jsonld_test_positive(remote_test_file_to_str("e100-in.jsonld"), remote_test_file_to_str("e100-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te100");
    jsonld_test_positive(remote_test_file_to_str("e101-in.jsonld"), remote_test_file_to_str("e101-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te101");
    jsonld_test_positive(remote_test_file_to_str("e102-in.jsonld"), remote_test_file_to_str("e102-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te102");
    jsonld_test_positive(remote_test_file_to_str("e103-in.jsonld"), remote_test_file_to_str("e103-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te103");
    jsonld_test_positive(remote_test_file_to_str("e104-in.jsonld"), remote_test_file_to_str("e104-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te104");
    jsonld_test_positive(remote_test_file_to_str("e105-in.jsonld"), remote_test_file_to_str("e105-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te105");
    jsonld_test_positive(remote_test_file_to_str("e106-in.jsonld"), remote_test_file_to_str("e106-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te106");
    jsonld_test_positive(remote_test_file_to_str("e107-in.jsonld"), remote_test_file_to_str("e107-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te107");
    jsonld_test_positive(remote_test_file_to_str("e108-in.jsonld"), remote_test_file_to_str("e108-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te108", true);
    // invalid IRI
    //jsonld_test_positive(remote_test_file_to_str("e109-in.jsonld"), remote_test_file_to_str("e109-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te109");
    jsonld_test_positive(remote_test_file_to_str("e110-in.jsonld"), remote_test_file_to_str("e110-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te110");
    // broken vocab iri
    //jsonld_test_positive(remote_test_file_to_str("e111-in.jsonld"), remote_test_file_to_str("e111-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te111");
    //jsonld_test_positive(remote_test_file_to_str("e112-in.jsonld"), remote_test_file_to_str("e112-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te112");
    jsonld_test_positive(remote_test_file_to_str("e113-in.jsonld"), remote_test_file_to_str("e113-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te113");
    jsonld_test_positive(remote_test_file_to_str("e114-in.jsonld"), remote_test_file_to_str("e114-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te114");
    jsonld_test_positive(remote_test_file_to_str("e117-in.jsonld"), remote_test_file_to_str("e117-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te117");
    jsonld_test_positive(remote_test_file_to_str("e118-in.jsonld"), remote_test_file_to_str("e118-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te118");
    jsonld_test_positive(remote_test_file_to_str("e119-in.jsonld"), remote_test_file_to_str("e119-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te119");
    // looks like keyword as id
    //jsonld_test_positive(remote_test_file_to_str("e120-in.jsonld"), remote_test_file_to_str("e120-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te120");
    jsonld_test_positive(remote_test_file_to_str("e121-in.jsonld"), remote_test_file_to_str("e121-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te121");
    jsonld_test_positive(remote_test_file_to_str("e122-in.jsonld"), remote_test_file_to_str("e122-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te122");
    // remote context
    //jsonld_test_positive(remote_test_file_to_str("e126-in.jsonld"), remote_test_file_to_str("e126-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te126");
    //jsonld_test_positive(remote_test_file_to_str("e127-in.jsonld"), remote_test_file_to_str("e127-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te127");
    //jsonld_test_positive(remote_test_file_to_str("e128-in.jsonld"), remote_test_file_to_str("e128-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te128");
    jsonld_test_positive(remote_test_file_to_str("e129-in.jsonld"), remote_test_file_to_str("e129-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te129");
    jsonld_test_positive(remote_test_file_to_str("e130-in.jsonld"), remote_test_file_to_str("e130-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/te130");
    jsonld_test_positive(remote_test_file_to_str("in01-in.jsonld"), remote_test_file_to_str("in01-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tin01");
    jsonld_test_positive(remote_test_file_to_str("in02-in.jsonld"), remote_test_file_to_str("in02-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tin02");
    jsonld_test_positive(remote_test_file_to_str("in03-in.jsonld"), remote_test_file_to_str("in03-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tin03");
    jsonld_test_positive(remote_test_file_to_str("in04-in.jsonld"), remote_test_file_to_str("in04-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tin04");
    jsonld_test_positive(remote_test_file_to_str("in05-in.jsonld"), remote_test_file_to_str("in05-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tin05");
    jsonld_test_positive(remote_test_file_to_str("in06-in.jsonld"), remote_test_file_to_str("in06-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tin06", true);
    jsonld_test_positive(remote_test_file_to_str("js01-in.jsonld"), remote_test_file_to_str("js01-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs01");
    jsonld_test_positive(remote_test_file_to_str("js02-in.jsonld"), remote_test_file_to_str("js02-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs02");
    jsonld_test_positive(remote_test_file_to_str("js03-in.jsonld"), remote_test_file_to_str("js03-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs03");
    jsonld_test_positive(remote_test_file_to_str("js04-in.jsonld"), remote_test_file_to_str("js04-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs04");
    jsonld_test_positive(remote_test_file_to_str("js05-in.jsonld"), remote_test_file_to_str("js05-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs05");
    // TODO? canonization
    jsonld_test_positive(remote_test_file_to_str("js06-in.jsonld"), R"(_:bn_0 <http://example.org/vocab#object> "{\"foo\": \"bar\"}"^^<http://www.w3.org/1999/02/22-rdf-syntax-ns#JSON> .)", "https://w3c.github.io/json-ld-streaming/tests/tjs06");
    jsonld_test_positive(remote_test_file_to_str("js07-in.jsonld"), R"(_:bn_0 <http://example.org/vocab#array> "[{\"foo\": \"bar\"}]"^^<http://www.w3.org/1999/02/22-rdf-syntax-ns#JSON> .)", "https://w3c.github.io/json-ld-streaming/tests/tjs07");
    jsonld_test_positive(remote_test_file_to_str("js15-in.jsonld"), R"(_:bn_0 <http://example.org/vocab#object> "{\"foo\": \"bar\"}"^^<http://www.w3.org/1999/02/22-rdf-syntax-ns#JSON> .)", "https://w3c.github.io/json-ld-streaming/tests/tjs15");
    jsonld_test_positive(remote_test_file_to_str("js16-in.jsonld"), R"(_:bn_0 <http://example.org/vocab#object> "{\"foo\": \"bar\"}"^^<http://www.w3.org/1999/02/22-rdf-syntax-ns#JSON> .)", "https://w3c.github.io/json-ld-streaming/tests/tjs16");
    // jsonld_test_positive(remote_test_file_to_str("js06-in.jsonld"), remote_test_file_to_str("js06-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs06");
    // jsonld_test_positive(remote_test_file_to_str("js07-in.jsonld"), remote_test_file_to_str("js07-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs07");
    // jsonld_test_positive(remote_test_file_to_str("js08-in.jsonld"), remote_test_file_to_str("js08-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs08");
    // jsonld_test_positive(remote_test_file_to_str("js09-in.jsonld"), remote_test_file_to_str("js09-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs09");
    // jsonld_test_positive(remote_test_file_to_str("js10-in.jsonld"), remote_test_file_to_str("js10-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs10");
    // jsonld_test_positive(remote_test_file_to_str("js11-in.jsonld"), remote_test_file_to_str("js11-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs11");
    // jsonld_test_positive(remote_test_file_to_str("js12-in.jsonld"), remote_test_file_to_str("js12-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs12");
    // jsonld_test_positive(remote_test_file_to_str("js13-in.jsonld"), remote_test_file_to_str("js13-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs13");
    // jsonld_test_positive(remote_test_file_to_str("js14-in.jsonld"), remote_test_file_to_str("js14-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs14");
    // jsonld_test_positive(remote_test_file_to_str("js15-in.jsonld"), remote_test_file_to_str("js15-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs15");
    // jsonld_test_positive(remote_test_file_to_str("js16-in.jsonld"), remote_test_file_to_str("js16-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs16");
    jsonld_test_positive(remote_test_file_to_str("js17-in.jsonld"), remote_test_file_to_str("js17-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs17");
    jsonld_test_positive(remote_test_file_to_str("js18-in.jsonld"), remote_test_file_to_str("js18-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs18");
    // jsonld_test_positive(remote_test_file_to_str("js19-in.jsonld"), remote_test_file_to_str("js19-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs19");
    // jsonld_test_positive(remote_test_file_to_str("js20-in.jsonld"), remote_test_file_to_str("js20-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs20");
    // jsonld_test_positive(remote_test_file_to_str("js21-in.jsonld"), remote_test_file_to_str("js21-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs21");
    // jsonld_test_positive(remote_test_file_to_str("js22-in.jsonld"), remote_test_file_to_str("js22-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs22");
    // jsonld_test_positive(remote_test_file_to_str("js23-in.jsonld"), remote_test_file_to_str("js23-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tjs23");
    jsonld_test_positive(remote_test_file_to_str("li01-in.jsonld"), remote_test_file_to_str("li01-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli01");
    jsonld_test_positive(remote_test_file_to_str("li02-in.jsonld"), remote_test_file_to_str("li02-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli02");
    jsonld_test_positive(remote_test_file_to_str("li03-in.jsonld"), remote_test_file_to_str("li03-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli03");
    jsonld_test_positive(remote_test_file_to_str("li04-in.jsonld"), remote_test_file_to_str("li04-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli04");
    jsonld_test_positive(remote_test_file_to_str("li05-in.jsonld"), remote_test_file_to_str("li05-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli05");
    jsonld_test_positive(remote_test_file_to_str("li06-in.jsonld"), remote_test_file_to_str("li06-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli06");
    jsonld_test_positive(remote_test_file_to_str("li07-in.jsonld"), remote_test_file_to_str("li07-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli07");
    jsonld_test_positive(remote_test_file_to_str("li08-in.jsonld"), remote_test_file_to_str("li08-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli08");
    jsonld_test_positive(remote_test_file_to_str("li09-in.jsonld"), remote_test_file_to_str("li09-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli09");
    jsonld_test_positive(remote_test_file_to_str("li10-in.jsonld"), remote_test_file_to_str("li10-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tli10");
    jsonld_test_positive(remote_test_file_to_str("m001-in.jsonld"), remote_test_file_to_str("m001-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm001");
    jsonld_test_positive(remote_test_file_to_str("m002-in.jsonld"), remote_test_file_to_str("m002-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm002");
    jsonld_test_positive(remote_test_file_to_str("m003-in.jsonld"), remote_test_file_to_str("m003-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm003");
    jsonld_test_positive(remote_test_file_to_str("m004-in.jsonld"), remote_test_file_to_str("m004-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm004");
    jsonld_test_positive(remote_test_file_to_str("m005-in.jsonld"), remote_test_file_to_str("m005-out.nq"), "http://example.org/");
    jsonld_test_positive(remote_test_file_to_str("m006-in.jsonld"), remote_test_file_to_str("m006-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm006");
    jsonld_test_positive(remote_test_file_to_str("m007-in.jsonld"), remote_test_file_to_str("m007-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm007");
    jsonld_test_positive(remote_test_file_to_str("m008-in.jsonld"), remote_test_file_to_str("m008-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm008");
    jsonld_test_positive(remote_test_file_to_str("m009-in.jsonld"), remote_test_file_to_str("m009-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm009");
    jsonld_test_positive(remote_test_file_to_str("m010-in.jsonld"), remote_test_file_to_str("m010-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm010");
    jsonld_test_positive(remote_test_file_to_str("m011-in.jsonld"), remote_test_file_to_str("m011-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm011");
    jsonld_test_positive(remote_test_file_to_str("m012-in.jsonld"), remote_test_file_to_str("m012-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm012");
    jsonld_test_positive(remote_test_file_to_str("m013-in.jsonld"), remote_test_file_to_str("m013-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm013");
    jsonld_test_positive(remote_test_file_to_str("m014-in.jsonld"), remote_test_file_to_str("m014-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm014");
    jsonld_test_positive(remote_test_file_to_str("m015-in.jsonld"), remote_test_file_to_str("m015-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm015");
    jsonld_test_positive(remote_test_file_to_str("m016-in.jsonld"), remote_test_file_to_str("m016-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm016");
    jsonld_test_positive(remote_test_file_to_str("m017-in.jsonld"), remote_test_file_to_str("m017-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm017");
    jsonld_test_positive(remote_test_file_to_str("m018-in.jsonld"), remote_test_file_to_str("m018-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm018");
    jsonld_test_positive(remote_test_file_to_str("m019-in.jsonld"), remote_test_file_to_str("m019-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tm019");
    jsonld_test_positive(remote_test_file_to_str("n001-in.jsonld"), remote_test_file_to_str("n001-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tn001");
    jsonld_test_positive(remote_test_file_to_str("n002-in.jsonld"), remote_test_file_to_str("n002-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tn002");
    jsonld_test_positive(remote_test_file_to_str("n003-in.jsonld"), remote_test_file_to_str("n003-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tn003");
    jsonld_test_positive(remote_test_file_to_str("n004-in.jsonld"), remote_test_file_to_str("n004-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tn004");
    jsonld_test_positive(remote_test_file_to_str("n005-in.jsonld"), remote_test_file_to_str("n005-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tn005");
    jsonld_test_positive(remote_test_file_to_str("n006-in.jsonld"), remote_test_file_to_str("n006-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tn006");
    jsonld_test_positive(remote_test_file_to_str("n007-in.jsonld"), remote_test_file_to_str("n007-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tn007");
    jsonld_test_positive(remote_test_file_to_str("n008-in.jsonld"), remote_test_file_to_str("n008-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tn008");
    jsonld_test_positive(remote_test_file_to_str("p001-in.jsonld"), remote_test_file_to_str("p001-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tp001");
    jsonld_test_positive(remote_test_file_to_str("p002-in.jsonld"), remote_test_file_to_str("p002-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tp002");
    jsonld_test_positive(remote_test_file_to_str("p003-in.jsonld"), remote_test_file_to_str("p003-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tp003");
    jsonld_test_positive(remote_test_file_to_str("p004-in.jsonld"), remote_test_file_to_str("p004-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tp004");
    jsonld_test_positive(remote_test_file_to_str("pi06-in.jsonld"), remote_test_file_to_str("pi06-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpi06");
    jsonld_test_positive(remote_test_file_to_str("pi07-in.jsonld"), remote_test_file_to_str("pi07-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpi07");
    jsonld_test_positive(remote_test_file_to_str("pi08-in.jsonld"), remote_test_file_to_str("pi08-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpi08");
    jsonld_test_positive(remote_test_file_to_str("pi09-in.jsonld"), remote_test_file_to_str("pi09-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpi09");
    jsonld_test_positive(remote_test_file_to_str("pi10-in.jsonld"), remote_test_file_to_str("pi10-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpi10");
    jsonld_test_positive(remote_test_file_to_str("pi11-in.jsonld"), remote_test_file_to_str("pi11-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpi11");
    jsonld_test_positive(remote_test_file_to_str("pr02-in.jsonld"), remote_test_file_to_str("pr02-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr02");
    jsonld_test_positive(remote_test_file_to_str("pr06-in.jsonld"), remote_test_file_to_str("pr06-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr06");
    jsonld_test_positive(remote_test_file_to_str("pr10-in.jsonld"), remote_test_file_to_str("pr10-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr10");
    jsonld_test_positive(remote_test_file_to_str("pr13-in.jsonld"), remote_test_file_to_str("pr13-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr13");
    jsonld_test_positive(remote_test_file_to_str("pr14-in.jsonld"), remote_test_file_to_str("pr14-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr14");
    jsonld_test_positive(remote_test_file_to_str("pr15-in.jsonld"), remote_test_file_to_str("pr15-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr15");
    jsonld_test_positive(remote_test_file_to_str("pr16-in.jsonld"), remote_test_file_to_str("pr16-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr16");
    jsonld_test_positive(remote_test_file_to_str("pr19-in.jsonld"), remote_test_file_to_str("pr19-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr19");
    jsonld_test_positive(remote_test_file_to_str("pr22-in.jsonld"), remote_test_file_to_str("pr22-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr22");
    jsonld_test_positive(remote_test_file_to_str("pr23-in.jsonld"), remote_test_file_to_str("pr23-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr23");
    jsonld_test_positive(remote_test_file_to_str("pr24-in.jsonld"), remote_test_file_to_str("pr24-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr24");
    jsonld_test_positive(remote_test_file_to_str("pr25-in.jsonld"), remote_test_file_to_str("pr25-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr25");
    jsonld_test_positive(remote_test_file_to_str("pr27-in.jsonld"), remote_test_file_to_str("pr27-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr27");
    jsonld_test_positive(remote_test_file_to_str("pr29-in.jsonld"), remote_test_file_to_str("pr29-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr29");
    jsonld_test_positive(remote_test_file_to_str("pr30-in.jsonld"), remote_test_file_to_str("pr30-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr30");
    jsonld_test_positive(remote_test_file_to_str("pr34-in.jsonld"), remote_test_file_to_str("pr34-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr34");
    jsonld_test_positive(remote_test_file_to_str("pr35-in.jsonld"), remote_test_file_to_str("pr35-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr35");
    // according to spec, this should produce invalid iri mapping (expand iri returns null, because @ignoreMe looks like a keyword, which is not iri, bn or keyword
    // jsonld_test_positive(remote_test_file_to_str("pr36-in.jsonld"), remote_test_file_to_str("pr36-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr36");
    // jsonld_test_positive(remote_test_file_to_str("pr37-in.jsonld"), remote_test_file_to_str("pr37-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr37");
    // jsonld_test_positive(remote_test_file_to_str("pr38-in.jsonld"), remote_test_file_to_str("pr38-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr38");
    // jsonld_test_positive(remote_test_file_to_str("pr39-in.jsonld"), remote_test_file_to_str("pr39-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr39");
    jsonld_test_positive(remote_test_file_to_str("pr40-in.jsonld"), remote_test_file_to_str("pr40-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tpr40");
    jsonld_test_positive(remote_test_file_to_str("rt01-in.jsonld"), remote_test_file_to_str("rt01-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/trt01");
    //import
    // jsonld_test_positive(remote_test_file_to_str("so05-in.jsonld"), remote_test_file_to_str("so05-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tso05");
    // jsonld_test_positive(remote_test_file_to_str("so06-in.jsonld"), remote_test_file_to_str("so06-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tso06");
    // jsonld_test_positive(remote_test_file_to_str("so08-in.jsonld"), remote_test_file_to_str("so08-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tso08");
    // jsonld_test_positive(remote_test_file_to_str("so09-in.jsonld"), remote_test_file_to_str("so09-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tso09");
    // jsonld_test_positive(remote_test_file_to_str("so11-in.jsonld"), remote_test_file_to_str("so11-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tso11");
    jsonld_test_positive(remote_test_file_to_str("tn02-in.jsonld"), remote_test_file_to_str("tn02-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/ttn02", true);
    jsonld_test_positive(remote_test_file_to_str("v001-in.jsonld"), remote_test_file_to_str("v001-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv001");
    jsonld_test_positive(remote_test_file_to_str("v002-in.jsonld"), remote_test_file_to_str("v002-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv002");
    jsonld_test_positive(remote_test_file_to_str("v003-in.jsonld"), remote_test_file_to_str("v003-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv003");
    jsonld_test_positive(remote_test_file_to_str("v004-in.jsonld"), remote_test_file_to_str("v004-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv004");
    jsonld_test_positive(remote_test_file_to_str("v005-in.jsonld"), remote_test_file_to_str("v005-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv005");
    jsonld_test_positive(remote_test_file_to_str("v006-in.jsonld"), remote_test_file_to_str("v006-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv006");
    jsonld_test_positive(remote_test_file_to_str("v008-in.jsonld"), remote_test_file_to_str("v008-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv008");
    jsonld_test_positive(remote_test_file_to_str("v009-in.jsonld"), remote_test_file_to_str("v009-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv009");
    jsonld_test_positive(remote_test_file_to_str("v010-in.jsonld"), remote_test_file_to_str("v010-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv010");
    jsonld_test_positive(remote_test_file_to_str("v011-in.jsonld"), remote_test_file_to_str("v011-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv011");
    jsonld_test_positive(remote_test_file_to_str("v012-in.jsonld"), remote_test_file_to_str("v012-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv012");
    jsonld_test_positive(remote_test_file_to_str("v013-in.jsonld"), remote_test_file_to_str("v013-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv013");
    jsonld_test_positive(remote_test_file_to_str("v014-in.jsonld"), remote_test_file_to_str("v014-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv014");
    jsonld_test_positive(remote_test_file_to_str("v015-in.jsonld"), remote_test_file_to_str("v015-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv015");
    jsonld_test_positive(remote_test_file_to_str("v016-in.jsonld"), remote_test_file_to_str("v016-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv016");
    jsonld_test_positive(remote_test_file_to_str("v017-in.jsonld"), remote_test_file_to_str("v017-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv017");
    jsonld_test_positive(remote_test_file_to_str("v018-in.jsonld"), remote_test_file_to_str("v018-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv018");
    jsonld_test_positive(remote_test_file_to_str("v019-in.jsonld"), remote_test_file_to_str("v019-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv019");
    jsonld_test_positive(remote_test_file_to_str("v020-in.jsonld"), remote_test_file_to_str("v020-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv020");
    jsonld_test_positive(remote_test_file_to_str("v021-in.jsonld"), remote_test_file_to_str("v021-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv021");
    jsonld_test_positive(remote_test_file_to_str("v022-in.jsonld"), remote_test_file_to_str("v022-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/tv022");
    // invalid iri
    // jsonld_test_positive(remote_test_file_to_str("wf01-in.jsonld"), remote_test_file_to_str("wf01-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/twf01");
    // jsonld_test_positive(remote_test_file_to_str("wf02-in.jsonld"), remote_test_file_to_str("wf02-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/twf02");
    // jsonld_test_positive(remote_test_file_to_str("wf03-in.jsonld"), remote_test_file_to_str("wf03-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/twf03");
    // jsonld_test_positive(remote_test_file_to_str("wf04-in.jsonld"), remote_test_file_to_str("wf04-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/twf04");
    // jsonld_test_positive(remote_test_file_to_str("wf05-in.jsonld"), remote_test_file_to_str("wf05-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/twf05");
    // jsonld_test_positive(remote_test_file_to_str("wf07-in.jsonld"), remote_test_file_to_str("wf07-out.nq"), "https://w3c.github.io/json-ld-streaming/tests/twf07");
    // negative tests
    //as far as i can tell, this contains no errors
    // jsonld_test_negative(remote_test_file_to_str("c029-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tc029");
    jsonld_test_negative(remote_test_file_to_str("c030-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tc030");
    jsonld_test_negative(remote_test_file_to_str("c032-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tc032");
    jsonld_test_negative(remote_test_file_to_str("c033-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tc033");
    jsonld_test_negative(remote_test_file_to_str("di08-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tdi08");
    //jsonld_test_negative(remote_test_file_to_str("e115-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/te115");
    //jsonld_test_negative(remote_test_file_to_str("e116-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/te116");
    jsonld_test_negative(remote_test_file_to_str("e123-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/te123");
    jsonld_test_negative(remote_test_file_to_str("ec01-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tec01");
    jsonld_test_negative(remote_test_file_to_str("ec02-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tec02");
    jsonld_test_negative(remote_test_file_to_str("em01-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tem01");
    jsonld_test_negative(remote_test_file_to_str("en01-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ten01");
    jsonld_test_negative(remote_test_file_to_str("en02-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ten02");
    jsonld_test_negative(remote_test_file_to_str("en03-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ten03");
    jsonld_test_negative(remote_test_file_to_str("en04-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ten04");
    jsonld_test_negative(remote_test_file_to_str("en05-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ten05");
    jsonld_test_negative(remote_test_file_to_str("en06-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ten06");
    //jsonld_test_negative(remote_test_file_to_str("ep02-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tep02");
    jsonld_test_negative(remote_test_file_to_str("ep03-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tep03");
    jsonld_test_negative(remote_test_file_to_str("er01-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter01");
    jsonld_test_negative(remote_test_file_to_str("er02-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter02");
    jsonld_test_negative(remote_test_file_to_str("er03-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter03");
    jsonld_test_negative(remote_test_file_to_str("er04-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter04");
    jsonld_test_negative(remote_test_file_to_str("er05-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter05");
    jsonld_test_negative(remote_test_file_to_str("er06-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter06");
    jsonld_test_negative(remote_test_file_to_str("er07-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter07");
    jsonld_test_negative(remote_test_file_to_str("er08-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter08");
    jsonld_test_negative(remote_test_file_to_str("er09-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter09");
    jsonld_test_negative(remote_test_file_to_str("er10-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter10");
    jsonld_test_negative(remote_test_file_to_str("er11-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter11");
    jsonld_test_negative(remote_test_file_to_str("er12-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter12");
    jsonld_test_negative(remote_test_file_to_str("er13-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter13");
    jsonld_test_negative(remote_test_file_to_str("er14-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter14");
    jsonld_test_negative(remote_test_file_to_str("er15-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter15");
    jsonld_test_negative(remote_test_file_to_str("er17-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter17");
    jsonld_test_negative(remote_test_file_to_str("er18-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter18");
    jsonld_test_negative(remote_test_file_to_str("er19-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter19");
    jsonld_test_negative(remote_test_file_to_str("er20-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter20");
    // jsonld_test_negative(remote_test_file_to_str("er21-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter21");
    jsonld_test_negative(remote_test_file_to_str("er22-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter22");
    jsonld_test_negative(remote_test_file_to_str("er23-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter23");
    // jsonld_test_negative(remote_test_file_to_str("er24-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter24");
    jsonld_test_negative(remote_test_file_to_str("er25-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter25");
    jsonld_test_negative(remote_test_file_to_str("er26-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter26");
    jsonld_test_negative(remote_test_file_to_str("er27-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter27");
    jsonld_test_negative(remote_test_file_to_str("er28-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter28");
    jsonld_test_negative(remote_test_file_to_str("er29-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter29");
    jsonld_test_negative(remote_test_file_to_str("er30-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter30");
    jsonld_test_negative(remote_test_file_to_str("er31-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter31");
    // jsonld_test_negative(remote_test_file_to_str("er32-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter32");
    jsonld_test_negative(remote_test_file_to_str("er33-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter33");
    jsonld_test_negative(remote_test_file_to_str("er34-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter34");
    jsonld_test_negative(remote_test_file_to_str("er35-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter35");
    jsonld_test_negative(remote_test_file_to_str("er36-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter36");
    jsonld_test_negative(remote_test_file_to_str("er37-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter37");
    jsonld_test_negative(remote_test_file_to_str("er38-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter38");
    jsonld_test_negative(remote_test_file_to_str("er39-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter39");
    jsonld_test_negative(remote_test_file_to_str("er40-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter40");
    jsonld_test_negative(remote_test_file_to_str("er41-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter41");
    // jsonld_test_negative(remote_test_file_to_str("er42-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter42");
    jsonld_test_negative(remote_test_file_to_str("er43-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter43");
    jsonld_test_negative(remote_test_file_to_str("er44-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter44");
    jsonld_test_negative(remote_test_file_to_str("er48-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter48");
    jsonld_test_negative(remote_test_file_to_str("er49-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter49");
    jsonld_test_negative(remote_test_file_to_str("er50-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter50");
    jsonld_test_negative(remote_test_file_to_str("er51-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter51");
    jsonld_test_negative(remote_test_file_to_str("er52-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter52");
    jsonld_test_negative(remote_test_file_to_str("er53-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ter53");
    jsonld_test_negative(remote_test_file_to_str("in07-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tin07");
    jsonld_test_negative(remote_test_file_to_str("in08-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tin08");
    jsonld_test_negative(remote_test_file_to_str("in09-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tin09");
    jsonld_test_negative(remote_test_file_to_str("m020-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tm020");
    jsonld_test_negative(remote_test_file_to_str("pi01-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpi01");
    jsonld_test_negative(remote_test_file_to_str("pi02-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpi02");
    jsonld_test_negative(remote_test_file_to_str("pi03-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpi03");
    jsonld_test_negative(remote_test_file_to_str("pi04-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpi04");
    jsonld_test_negative(remote_test_file_to_str("pi05-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpi05");
    jsonld_test_negative(remote_test_file_to_str("pr01-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr01");
    jsonld_test_negative(remote_test_file_to_str("pr03-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr03");
    jsonld_test_negative(remote_test_file_to_str("pr04-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr04");
    jsonld_test_negative(remote_test_file_to_str("pr05-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr05");
    jsonld_test_negative(remote_test_file_to_str("pr08-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr08");
    jsonld_test_negative(remote_test_file_to_str("pr09-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr09");
    jsonld_test_negative(remote_test_file_to_str("pr11-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr11");
    jsonld_test_negative(remote_test_file_to_str("pr12-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr12");
    jsonld_test_negative(remote_test_file_to_str("pr17-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr17");
    jsonld_test_negative(remote_test_file_to_str("pr18-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr18");
    jsonld_test_negative(remote_test_file_to_str("pr20-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr20");
    jsonld_test_negative(remote_test_file_to_str("pr21-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr21");
    jsonld_test_negative(remote_test_file_to_str("pr26-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr26");
    jsonld_test_negative(remote_test_file_to_str("pr28-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr28");
    jsonld_test_negative(remote_test_file_to_str("pr31-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr31");
    jsonld_test_negative(remote_test_file_to_str("pr32-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr32");
    jsonld_test_negative(remote_test_file_to_str("pr33-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tpr33");
    // jsonld_test_negative(remote_test_file_to_str("se01-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tse01");
    // jsonld_test_negative(remote_test_file_to_str("se02-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tse02");
    // jsonld_test_negative(remote_test_file_to_str("se03-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tse03");
    // jsonld_test_negative(remote_test_file_to_str("se04-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tse04");
    // jsonld_test_negative(remote_test_file_to_str("se05-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tse05");
    // jsonld_test_negative(remote_test_file_to_str("se06-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tse06");
    // jsonld_test_negative(remote_test_file_to_str("se07-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tse07");
    // jsonld_test_negative(remote_test_file_to_str("se08-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tse08");
    // jsonld_test_negative(remote_test_file_to_str("se09-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tse09");
    jsonld_test_negative(remote_test_file_to_str("so01-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tso01");
    jsonld_test_negative(remote_test_file_to_str("so02-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tso02");
    jsonld_test_negative(remote_test_file_to_str("so03-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tso03");
    jsonld_test_negative(remote_test_file_to_str("so07-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tso07");
    jsonld_test_negative(remote_test_file_to_str("so10-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tso10");
    jsonld_test_negative(remote_test_file_to_str("so12-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tso12");
    jsonld_test_negative(remote_test_file_to_str("so13-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/tso13");
    // jsonld_test_negative(remote_test_file_to_str("tn01-in.jsonld"), "https://w3c.github.io/json-ld-streaming/tests/ttn01");
}