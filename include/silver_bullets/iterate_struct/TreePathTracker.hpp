#pragma once

#include <vector>
#include <string>

namespace silver_bullets::iterate_struct {

class TreePathTracker
{
public:
    void enterNode(const char* name) {
        m_path.push_back(name);
    }

    void exitNode() {
        m_path.pop_back();
    }

    const std::vector<std::string>& path() const {
        return m_path;
    }

private:
    std::vector<std::string> m_path;
};

} // namespace silver_bullets::iterate_struct
