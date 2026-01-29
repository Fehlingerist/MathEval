#include <token_normalizer.hpp>

#include <symbol_classifier.hpp>
#include <keyword_classifier.hpp>

#include <DebuggerAssets/debugger/debugger.hpp>

#define NormalizerError "Normalizer Error: "
#define NormalizerErrorEnd "\n"

/*
Number consumer is fragile: it assumes certain token types and sequences 
(e.g., numeric tokens followed by special chars like '.' or 'b'/'x') 
without a robust type system, leading to potential misclassification 
or silent errors. Structurally, branching logic is repeated and tightly 
coupled to token lookahead, making it error-prone for non-decimal, real, 
and integer numbers. Should use explicit number hints or token subtypes 
to simplify and harden the logic.
*/

/*
TRY_CONSUME should be used when multiple consume branches are possible within the same function
*/
#define TRY_CONSUME(func, context) \
    func(context); \
    if ((context).ultimate_token_type == NormalTokenType::Error) { \
        return; \
    }

namespace Normalizer {
    using namespace Util;
    /*
        Vague Normal Token Type is a helper enum
        for guess_token_type, to help determine how to consume the 
        token by type properly. There are some indications that the token is a number, 
        but the token normalizer must further down specify which number type: float/int...
        
        Same goes for identifiers...
    */

    enum class VagueNormalTokenType {
        Error,
        Number,
        Identifier,
        StringLiteral,
        Symbol,
        CommentBlock,
        Whitespace,
        EOFToken,
    };

    namespace NormalizerTypeClassificator {
        inline bool is_string_char(char string_char)
        {
            return string_char == '\'' || string_char == '"';
        };
        inline bool is_slash(char slash_char)
        {
            return slash_char == '/';
        }
        inline bool is_asterisk(char asterik_char)
        {
            return asterik_char == '*';
        }
    };

    namespace TypeGuessing {
        inline bool is_neutral_token(TokenType type) {
            switch (type) {
                case TokenType::Whitespace:
                case TokenType::NewLine:
                case TokenType::EndOfFile:
                    return true;
                default:
                    return false;
            }
        }

        inline bool is_numeric_char(char numeric_char)
        {
            return numeric_char >= '0' && numeric_char <= '9';
        }; 

        char to_lower_case(char ascii_char)
        {
            const auto offset = 'a' - 'A';
            if (ascii_char >= 'A' && ascii_char <= 'Z')
            {
                return ascii_char + offset;
            }
            return ascii_char;
        };
        
        inline bool is_hex_code_char(char hex_code)
        {
            return (to_lower_case(hex_code) >= 'a' && to_lower_case(hex_code) <= 'f') || is_numeric_char(hex_code);
        };

        inline bool is_bin_code_char(char bin_code)
        {
            return bin_code >= '0' && bin_code <= '1';
        };

        inline bool is_symbol_char(NormalTokenStreamContext& normal_token_stream_context,Token current_token,unsigned char expected_char)
        {
            Assert(
                current_token.token_type == TokenType::SpecialChar,
                NormalizerError
                "expected symbol token, got something else"
                NormalizerErrorEnd
            )

            if (current_token.length > 1)
            {
                return false;
            };

            auto source_viewer = Source(normal_token_stream_context.token_stream_reader.source_buffer + current_token.offset,current_token.length);
            auto current_char = source_viewer.see_current();

            return current_char == expected_char;
        };
        
        inline bool is_number_char(NormalTokenStreamContext& normal_token_stream_context,Token current_token,unsigned char expected_number_char)
        {
            Assert(
                current_token.token_type == TokenType::Numeric,
                NormalizerError
                "expected symbol token, got something else"
                NormalizerErrorEnd
            )

            if (current_token.length > 1)
            {
                return false;
            };

            auto source_viewer = Source(normal_token_stream_context.token_stream_reader.source_buffer + current_token.offset,current_token.length);
            auto current_char = source_viewer.see_current();

            return current_char == expected_number_char;
        };

        inline bool is_inline_comment(NormalTokenStreamContext& normal_token_stream_context)
        {
            auto& token_reader = normal_token_stream_context.token_stream_reader;
            auto current_token = token_reader.see_current();

            auto source_viewer = Source(token_reader.source_buffer + current_token.offset,current_token.length);
            
            auto current_char = source_viewer.see_current();
            auto next_char = source_viewer.peek();

            return NormalizerTypeClassificator::is_slash(current_char) && (NormalizerTypeClassificator::is_slash(next_char));
        };    

