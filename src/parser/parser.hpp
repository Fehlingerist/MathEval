#pragma once

#include <lexer/lexer.hpp>
#include <linear_allocator/linear_allocator.hpp>

#include "ast_nodes.hpp"

namespace ASTParser{
    using namespace std::literals::string_literals;

    constexpr auto ParserError = "Parser Error: "s;
    constexpr auto ParserErrorEnd = "\n"s;
    
    //AST-concept-1 should only parse math expressions and evaluate them immedieatly i
    enum class ParserErrorCode {
        None,
        LexerError,
        InvalidExpression
    };

    class ParserContext{
        Util::Lexer lexer;
        Util::LinearAllocator linear_allocator;
        
        Util::TokenGeneric current_token = static_cast<Util::TokenGeneric>(Util::NoToken()); //last acquired token really, but it points to current token in a way
        Util::TokenGeneric last_token = static_cast<Util::TokenGeneric>(Util::NoToken()); //last acquired token really, but it points to current token in a way

        ParserErrorCode last_parser_error = ParserErrorCode::None;

        size_t report_index = 0;

        Util::TokenGeneric get_next_non_neutral_token()
        {
            Util::TokenGeneric token;

            do
            {
                token = lexer.process_next_token();
            }
            while (token.token_type == Util::TokenType::Comment ||
                token.token_type == Util::TokenType::NewLine ||
                token.token_type == Util::TokenType::Whitespace);

            return token;
        }

        public:
        ParserContext(Util::Source& source): lexer(source)
        {};

        Util::Lexer& get_lexer()
        {
            return lexer;
        }

        Util::TokenGeneric get_next_token()
        {
            auto next_token = get_next_non_neutral_token();
            last_token = current_token;
            current_token = next_token;
            return next_token;
        };

        Util::TokenGeneric see_current_token()
        {
            return current_token;
        };

        Util::TokenGeneric get_last_token() {
            return last_token;
        };

        Util::TokenGeneric peek_next_token()
        {
            return lexer.peek_next_token();
        };

        Util::NumberHint get_last_number_hint()
        {
            return lexer.get_last_number_hint();
        };

        Util::Error get_last_error()
        {
            return lexer.get_last_error();
        };

        SymbolClassifier::SymbolKind get_current_symbol()
        {
            if (current_token.token_type != Util::TokenType::Symbol) [[unlikely]]
            {
                return SymbolClassifier::SymbolKind::Unknown;
            };
            return lexer.get_last_symbol();
        };

        KeywordClassifier::Keyword get_last_keyword()
        {
            return lexer.get_last_keyword();
        };
    
        void record_error(ParserErrorCode error_code)
        {
            Assert(
                report_index < (current_token.offset + 1),
                ParserError +
                "attempted to record the same token under different error code"s + 
                ParserErrorEnd
            )
            report_index = current_token.offset + 1;
            last_parser_error = error_code;
        };
    };

    class Parser{
        ParserContext parser_context;
        public:

        Parser(Util::Source& source): parser_context(source)
        {};        

        void generate_AST();

        double eval_math();
    };
}