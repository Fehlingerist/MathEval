#include "parser.hpp"

#include <cmath>

namespace ASTParser{
    using namespace SymbolClassifier;

    using TokenType = Util::TokenType;
    using NumberBase = Util::NumberBase;
    using NumberType = Util::NumberType;
    using Source = Util::Source;

    double eval_local_expression();
    double eval_subexpression();

    bool is_additive_operator(SymbolKind symbol_kind)
    {
        switch (symbol_kind)
        {
        case SymbolKind::Plus:
        case SymbolKind::Minus:
            return true;
        default:
            return false;
        };
    };

    bool is_multiplicative_operator(SymbolKind symbol_kind)
    {
        switch (symbol_kind)
        {
        case SymbolKind::Star:
        case SymbolKind::Slash:
            return true;
        default:
            return false;
        }
    };

    bool is_math_symbol_operator(SymbolKind symbol_kind)
    {
        return is_additive_operator(symbol_kind) || is_multiplicative_operator(symbol_kind);
    };

    double eval_numeric_token_bin_base(ParserContext& parser_context)
    {
        auto& lexer = parser_context.get_lexer();
        auto& lexer_context = lexer.get_lexer_context();
        auto current_token = parser_context.see_current_token();
        auto source_slice = lexer_context.source.slice(current_token.offset,current_token.length);

        source_slice.consume(2); //0x
        auto current_char = source_slice.see_current();
        
        double number_val = 0;

        while (current_char != '\0')
        {
            Assert(
                    Util::TypeClassificator::is_bin_code(current_char),
                    ParserError + 
                    "invalid binary number code, Lexer failed to validate bin code"s + 
                    ParserErrorEnd
            )

            unsigned int digit_value = current_char - '0';

            source_slice.consume();
            current_char = source_slice.see_current();
            number_val *= 2;
            number_val += digit_value;
        };

        return number_val;
    };

    double eval_numeric_token_hex_base(ParserContext& parser_context)
    {
        auto& lexer = parser_context.get_lexer();
        auto& lexer_context = lexer.get_lexer_context();
        auto current_token = parser_context.see_current_token();
        auto source_slice = lexer_context.source.slice(current_token.offset,current_token.length);

        source_slice.consume(2); //0x
        auto current_char = source_slice.see_current();
        
        double number_val = 0;

        while (current_char != '\0')
        {
            Assert(
                    Util::TypeClassificator::is_hex_code(current_char),
                    ParserError + 
                    "invalid binary number code, Lexer failed to validate bin code"s + 
                    ParserErrorEnd
            )

            unsigned int digit_value = 0;

            if (Util::TypeClassificator::is_numeric_char(current_char))
            {
                digit_value = current_char - '0';
            } else{
                if (current_char >= 'a')
                {
                    digit_value = current_char - 'a';
                } else{
                    digit_value = current_char - 'A';
                };
                digit_value += 10;
            };

            source_slice.consume();
            current_char = source_slice.see_current();
            number_val *= 16;
            number_val += digit_value;
        };

        return number_val;
    };

    double parse_double(Source& double_slice)
    {
        double initial_number = parse_int(double_slice);
        auto current_char = double_slice.see_current();

        Assert(
            current_char == '.',
            ParserError + 
            "Float expected, got something else instead"s + 
            ParserErrorEnd
        );

        double_slice.consume();

        auto start = double_slice.index;
        auto fraction_part = parse_int(double_slice);
        auto end = double_slice.index;
        auto divisor_size = std::pow(10,end-start);

        return initial_number + fraction_part / divisor_size;
    }

    double parse_int(Source& int_slice)
    {  
        double number_value = 0;
        auto current_char = int_slice.see_current();

        while (Util::TypeClassificator::is_numeric_char(current_char))
        {
            unsigned int digit_value = 0;
            
            digit_value = current_char - '0';

            int_slice.consume();
            current_char = int_slice.see_current();
            number_value *= 10;
            number_value += digit_value;
        };

        return number_value;
    };

    double eval_double(ParserContext& parser_context)
    {
        auto& lexer = parser_context.get_lexer();
        auto& lexer_context = lexer.get_lexer_context();

        auto current_token = parser_context.see_current_token();
        auto source_slice = lexer_context.source.slice(current_token.offset,current_token.length);

        return parse_double(source_slice);
    };

    double eval_integer(ParserContext& parser_context)
    {
        auto& lexer = parser_context.get_lexer();
        auto& lexer_context = lexer.get_lexer_context();

        auto current_token = parser_context.see_current_token();
        auto source_slice = lexer_context.source.slice(current_token.offset,current_token.length);

        return parse_int(source_slice);
    };

    double eval_number_literal_value(ParserContext& parser_context){
        auto current_token = parser_context.see_current_token();
        if (current_token.token_type != TokenType::Numeric)
        {
            parser_context.record_error(ParserErrorCode::InvalidExpression);
            return 0;
        };

        auto number_kind = parser_context.get_last_number_hint();
        if (number_kind.number_base == NumberBase::Binary)
        {
            return eval_numeric_token_bin_base(parser_context);
        } else if(number_kind.number_base == NumberBase::Hexdecimal)
        {
            return eval_numeric_token_hex_base(parser_context);
        } else if(number_kind.number_base == NumberBase::Decimal)
        {
            if (number_kind.number_type == NumberType::Float)
            {
                return eval_double(parser_context);
            } else if(number_kind.number_type == NumberType::Integer) {
                return eval_integer(parser_context);
            } else {
                return 0;
            };
        }
    };

    double eval_subexpression(ParserContext& parser_context){
        auto current_token = parser_context.see_current_token();
        auto n_factor = 1;

        auto current_symbol = parser_context.get_current_symbol();

        if (current_symbol == SymbolKind::Minus) //unary operation most likely
        {   
            auto is_negative = true; //Unary op effect
            n_factor = -1;
            parser_context.get_next_token();
            current_token = parser_context.see_current_token();
        }
        else if (current_token.token_type != TokenType::Numeric)
        {
            //syntax error
            return 0;
        };

        double number_value = 0;

        if (parser_context.get_current_symbol() == SymbolKind::LBrace)
        {
            number_value = eval_local_expression(parser_context);
        } else if(current_token.token_type == TokenType::Numeric)
        {
            number_value = eval_number_literal_value(parser_context);
        };

        static_assert(sizeof(number_value * n_factor) == sizeof(double));
        return number_value * n_factor;
    };

    double eval_local_expression(ParserContext& parser_context)
    {

    };

    double eval_math_expression(ParserContext& parser_context)
    {

    };

    double Parser::eval_math()
    {
        auto token = parser_context.get_next_token();
        return eval_math_expression(parser_context);
    };
};