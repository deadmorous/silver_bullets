#pragma once

namespace silver_bullets::iterate_struct {

struct EmptyValidator
{
    void enterNode(const char*) {}

    void exitNode() {}

    template<class T, class ChildNameGetter>
    void validate(const T&, const ChildNameGetter&) {}
};

} // namespace silver_bullets::iterate_struct
