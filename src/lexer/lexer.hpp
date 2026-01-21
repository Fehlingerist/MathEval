#include <DebuggerAssets/debugger/debugger.hpp>

#include <cstdint>
#include <vector>

#define LexerError "Lexer Error: "
#define LexerErrorEnd "\n"

namespace Util {

    enum class ErrorCode {
        None,
        UnexpectedCharacter,
        InvalidByte,
        TruncatedSequence
    };

    enum class TokenType {
        Identifier,
        UnicodeSequence,
        Numeric,
        SpecialChar,
        Whitespace,
        EndOfFile,
        Error,  
    };

    class Source {
        public:
        size_t index;
        private:
        size_t source_size;       
        unsigned char* source_buffer;

        public:
        
        Source() = default;
        Source(unsigned char* source_buffer, size_t source_size) : source_buffer(source_buffer), source_size(source_size), index(0)
        {
            Assert(source_buffer,
                LexerError 
                "Source buffer must exist" 
                LexerErrorEnd
            );
        };

        inline bool can_consume_sentinel()
        {
            //source_size, because the additional character is a null terminator
            return index < source_size + 1;
        };

        inline bool can_consume()
        {
            return index < source_size;
        };

        inline void consume()
        {
            Assert(
                can_consume_sentinel(),
                LexerError 
                "index is reading beyond the source_buffer"
                LexerErrorEnd
            );
            index++;
        };

        inline unsigned char see_current()
        {
            Assert(
                can_consume_sentinel(),
                LexerError 
                "index is reading beyond the source_buffer"
                LexerErrorEnd
            );
            if (!can_consume())
            {
                return '\0';
            };
            return source_buffer[index];
        };

        inline bool can_peek_sentinel(size_t peek_distance) const noexcept
        {  
            return (index+peek_distance) < source_size + 1;
        };

        inline bool can_peek(size_t peek_distance = 1) const noexcept
        {
            return (index+peek_distance) < source_size;
        };

        inline unsigned char peek(size_t peek_distance = 1)
        {   
            Assert(
                can_peek_sentinel(peek_distance),
                LexerError 
                "Can't peek here" 
                LexerErrorEnd
            );
            if (!can_peek())
            {
                return (unsigned char)'\0';
            };
            return source_buffer[index+peek_distance];
        };
    };

    struct Token
    {
        TokenType token_type = TokenType::Error;
        size_t length = 0;
        size_t offset = 0;
    };

    struct Error {
        size_t offset = 0;
        ErrorCode error_code;
    };
    
    class LexerContext {
        public:
        Source source;
        std::vector<Error> errors;

        TokenType ultimate_token_type;
        TokenType original_token_type; //this variable is strictly for recover if user chooses to do so

        LexerContext() = default;
        LexerContext(Source& source): source(source)
        {};

        void emit_error(ErrorCode error_code)
        {
            Error error;
            error.error_code = error_code;
            error.offset = source.index;
            errors.push_back(error);

            original_token_type = ultimate_token_type;
            ultimate_token_type = TokenType::Error;
        };
    };

    class Lexer
    {
        private:
        LexerContext lexer_context;

        public:
        Lexer(Source& source)
        {
            lexer_context = LexerContext(source);
        };

        Token get_next_token();
    };
}   