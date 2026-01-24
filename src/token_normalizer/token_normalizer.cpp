#include <token_normalizer.hpp>

#include <DebuggerAssets/debugger/debugger.hpp>

#define CONDITIONAL_CONSUME()\

#define NormalizerError "Normalizer Error: "
#define NormalizerErrorEnd "\n"


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
        EOFToken,
    };

    enum class SpecificSymbolType {
        None,
        Operator,
        Punctuation,
        String,
        Char
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

    inline void consume_number_token(NormalTokenStreamContext& normal_token_stream_context)
    {

    };

    inline void consume_comment_block(NormalTokenStreamContext& normal_token_steam_context)
    {
        
    };

    inline void consume_identifier_token(NormalTokenStreamContext& normal_token_stream_context)
    {

    };

    inline void make_eof_token(NormalTokenStreamContext& normal_token_steam_context)
    {

    };

    inline void consume_symbol(NormalTokenStreamContext& normal_token_stream_context)
    {

    };

    inline void error_token(NormalTokenStreamContext& normal_token_stream_context)
    {

    };

    namespace TypeGuessing {
        VagueNormalTokenType guess_token_type_string_symbol_char(NormalTokenStreamContext& normal_token_stream_context)
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
                    return guess_token_type_string_symbol_char(normal_token_stream_context);
                case TokenType::UnicodeSequence: //this shouldn't be read by guesser because UnicodeSequence is for consumption only
                    return VagueNormalTokenType::Error;
                case TokenType::Whitespace:
                    return VagueNormalTokenType::CommentBlock;
                break;
            }
        };
    };

    NormalToken NormalTokenStream::get_next_token()
    {
        auto type = TypeGuessing::guess_token_type(normal_token_stream_context);

        size_t token_start = normal_token_stream_context.token_stream_reader.index;

        switch (type) {
            case VagueNormalTokenType::StringLiteral: 
            {
                normal_token_stream_context.ultimate_token_type = NormalTokenType::StringLiteral;
                consume_string_token(normal_token_stream_context); 
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