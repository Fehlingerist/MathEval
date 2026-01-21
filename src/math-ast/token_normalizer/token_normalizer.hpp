#pragma once
#include <cstdint>
#include <vector>
#include <lexer/lexer.hpp>

namespace AST{
    enum class NormalizedTokenType {
        Error,
        IntNumber, FloatNumber,
        KeywordIdentifier, LiteralIdentifier,
        StringLiteral,
        Symbol,
        CommentBlock
    };

    struct NormalizedToken {
        NormalizedTokenType token_type = NormalizedTokenType::Error;
        size_t length = 0;
        size_t offset = 0;
    };  

    /*
        Token Normalizer is language aware at some point.
        It detects basic expressions like comments and distinguishes number literals from each other.
    */
    class NormalizedTokenStream {
        std::vector<Util::Token> token_stream;

        NormalizedTokenStream(Util::Source& source)
        {
            
        };  
    };
}