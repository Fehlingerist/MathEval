#pragma once

#include <string>
#include <unordered_map>
#include <stdint.h>

namespace SymbolClassifier {
enum class SymbolKind: uint8_t {
    PLUS,
    DOUBLE_PLUS,
    PLUS_EQUAL,
    MINUS,
    DOUBLE_MINUS,
    MINUS_EQUAL,
    STAR,
    STAR_EQUAL,
    SLASH,
    SLASH_EQUAL,
    PERCENT,
    PERCENT_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    NOT_EQUAL,
    LESS,
    LESS_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LOGICAL_AND,
    LOGICAL_OR,
    BANG,
    DOT,
    RANGE,
    COMMA,
    SEMICOLON,
    COLON,
    ARROW,
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,

    // Bitwise operators
    BIT_AND,
    BIT_OR,
    BIT_XOR,
    BIT_NOT,
    BIT_LSHIFT,
    BIT_RSHIFT,

    // Compound bitwise assignments
    BIT_AND_EQUAL,
    BIT_OR_EQUAL,
    BIT_XOR_EQUAL,
    BIT_LSHIFT_EQUAL,
    BIT_RSHIFT_EQUAL,

    // Misc
    QUESTION,
    TERNARY_ASSIGN,
    AT_SIGN,

    UNKNOWN
};

std::unordered_map<std::string, SymbolKind> normalized_symbols = {
        {"++", SymbolKind::DOUBLE_PLUS},        
        {"+=", SymbolKind::PLUS_EQUAL},
        {"--", SymbolKind::DOUBLE_MINUS},       
        {"-=", SymbolKind::MINUS_EQUAL},
        {"*=", SymbolKind::STAR_EQUAL},
        {"/=", SymbolKind::SLASH_EQUAL},
        {"%=", SymbolKind::PERCENT_EQUAL},
        {"==", SymbolKind::EQUAL_EQUAL},
        {"!=", SymbolKind::NOT_EQUAL},
        {"<=", SymbolKind::LESS_EQUAL},
        {">=", SymbolKind::GREATER_EQUAL},
        {"&&", SymbolKind::LOGICAL_AND},
        {"||", SymbolKind::LOGICAL_OR},
        {"->", SymbolKind::ARROW},

        // Bitwise
        {"&", SymbolKind::BIT_AND},
        {"|", SymbolKind::BIT_OR},
        {"^", SymbolKind::BIT_XOR},
        {"~", SymbolKind::BIT_NOT},
        {"<<", SymbolKind::BIT_LSHIFT},
        {">>", SymbolKind::BIT_RSHIFT},

        // Compound bitwise assignments
        {"&=", SymbolKind::BIT_AND_EQUAL},
        {"|=", SymbolKind::BIT_OR_EQUAL},
        {"^=", SymbolKind::BIT_XOR_EQUAL},
        {"<<=", SymbolKind::BIT_LSHIFT_EQUAL},
        {">>=", SymbolKind::BIT_RSHIFT_EQUAL},

        // Single-character symbols
        {"+", SymbolKind::PLUS},
        {"-", SymbolKind::MINUS},
        {"*", SymbolKind::STAR},
        {"/", SymbolKind::SLASH},
        {"%", SymbolKind::PERCENT},
        {"=", SymbolKind::EQUAL},
        {"<", SymbolKind::LESS},
        {">", SymbolKind::GREATER},
        {"!", SymbolKind::BANG},
        {".", SymbolKind::DOT},
        {",", SymbolKind::COMMA},
        {";", SymbolKind::SEMICOLON},
        {":", SymbolKind::COLON},
        {"(", SymbolKind::LPAREN},
        {")", SymbolKind::RPAREN},
        {"{", SymbolKind::LBRACE},
        {"}", SymbolKind::RBRACE},
        {"[", SymbolKind::LBRACKET},
        {"]", SymbolKind::RBRACKET},
        {"?", SymbolKind::QUESTION},
        {"?=", SymbolKind::TERNARY_ASSIGN},
        {"@", SymbolKind::AT_SIGN}
    };

    SymbolKind get_symbol_from_buffer_fragment(const char* buffer_fragment, size_t length)
    {
        if (length == 0 || buffer_fragment == nullptr)
            return SymbolKind::UNKNOWN; 

        for (const auto& [symbol_str, kind] : normalized_symbols)
        {
            if (symbol_str.size() != length)
                continue;

            bool match = true;
            for (size_t i = 0; i < length; ++i)
            {
                if (buffer_fragment[i] != symbol_str[i])
                {
                    match = false;
                    break;
                }
            }
            if (match)
                return kind;
            }   
            return SymbolKind::UNKNOWN; 
        }
}