        inline bool is_inline_comment_end(NormalTokenStreamContext& normal_token_stream_context)
        {
            auto& token_reader = normal_token_stream_context.token_stream_reader;
            auto current_token = token_reader.see_current();

            return current_token.token_type == TokenType::NewLine;
        };

        inline bool is_block_comment(NormalTokenStreamContext& normal_token_stream_context)
        {
            auto& token_reader = normal_token_stream_context.token_stream_reader;
            auto current_token = token_reader.see_current();

            auto source_viewer = Source(token_reader.source_buffer + current_token.offset,current_token.length);
            
            auto current_char = source_viewer.see_current();
            auto next_char = source_viewer.peek();

            return NormalizerTypeClassificator::is_slash(current_char) && (NormalizerTypeClassificator::is_asterisk(next_char));
        };

        inline bool is_block_comment_end(NormalTokenStreamContext& normal_token_stream_context)
        {
            auto& token_reader = normal_token_stream_context.token_stream_reader;
            auto current_token = token_reader.see_current();

            auto source_viewer = Source(token_reader.source_buffer + current_token.offset,current_token.length);
            
            auto current_char = source_viewer.see_current();
            auto next_char = source_viewer.peek();

            return NormalizerTypeClassificator::is_asterisk(current_char) && NormalizerTypeClassificator::is_slash(next_char);
        };

        inline bool is_comment_type(NormalTokenStreamContext& normal_token_stream_context)
        {
            auto& token_reader = normal_token_stream_context.token_stream_reader;
            auto current_token = token_reader.see_current();

            auto source_viewer = Source(token_reader.source_buffer + current_token.offset,current_token.length);
            
            auto current_char = source_viewer.see_current();
            auto next_char = source_viewer.peek();

            return NormalizerTypeClassificator::is_slash(current_char) && (NormalizerTypeClassificator::is_slash(next_char) || NormalizerTypeClassificator::is_asterisk(next_char));
        };

        VagueNormalTokenType guess_token_type_string_symbol_comment(NormalTokenStreamContext& normal_token_stream_context)
        {
            auto& token_reader = normal_token_stream_context.token_stream_reader;
            auto current_token = token_reader.see_current();

            Assert(
                current_token.length == 1 && current_token.token_type == TokenType::SpecialChar,
                NormalizerError
                "presumption broken, current_token.length should always be 1 when the type of token is a symbol"
                NormalizerErrorEnd
            )
            auto source_viewer = Source(token_reader.source_buffer + current_token.offset,current_token.length);
            auto current_char = source_viewer.see_current();

            if (NormalizerTypeClassificator::is_string_char(current_char))
            {
                return VagueNormalTokenType::StringLiteral;
            };

            if (!is_comment_type(normal_token_stream_context))
            {
                return VagueNormalTokenType::Symbol;
            };

            return VagueNormalTokenType::CommentBlock;
        };

        VagueNormalTokenType guess_token_type(NormalTokenStreamContext& normal_token_stream_context)
        {
            auto& token_reader = normal_token_stream_context.token_stream_reader;
            auto current_token = token_reader.see_current();
        
            switch (current_token.token_type)
            {
                case TokenType::EndOfFile:
                    return VagueNormalTokenType::EOFToken;
                case TokenType::Error:
                    return VagueNormalTokenType::Error;
                case TokenType::Identifier:
                    return VagueNormalTokenType::Identifier;
                case TokenType::Numeric:
                    return VagueNormalTokenType::Number;
                case TokenType::SpecialChar:
                    return guess_token_type_string_symbol_comment(normal_token_stream_context);
                case TokenType::UnicodeSequence: //this shouldn't be read by guesser because UnicodeSequence is for consumption only
                    return VagueNormalTokenType::Error;
                case TokenType::Whitespace:
                    return VagueNormalTokenType::Whitespace;
                break;
            }
        };
    };

    inline unsigned char get_char_from_symbol(NormalTokenStreamContext& normal_token_stream_context,Token& token)
    {
        Assert(
            token.token_type == TokenType::SpecialChar,
            NormalizerError
            "assumption that token_type is a symbol is broken"
            NormalizerErrorEnd
        );
        Source current_source_context = Source(normal_token_stream_context.token_stream_reader.source_buffer+token.offset,token.length);
        return current_source_context.see_current();
    }

