#pragma once

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/istreamwrapper.h>

#include <sstream>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

namespace silver_bullets {
namespace iterate_struct {

class JsonValue
{
public:
    JsonValue() :
        m_value("null")
    {}

    JsonValue(const JsonValue&) = default;

    template<class T>
    explicit inline JsonValue(const T& x);

    explicit JsonValue(const rapidjson::Value& x) :
        m_value(fromRapidjsonValue(x))
    {}

    template<class T>
    inline T value() const;

    static JsonValue fromString(const std::string& s)
    {
        JsonValue result;
        result.m_value = s;
        return result;
    }

    const std::string& asString() const {
        return m_value;
    }

    rapidjson::Document toRapidjsonDocument() const {
        return toRapidjsonDocument(m_value);
    }

private:
    std::string m_value;

    static std::string fromRapidjsonValue(const rapidjson::Value& value)
    {
        std::ostringstream s;
        rapidjson::OStreamWrapper osw(s);
        auto writer = rapidjson::Writer<
            rapidjson::OStreamWrapper,
            rapidjson::UTF8<>,
            rapidjson::UTF8<>, rapidjson::CrtAllocator,
            rapidjson::kWriteDefaultFlags
            >(osw);
        value.Accept(writer);
        return s.str();
    }

    static rapidjson::Document toRapidjsonDocument(const std::string& value)
    {
        std::istringstream s(value);
        rapidjson::IStreamWrapper isw(s);
        rapidjson::Document result;
        result.ParseStream(isw);
        if (result.HasParseError())
            throw std::runtime_error(std::string("JSON parse error, code=") + boost::lexical_cast<std::string>(result.GetParseError()));
        return result;
    }
};

} // namespace iterate_struct
} // namespace silver_bullets
