#pragma once

#include <fstream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>

#include <rapidjson/document.h>
// #include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/istreamwrapper.h>

inline rapidjson::Document read_json_doc(std::istream& s)
{
    rapidjson::IStreamWrapper isw(s);
    rapidjson::Document document;
    document.ParseStream(isw);
    if (document.HasParseError())
        throw std::runtime_error(std::string("JSON parse error, code=") + boost::lexical_cast<std::string>(document.GetParseError()));
    return document;
}

inline rapidjson::Document read_json_doc(const std::string& fileName)
{
    std::ifstream is(fileName);
    if (!is.is_open())
        throw std::runtime_error(std::string("Failed to open input file '")+fileName+"'");
    // is.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    return read_json_doc(is);
}



inline void write_json_doc(std::ostream& s, const rapidjson::Document& document)
{
    rapidjson::OStreamWrapper osw(s);
    auto writer = rapidjson::PrettyWriter<
            rapidjson::OStreamWrapper,
            rapidjson::UTF8<>,
            rapidjson::UTF8<>, rapidjson::CrtAllocator,
            rapidjson::kWriteDefaultFlags
            >(osw);
    document.Accept(writer);
}

inline void write_json_doc(const std::string& fileName, const rapidjson::Document& document)
{
    std::ofstream os(fileName);
    if (!os.is_open())
        throw std::runtime_error(std::string("Failed to open input file '")+fileName+"'");
    os.exceptions (std::ofstream::failbit | std::ofstream::badbit);
    write_json_doc(os, document);
}