    inline void consume_string_token(NormalTokenStreamContext& normal_token_stream_context)
    {
        
    };

    inline void consume_char_token(NormalTokenStreamContext& normal_token_stream_context)
    {

    };

    inline void consume_string_literal(NormalTokenStreamContext& normal_token_stream_context)
    {

    };

    inline void consume_bin_decimal_code(NormalTokenStreamContext& normal_token_stream_context)
    {
        auto current_token = normal_token_stream_context.token_stream_reader.see_current();
            
        Assert(
            current_token.token_type == TokenType::Numeric,
            NormalizerError
            "expected numeric token, but got something else"
            NormalizerErrorEnd
        )   

        auto source_viewer = Source(normal_token_stream_context.token_stream_reader.source_buffer + current_token.offset,current_token.length);

        auto current_char = source_viewer.see_current();
        while (current_char != '\0')
        {
            if (!TypeGuessing::is_bin_code_char(current_char))
            {
                return normal_token_stream_context.emit_error(NormalErrorCode::MalformedNumber);
            }    
            source_viewer.consume();
            current_char = source_viewer.see_current();
        };
    };  

    inline void consume_hex_decimal_code(NormalTokenStreamContext& normal_token_stream_context)
    {
        auto current_token = normal_token_stream_context.token_stream_reader.see_current();
            
        Assert(
            current_token.token_type == TokenType::Numeric || current_token.token_type == TokenType::Identifier,
            NormalizerError
            "expected numeric/literal token string but got something else"
            NormalizerErrorEnd
        )

        auto start_token = current_token;
        auto end_token = current_token;

        while (current_token.token_type == TokenType::Numeric || current_token.token_type == TokenType::Identifier)
        {
            end_token = current_token;
            normal_token_stream_context.token_stream_reader.consume();
            current_token = normal_token_stream_context.token_stream_reader.see_current();
        };

        auto token_start = normal_token_stream_context.token_stream_reader.source_buffer + start_token.offset;
        auto token_length = end_token.offset - start_token.offset + end_token.length;

        if (token_length == 0)
        {
            return normal_token_stream_context.emit_error(NormalErrorCode::MalformedNumber);
        };

        auto source_viewer = Source(token_start,token_length);

        auto current_char = source_viewer.see_current();
        while (current_char != '\0')
        {
            if (!TypeGuessing::is_hex_code_char(current_char))
            {
                return normal_token_stream_context.emit_error(NormalErrorCode::MalformedNumber);
            }    
            source_viewer.consume();
            current_char = source_viewer.see_current();
        };
    };

    inline void consume_non_decimal_number(NormalTokenStreamContext& normal_token_stream_context)
    {
        auto current_token = normal_token_stream_context.token_stream_reader.see_current();
        
        Assert(
            current_token.token_type == TokenType::Identifier,
            NormalizerError
            "expected numeric type token, got something else"
            NormalizerErrorEnd
        )

        if (TypeGuessing::is_symbol_char(normal_token_stream_context,current_token,'b'))
        {
            return consume_bin_decimal_code(normal_token_stream_context);
        } else if(TypeGuessing::is_symbol_char(normal_token_stream_context,current_token,'x'))
        {
            return consume_hex_decimal_code(normal_token_stream_context);
        } else {
            return normal_token_stream_context.emit_error(NormalErrorCode::MalformedNumber);
        };
    };

    inline void consume_real_number(NormalTokenStreamContext& normal_token_stream_context)
    {
        auto current_token = normal_token_stream_context.token_stream_reader.see_current();

        Assert(
            current_token.token_type == TokenType::SpecialChar,
            NormalizerError
            "expected numeric type token, got something else"
            NormalizerErrorEnd
        )

        if (TypeGuessing::is_symbol_char(normal_token_stream_context,current_token,'.'))
        {
            return normal_token_stream_context.emit_error(NormalErrorCode::MalformedNumber);
        };

        auto next_token = normal_token_stream_context.token_stream_reader.peek();

        normal_token_stream_context.token_stream_reader.consume();

        if (next_token.token_type == TokenType::Numeric)
        {
            normal_token_stream_context.token_stream_reader.consume();
            return;
        } else {
            return normal_token_stream_context.emit_error(NormalErrorCode::MalformedNumber);
        };
    };

    inline void consume_integer_number(NormalTokenStreamContext& normal_token_stream_context)
    {
        auto current_token = normal_token_stream_context.token_stream_reader.see_current();
        Assert(
            current_token.token_type == TokenType::Numeric,
            NormalizerError
            "expected numeric type token, got something else"
            NormalizerErrorEnd
        )
        normal_token_stream_context.token_stream_reader.consume();
    };

