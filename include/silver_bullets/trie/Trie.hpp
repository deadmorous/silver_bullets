#include <algorithm>
#include <type_traits>
#include <string>
#include <stdexcept>

namespace silver_bullets {

template<class Alphabet, class Index>
class Trie
{
public:
    using character_type = typename Alphabet::character_type;

    static constexpr const Index InvalidIndex = Index(~0);

    Index add(const std::string& str,
              std::enable_if_t<std::is_same_v<character_type, char>, int> = 0)
    {
        return add(str.c_str());
    }

    Index add(const character_type *str)
    {
        auto node = &m_root;
        while(true) {
            auto c = Alphabet::translate(*str);
            if (c < Alphabet::size) {
                auto child = node->children[c];
                if (!child)
                    child = node->children[c] = new Node;
                node = child;
                ++str;
            }
            else if (c == Alphabet::terminator) {
                if (node->index == InvalidIndex)
                    node->index = m_count++;
                return node->index;
            }
            else
                throw std::runtime_error("Adding an invalid sequence to the trie");
        }
    }

    template<class ... Char>
    Index find(const Char* ... str) const
    {
        auto node = findNode(&m_root, str...);
        return node? node->index: InvalidIndex;
    }

    Index find(const std::string& str) const {
        return find(str.c_str());
    }

private:
    struct Node {
        Node* children[Alphabet::size];
        Index index = InvalidIndex;
        Node() {
            std::fill(children, children+Alphabet::size, nullptr);
        }
        ~Node() {
            std::for_each(children, children+Alphabet::size, [](Node *child) { delete child; });
        }
    };
    Node m_root;
    Index m_count = Index();

    const Node *findNode(const Node *root) const {
        return root;
    }

    const Node *findNode(const Node *root, const character_type *str) const
    {
        auto node = root;
        while(node) {
            auto c = Alphabet::translate(*str);
            if (c < Alphabet::size) {
                node = node->children[c];
                ++str;
            }
            else if (c == Alphabet::terminator)
                return node;
            else
                break;
        }
        return nullptr;
    }

    template<class ... Char>
    const Node *findNode(
            const Node *root,
            const character_type *str1,
            const character_type *str2,
            const Char* ... rest) const
    {
        return findNode(findNode(findNode(root, str1), str2), rest...);
    }
};

template<char first, char last>
struct CharRangeAlphabet
{
    using character_type = char;
    static constexpr const unsigned int size = last - first + 1;
    static constexpr const unsigned int terminator = ~0u;
    static unsigned int translate(char c)
    {
        if (c >= first && c <= last)
            return static_cast<unsigned int>(c - first);
        else if (c == 0)
            return terminator;
        else
            return size;
    }
};

using LatinUppercaseAlphabet = CharRangeAlphabet<'A', 'Z'>;
using LatinLowercaseAlphabet = CharRangeAlphabet<'a', 'z'>;
using AsciiCharAlphabet = CharRangeAlphabet<' ', '~'>;

struct AsciiCharCaseInsensitiveAlphabet
{
    using character_type = char;
    static constexpr const unsigned int size = '~' - ' ' + 1 - 25;
    static constexpr const unsigned int terminator = ~0u;
    static unsigned int translate(char c)
    {
        if (c == 0)
            return terminator;
        else if (c < ' ')
            return size;
        else if (c < 'a')
            return static_cast<unsigned int>(c - ' ');
        else if (c <= 'z')
            return static_cast<unsigned int>(c - 'a' + 'A' - ' ');
        else if (c <= '~')
            return static_cast<unsigned int>(c - 25 - ' ');
        else
            return size;
    }
};

} // namespace silver_bullets
