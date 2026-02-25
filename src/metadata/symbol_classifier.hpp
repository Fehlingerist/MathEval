#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <stdint.h>

namespace SymbolClassifier {
    enum class SymbolKind : uint8_t {
        Plus,
        DoublePlus,
        PlusEqual,
        Minus,
        DoubleMinus,
        MinusEqual,
        Star,
        StarEqual,
        Slash,
        SlashEqual,
        Percent,
        PercentEqual,
        Equal,
        EqualEqual,
        NotEqual,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,
        LogicalAnd,
        LogicalOr,
        Bang,
        Dot,
        Range,
        Comma,
        Semicolon,
        Colon,
        Arrow,
        LParen,
        RParen,
        LBrace,
        RBrace,
        LBracket,
        RBracket,

        // Bitwise operators
        BitAnd,
        BitOr,
        BitXor,
        BitNot,
        BitLShift,
        BitRShift,

        // Compound bitwise assignments
        BitAndEqual,
        BitOrEqual,
        BitXorEqual,
        BitLShiftEqual,
        BitRShiftEqual,

        // Misc
        Question,
        TernaryAssign,
        AtSign,

        Unknown
    };

    // Use string_view as the key for better performance during lookups
    inline const std::unordered_map<std::string_view, SymbolKind> NormalizedSymbols = {
        {"++", SymbolKind::DoublePlus}, {"+=", SymbolKind::PlusEqual},
        {"--", SymbolKind::DoubleMinus}, {"-=", SymbolKind::MinusEqual},
        {"*=", SymbolKind::StarEqual}, {"/=", SymbolKind::SlashEqual},
        {"%=", SymbolKind::PercentEqual}, {"==", SymbolKind::EqualEqual},
        {"!=", SymbolKind::NotEqual}, {"<=", SymbolKind::LessEqual},
        {">=", SymbolKind::GreaterEqual}, {"&&", SymbolKind::LogicalAnd},
        {"||", SymbolKind::LogicalOr}, {"->", SymbolKind::Arrow},
        {"&", SymbolKind::BitAnd}, {"|", SymbolKind::BitOr},
        {"^", SymbolKind::BitXor}, {"~", SymbolKind::BitNot},
        {"<<", SymbolKind::BitLShift}, {">>", SymbolKind::BitRShift},
        {"&=", SymbolKind::BitAndEqual}, {"|=", SymbolKind::BitOrEqual},
        {"^=", SymbolKind::BitXorEqual}, {"<<=", SymbolKind::BitLShiftEqual},
        {">>=", SymbolKind::BitRShiftEqual}, {"+", SymbolKind::Plus},
        {"-", SymbolKind::Minus}, {"*", SymbolKind::Star},
        {"/", SymbolKind::Slash}, {"%", SymbolKind::Percent},
        {"=", SymbolKind::Equal}, {"<", SymbolKind::Less},
        {">", SymbolKind::Greater}, {"!", SymbolKind::Bang},
        {".", SymbolKind::Dot}, {",", SymbolKind::Comma},
        {";", SymbolKind::Semicolon}, {":", SymbolKind::Colon},
        {"(", SymbolKind::LParen}, {")", SymbolKind::RParen},
        {"{", SymbolKind::LBrace}, {"}", SymbolKind::RBrace},
        {"[", SymbolKind::LBracket}, {"]", SymbolKind::RBracket},
        {"?", SymbolKind::Question}, {"?=", SymbolKind::TernaryAssign},
        {"@", SymbolKind::AtSign}
    };

    inline SymbolKind get_symbol_from_buffer(const char* buffer, size_t length) {
        if (!buffer || length == 0) return SymbolKind::Unknown;

        // Construct a view to search the map directly
        std::string_view query(buffer, length);
        auto it = NormalizedSymbols.find(query);
        
        if (it != NormalizedSymbols.end()) {
            return it->second;
        }

        return SymbolKind::Unknown;
    }
}