    inline void consume_decimal_number(NormalTokenStreamContext& normal_token_stream_context)
    {
        auto current_token = normal_token_stream_context.token_stream_reader.see_current();

        Assert(
            current_token.token_type == TokenType::Numeric,
            NormalizerError
            "expected numeric type token, got something else"
            NormalizerErrorEnd
        )

        TRY_CONSUME(consume_integer_number,normal_token_stream_context);

        auto next_token = normal_token_stream_context.token_stream_reader.see_current();

        if (TypeGuessing::is_neutral_token(next_token.token_type))
        {
            return;
        } else if(next_token.token_type == TokenType::SpecialChar)
        {
            return consume_real_number(normal_token_stream_context);
        } else {
            return normal_token_stream_context.emit_error(NormalErrorCode::MalformedNumber);
        };
    };

    inline void consume_number_token(NormalTokenStreamContext& normal_token_stream_context)
    {
        auto current_token = normal_token_stream_context.token_stream_reader.see_current();
        auto token_size = current_token.length;

        auto next_token = normal_token_stream_context.token_stream_reader.peek();

        if (next_token.token_type == TokenType::Whitespace)
        {
            return consume_decimal_number(normal_token_stream_context);
        }
        else if (TypeGuessing::is_number_char(normal_token_stream_context,current_token,'0') && next_token.token_type == TokenType::Identifier)
        {
            return consume_non_decimal_number(normal_token_stream_context);
        }
        else {
            return normal_token_stream_context.emit_error(NormalErrorCode::MalformedNumber);
        }
    };
    
    inline void consume_inline_comment(NormalTokenStreamContext& normal_token_stream_context)
    {
        Assert(
            TypeGuessing::is_inline_comment(normal_token_stream_context),
            NormalizerError
            "expected inline comment, but got something else"
            NormalizerErrorEnd
        )
        auto last_token = normal_token_stream_context.token_stream_reader.see_current();
        while (last_token.token_type != TokenType::EndOfFile && last_token.token_type != TokenType::Error && last_token.token_type != TokenType::NewLine)
        {
            normal_token_stream_context.token_stream_reader.consume();
            last_token = normal_token_stream_context.token_stream_reader.see_current();
        }
    };

    inline void consume_block_comment(NormalTokenStreamContext& normal_token_stream_context)
    {
        Assert(
            TypeGuessing::is_block_comment(normal_token_stream_context),
            NormalizerError
            "expected block comment, but got something else"
            NormalizerErrorEnd
        )
        auto last_token = normal_token_stream_context.token_stream_reader.see_current();
        while (last_token.token_type != TokenType::EndOfFile && last_token.token_type != TokenType::Error)
        {
            auto next_token = normal_token_stream_context.token_stream_reader.peek();

            if (last_token.token_type == next_token.token_type && next_token.token_type == TokenType::SpecialChar
            && TypeGuessing::is_block_comment_end(normal_token_stream_context))
            {
                normal_token_stream_context.token_stream_reader.consume(); // '*'
                normal_token_stream_context.token_stream_reader.consume(); // '/'
                return;
            };

            normal_token_stream_context.token_stream_reader.consume();
            last_token = normal_token_stream_context.token_stream_reader.see_current();
        };
        normal_token_stream_context.emit_error(NormalErrorCode::CommentWithoutClosure);
        return;
    }; 

    inline void consume_comment_block(NormalTokenStreamContext& normal_token_steam_context)
    {
        auto is_inline = TypeGuessing::is_inline_comment(normal_token_steam_context);

        if(is_inline)
        {
            return consume_inline_comment(normal_token_steam_context);
        }

        return consume_block_comment(normal_token_steam_context);
    };

    inline void emit_keyword_token(NormalTokenStreamContext& normal_token_stream_context,KeywordClassifier::Keyword kewyword)
    {
        normal_token_stream_context.emit_keyword(kewyword);
    };

