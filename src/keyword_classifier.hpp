#pragma once

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
    Keyword(Const, "const") \
    Keyword(Static, "static") \
    Keyword(Template, "template") \
    Keyword(Class, "class") \
    Keyword(Struct, "Struct") \
    Keyword(Enum, "enum") \
    Keyword(Union, "union") \
    Keyword(Public, "public") \
    Keyword(Private, "private") \
    Keyword(Protected, "protected") \
    Keyword(Virtual, "virtual") \
    Keyword(Inline, "inline") \
    Keyword(Using, "using") \
    Keyword(Namespace, "namespace") \
    Keyword(Volatile, "volatile") \
    Keyword(Mutable, "mutable") \
    Keyword(Extern, "extern") \
    Keyword(Friend, "friend") \
    Keyword(New, "new") \
    Keyword(Delete, "delete") \
    Keyword(True, "true") \
    Keyword(False, "false") \
    Keyword(Nil, "nullptr") \
    Keyword(Typedef, "typedef") \
    Keyword(Auto, "auto") \
    Keyword(Decltype, "decltype") \
    Keyword(Constexpr, "constexpr") \
    Keyword(Consteval, "consteval") \
    Keyword(StaticAssert, "static_assert") \
    Keyword(Sizeof, "sizeof")   \
    Keyword(Lua,"Lua")

namespace KeywordClassifier {
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
    };

    Keyword get_keyword_type(std::string_view keyword)
    {
        if (keyword.length() == 0) return Keyword::Unknown;

        #define Keyword(KeywordValue, KeywordString) \
            else if (keyword == KeywordString) { \
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