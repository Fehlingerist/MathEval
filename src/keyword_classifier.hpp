#include <cstring>

#define KEYWORDS \
    Keyword(If, "if") \
    Keyword(Else, "else") \
    Keyword(For, "for") \
    Keyword(While, "while") \
    Keyword(Do, "do") \
    Keyword(Switch, "switch") \
    Keyword(Case, "case") \
    Keyword(Default, "default") \
    Keyword(Break, "break") \
    Keyword(Continue, "continue") \
    Keyword(Return, "return") \
    Keyword(Const, "Const") \
    Keyword(Static, "Static") \
    Keyword(Template, "Template") \
    Keyword(Class, "Class") \
    Keyword(Struct, "Struct") \
    Keyword(Enum, "Enum") \
    Keyword(Union, "Union") \
    Keyword(Public, "public") \
    Keyword(Private, "private") \
    Keyword(Protected, "protected") \
    Keyword(Virtual, "Virtual") \
    Keyword(Inline, "Inline") \
    Keyword(Using, "Using") \
    Keyword(Namespace, "Namespace") \
    Keyword(Volatile, "Volatile") \
    Keyword(Mutable, "Mutable") \
    Keyword(Extern, "Extern") \
    Keyword(Friend, "Friend") \
    Keyword(New, "New") \
    Keyword(Delete, "Delete") \
    Keyword(True, "true") \
    Keyword(False, "false") \
    Keyword(Nil, "nullptr") \
    Keyword(Typedef, "typedef") \
    Keyword(Auto, "Auto") \
    Keyword(Decltype, "decltype") \
    Keyword(Constexpr, "Constexpr") \
    Keyword(StaticAssert, "static_assert") \
    Keyword(Sizeof, "sizeof")

namespace AST {
    enum class Keyword {
        #define Keyword(KeywordValue,KeywordString)\
            KeywordValue,    
            KEYWORDS
        #undef Keyword
        Unknown
    };

    Keyword get_keyword_type(const char* keyword)
    {
        if (!keyword || !*keyword) return Keyword::Unknown;

        #define Keyword(KeywordValue, KeywordString) \
            else if (std::strcmp(keyword, KeywordString) == 0) { \
                return Keyword::KeywordValue; \
            }
            KEYWORDS
        #undef Keyword

        else return Keyword::Unknown;
    }

    const char* keyword_to_string(Keyword kw)
    {
        switch (kw)
        {
        #define Keyword(KeywordValue, KeywordString) \
            case Keyword::KeywordValue: return KeywordString;
                KEYWORDS
        #undef Keyword
                default: return "<Unknown>";
        }
    }

};

#undef KEYWORDS