    inline void consume_identifier_token(NormalTokenStreamContext& normal_token_stream_context)
    {
        auto current_token = normal_token_stream_context.token_stream_reader.see_current();

        Assert(
            current_token.token_type == TokenType::Identifier,
            NormalizerError
            "expected identifier lexer token, but got something else instead"
            NormalizerErrorEnd
        )

        auto keyword_source = reinterpret_cast<char*>(normal_token_stream_context.token_stream_reader.source_buffer) + current_token.offset; //invalid
        auto keyword_length = current_token.length;

        auto keyword_string = std::string(keyword_source,keyword_length);

        auto keyword = KeywordClassifier::get_keyword_type(keyword_string.c_str());

        if (keyword != KeywordClassifier::Keyword::Unknown)
        {
            emit_keyword_token(normal_token_stream_context,keyword);
        }
        else {
            return normal_token_stream_context.emit_error(NormalErrorCode::UnknowSymbol);
        }

        normal_token_stream_context.token_stream_reader.consume();
    };

    inline void make_eof_token(NormalTokenStreamContext& normal_token_stream_context)
    {
        auto current_token = normal_token_stream_context.token_stream_reader.see_current();
        Assert(
            current_token.token_type == TokenType::EndOfFile,
            NormalizerError
            "expected EOF token, but got something else instead"
            NormalizerErrorEnd
        )
        normal_token_stream_context.token_stream_reader.consume();
    };

    inline void consume_symbol(NormalTokenStreamContext& normal_token_stream_context)
    {
        using namespace SymbolClassifier;

        auto current_token = normal_token_stream_context.token_stream_reader.see_current();
        auto start_token = current_token;

        Assert(
            current_token.token_type == TokenType::SpecialChar,
            NormalizerError
            "expected symbol lexer token, got something else"
            NormalizerErrorEnd
        )

        const auto* token_start_ptr = (char*)normal_token_stream_context.token_stream_reader.source_buffer + start_token.offset;
        const size_t token_length = (current_token.offset + current_token.length) - start_token.offset;

        const std::string token_string(token_start_ptr, token_length);

        auto keyword = get_symbol_from_cchar(token_string.c_str());

        while (current_token.token_type == TokenType::SpecialChar && keyword != SymbolKind::UNKNOWN)
        {
            normal_token_stream_context.token_stream_reader.consume();
            current_token = normal_token_stream_context.token_stream_reader.see_current();
        };

        if (keyword == SymbolKind::UNKNOWN)
        {
            normal_token_stream_context.emit_error(NormalErrorCode::UnknowSymbol);
        };
    };

    inline void error_token(NormalTokenStreamContext& normal_token_stream_context)
    {
        normal_token_stream_context.emit_error(NormalErrorCode::UnexpectedToken);
        normal_token_stream_context.token_stream_reader.consume();
    };

    NormalToken NormalTokenStream::process_next_token()
    {
        auto type = TypeGuessing::guess_token_type(normal_token_stream_context);

        size_t token_start = normal_token_stream_context.token_stream_reader.index;

        switch (type) {
            case VagueNormalTokenType::StringLiteral: 
            {
                normal_token_stream_context.ultimate_token_type = NormalTokenType::StringLiteral;
                consume_string_literal(normal_token_stream_context); 
                break;
            }
            case VagueNormalTokenType::Number: {
                normal_token_stream_context.ultimate_token_type = NormalTokenType::Number;
                consume_number_token(normal_token_stream_context); 
                break;
            }
            case VagueNormalTokenType::CommentBlock: 
            {   
                normal_token_stream_context.ultimate_token_type = NormalTokenType::CommentBlock;
                consume_comment_block(normal_token_stream_context); 
                break;
            }
            case VagueNormalTokenType::EOFToken: 
            {
                normal_token_stream_context.ultimate_token_type = NormalTokenType::EOFToken;
                make_eof_token(normal_token_stream_context); 
                break;
            }
            case VagueNormalTokenType::Symbol: {
                normal_token_stream_context.ultimate_token_type = NormalTokenType::Symbol;
                consume_symbol(normal_token_stream_context); 
                break;
            }
            case VagueNormalTokenType::Identifier: 
            {   
                normal_token_stream_context.ultimate_token_type = NormalTokenType::LiteralIdentifier;
                consume_identifier_token(normal_token_stream_context); 
                break;
            }
            default: 
            {   
                normal_token_stream_context.ultimate_token_type = NormalTokenType::Error;
                error_token(normal_token_stream_context); 
                break;
            }
        }
        
        size_t token_end = normal_token_stream_context.token_stream_reader.index;

        NormalToken token;
        token.token_type = normal_token_stream_context.ultimate_token_type;
        token.index = token_start;
        token.length = token_end - token_start;

        return token;
    }
}
