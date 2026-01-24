#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <lexer/lexer.hpp>

#include <keyword_classifier.hpp>
#include <DebuggerAssets/debugger/debugger.hpp>

#define NormalizerError "Normalizer Error: "
#define NormalizerErrorEnd "\n"

namespace Normalizer{
    enum class NormalErrorCode{
        None,
    };

    enum class NormalTokenType {
        Error,
        Number,
        KeywordIdentifier, LiteralIdentifier,
        StringLiteral,
        Char,
        Symbol,
        CommentBlock,
        EOFToken,
    };

    struct NormalError {
        NormalErrorCode error_code = NormalErrorCode::None;
        size_t index = 0;
    };

    struct NormalToken {
        size_t length = 0; //span in lexer tokens
        size_t index = 0; // index into lexer token stream
        NormalTokenType token_type = NormalTokenType::Error;
    };  
    /*
        Token Normalizer is language aware at some point.
        It detects basic expressions like comments and distinguishes number literals from each other.
    */
    class TokenStreamReader {   
        static inline Util::Token EOF_TOKEN = (Util::Token)[](){
            Util::Token TOKEN = Util::Token();
            TOKEN.token_type = Util::TokenType::EndOfFile;
            return TOKEN;
        }();

        Util::Lexer lexer; //It's for error analysis/backtracking if ever needed (will be needed really)
        std::shared_ptr<std::vector<Util::Token>> token_stream_ptr;

        public:
        size_t limit = 0; 
        size_t index = 0;
        unsigned char* source_buffer = nullptr;

        TokenStreamReader() = default;
        TokenStreamReader(Source& source): lexer(source), source_buffer(source.get_source_buffer()), token_stream_ptr(std::make_shared<std::vector<Util::Token>>())
        {   
            AssertNull(source_buffer)

            auto next_token = lexer.get_next_token();
            while (next_token.token_type != Util::TokenType::EndOfFile)
            {
                token_stream_ptr->push_back(next_token);
                /*
                    I should add an ability to recover from these errors for my language here to display multiple error messages at lexer
                    and token normalizer level.
                */
                if (next_token.token_type == Util::TokenType::Error)
                {
                    std::cout << "unexpected token type, can't process that stream further (Error)" << std::endl;
                    break;
                };
                next_token = lexer.get_next_token();
                limit++;
            };
        };

        inline TokenStreamReader slice(size_t start_index,size_t length)
        {
            Assert(
                start_index+length <= limit,
                NormalizerError
                "out of bounds slice"
                NormalizerErrorEnd
            )
            auto new_token_stream_reader = *this;
            
            new_token_stream_reader.index = start_index;
            new_token_stream_reader.limit = start_index + length;

            return new_token_stream_reader;
        };

        inline TokenStreamReader slice(size_t start_index)
        {
            return slice(start_index,limit - start_index);
        };

        inline bool can_consume()
        {
            return index < limit;
        };  

        inline void consume()
        {
            Assert(
                can_consume(),
                NormalizerError
                "Can't consume more lexer tokens"
                NormalizerErrorEnd
            );
            index++;
        };

        inline Util::Token see_current()
        {
            if (!can_consume())
            {
                return EOF_TOKEN;
            };
            return (*token_stream_ptr)[index];
        };

        inline bool can_peek(size_t peek_distance = 1) const noexcept
        {
           return index+peek_distance < limit;
        };

        inline Util::Token peek(size_t peek_distance = 1)
        {   
            if (!can_peek())
            {
                return EOF_TOKEN;
            };
            return (*token_stream_ptr)[index+peek_distance];
        };
    };

    class NormalTokenStreamContext {
        public:
        TokenStreamReader token_stream_reader;

        std::vector<NormalError> errors;
        NormalTokenType ultimate_token_type;

        NormalTokenStreamContext(Util::Source& source): token_stream_reader(source)
        {};  

        void emit_error(NormalErrorCode error_code)
        {
            NormalError error;
            error.error_code = error_code;
            error.index = token_stream_reader.index;
            errors.push_back(error);

            ultimate_token_type = NormalTokenType::Error;
        };
    };

    class NormalTokenStream {

        NormalTokenStreamContext normal_token_stream_context;

        NormalTokenStream(Util::Source& source): normal_token_stream_context(source)
        {};  

        inline NormalToken process_next_token();

        public:
        inline NormalToken get_next_token()
        {
            auto next_token = process_next_token();
            return next_token;
        };
    };
}

#undef NormalizerError
#undef NormalizerErrorEnd