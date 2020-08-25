#pragma once

#include "JsonValue_fwd.hpp"
#include "json_doc_converter.hpp"

namespace silver_bullets {
namespace iterate_struct {

template<class T>
inline JsonValue::JsonValue(const T& x) :
    m_value(fromRapidjsonValue(to_json_doc(x)))
{}

template<class T>
inline T JsonValue::value() const {
    return from_json_doc<T>(toRapidjsonDocument(m_value));
}

} // namespace iterate_struct
} // namespace silver_bullets
