#pragma once

#include <DebuggerAssets/debugger/debugger.hpp>
#include <symbol_classifier.hpp>
#include <keyword_classifier.hpp>

#include <stdint.h>
#include <vector>
#include <type_traits>
#include <concepts>

namespace Util {

    using namespace std::string_literals;

    constexpr auto LexerError = "Lexer Error: "s;
    constexpr auto LexerErrorEnd = "\n"s;

    enum class ErrorCode: uint8_t {
        None,
        UnknownSymbol,
        UnexpectedCharacter,
        UnexpectedTokenType,
        InvalidByte,
        TruncatedUnicodeSequence,
        TruncatedNumberSequence,
        MalformedNumber,
        UnclosedComment,
        UnclosedString,
        UnclosedChar,
        InvalidCharCode,
        TooLongChar,
        UnclosedLuaBlock,
    };

    enum class TokenType: uint8_t {
        Identifier,
        Numeric,
        Symbol,
        Whitespace,
        NewLine,
        Comment,
        String,
        Char,
        EndOfFile,
        LuaBlock,
        Error,
        None
    };

    enum class NumberType: uint8_t {
        Integer,
        Float,
        None,
    };

    enum class NumberBase: uint8_t  {
        Hexdecimal,
        Decimal,
        Binary,
        None,
    };

    struct SourceView {
        unsigned char* source_buffer;
        size_t source_size;
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
                LexerError +
                "Source buffer must exist"s +
                LexerErrorEnd
            );
        };

        Source slice(size_t start_index = 0,size_t length = 0)
        {
            Assert(length > 0,
                LexerError +
                "length must be greater than 0"s +
                LexerErrorEnd
            )
            size_t end_index = start_index + length;
            Assert(
                end_index <= source_size,
                LexerError +
                "broken assumption that end_index <= source_size is true"s +
                LexerErrorEnd
            )
            return Source(source_buffer + start_index,length);
        };

        Source slice(size_t start_index = 0)
        {
            Assert(
                source_size > start_index,
                LexerError +
                "source_size > start_index is not true"s +
                LexerErrorEnd
            )

            return slice(start_index,source_size-start_index);
        };

        inline unsigned char* get_source_buffer()
        {
            return source_buffer;
        };

        inline bool can_consume_sentinel(size_t consume_distance = 1)
        {
            //source_size, because the additional character is a null terminator
            return index + consume_distance - 1 < source_size + 1;
        };

        inline bool can_consume(size_t consume_distance = 1)
        {
            return index + consume_distance - 1 < source_size;
        };

        inline void consume(size_t consume_distance = 1)
        {
            Assert(
                can_consume_sentinel(consume_distance),
                LexerError +
                "index is reading beyond the source_buffer"s +
                LexerErrorEnd
            );
            index += consume_distance;
        };

        inline unsigned char see_current()
        {
            Assert(
                can_consume_sentinel(),
                LexerError +
                "index is reading beyond the source_buffer"s + 
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
                LexerError +
                "Can't peek here"s + 
                LexerErrorEnd
            );
            if (!can_peek())
            {
                return (unsigned char)'\0';
            };
            return source_buffer[index+peek_distance];
        };
    };

    struct TokenBase
    {
        TokenType token_type = TokenType::Error;
        size_t length = 0;
        size_t offset = 0;
    };
    template <typename T>
    struct TokenKind {
        inline static constexpr TokenType value = TokenType::None;
    };

    struct IdentifierToken : TokenBase {};
    template<>
    struct TokenKind<IdentifierToken> {
        inline static constexpr TokenType value = TokenType::Identifier;
    };  

    struct NumericToken : TokenBase {};
    template<>
    struct TokenKind<NumericToken> {
        inline static constexpr TokenType value = TokenType::Numeric;
    };  

    struct SymbolToken : TokenBase {};
    template<>
    struct TokenKind<SymbolToken> {
        inline static constexpr TokenType value = TokenType::Symbol;
    };  

    struct WhitespaceToken : TokenBase {};
    template<>
    struct TokenKind<WhitespaceToken> {
        inline static constexpr TokenType value = TokenType::Whitespace;
    };  

    struct CommentToken: TokenBase {};
    template<>
    struct TokenKind<CommentToken>
    {
        inline static constexpr TokenType value = TokenType::Comment;
    };

    struct StringToken : TokenBase {};
    template<>
    struct TokenKind<StringToken> {
        inline static constexpr TokenType value = TokenType::String;
    };  

    struct CharToken: TokenBase {};
    template<>
    struct TokenKind<CharToken> {
        inline static constexpr TokenType value = TokenType::Char;
    };

    struct NewLineToken : TokenBase {};
    template<>
    struct TokenKind<NewLineToken> {
        inline static constexpr TokenType value = TokenType::NewLine;
    };  

    struct LuaBlockToken: TokenBase {};
    template<>
    struct TokenKind<LuaBlockToken>
    {
        inline static constexpr TokenType value = TokenType::LuaBlock;
    };

    struct EOFToken : TokenBase {};
    template<>
    struct TokenKind<EOFToken> {
        inline static constexpr TokenType value = TokenType::EndOfFile;
    };  

    struct ErrorToken : TokenBase {};   
    template<>
    struct TokenKind<ErrorToken> {
        inline static constexpr TokenType value = TokenType::Error;
    };  

    struct NoToken : TokenBase {};   
    template<>
    struct TokenKind<NoToken> {
        inline static constexpr TokenType value = TokenType::None;
    };  

    struct TokenGeneric : TokenBase {

        template <typename T>
        requires std::derived_from<T, TokenBase>
        T& as() {
            static_assert(sizeof(T) == sizeof(TokenGeneric), 
                "Relabeling failed: Derived struct has extra data members"s);
        
            const TokenType expected = TokenKind<T>::value;

            static_assert(expected != TokenType::None,
                "Invalid token template type is being used, they must be derived from TokenBase"s
            );

            Assert(
                token_type == expected,
                LexerError 
                + "expected this token type: "s 
                + std::to_string(static_cast<int>(expected)) 
                + " got: "s 
                + std::to_string(static_cast<int>(token_type)) 
                + LexerErrorEnd
            );

            return reinterpret_cast<T&>(*this);
        }

        template <typename T>
        requires std::derived_from<T, TokenBase>
        const T& as() const {
            static_assert(sizeof(T) == sizeof(TokenGeneric), "Size mismatch"s);
            
            const TokenType expected = TokenKind<T>::value;

            static_assert(expected != TokenType::None,
                "Invalid token template type is being used, they must be derived from TokenBase"s
            );

            Assert(
                token_type == expected,
                LexerError 
                + "expected this token type: "s 
                + std::to_string(static_cast<int>(expected)) 
                + " got: "s 
                + std::to_string(static_cast<int>(token_type)) 
                + LexerErrorEnd
            );

            return reinterpret_cast<const T&>(*this);
        }
    };

    struct Error {
        ErrorCode error_code;
    };

    struct NumberHint {
        NumberType number_type = NumberType::None;
        NumberBase number_base = NumberBase::None;
    };

    enum class ConsumerMode  {
        CLua,
        LuaU,
        LuaUCapture,
    };

    struct LuaUCaptureState {
        size_t brace_balance = 0; //Brace balance is how many "[" braces are against "]"
        bool met_first_brace = false;
    };

    struct LuaUCodeState {
        size_t brace_balance = 0; //Brace balance is how many "{" braces are against "}"
        bool met_first_brace = false;
    };
    
    class LexerContext {
        private:
        bool emitted = false;
        ConsumerMode consumer_type = ConsumerMode::CLua;

        public:

        LuaUCaptureState luau_capture_state;
        LuaUCodeState luau_code_state;

        Source source;
        std::vector<Error> errors;
        std::vector<NumberHint> numbers;
        std::vector<SymbolClassifier::SymbolKind> symbols;  
        std::vector<KeywordClassifier::Keyword> keywords;

        TokenType ultimate_token_type = TokenType::Error;
        TokenType original_token_type = ultimate_token_type; //this variable is strictly for recover if user chooses to do so

        LexerContext() = default;
        LexerContext(Source& source): source(source)
        {};

        inline ConsumerMode see_current_consumer_mode()
        {
            return consumer_type;
        };

        inline void switch_consumer_mode(ConsumerMode new_consumer_type)
        {
            consumer_type = new_consumer_type;
            luau_capture_state = LuaUCaptureState();
            luau_code_state = LuaUCodeState();
        };

        inline void token_enter()
        {
            emitted = false;
        };

        private:
        inline void on_emit(){
            Assert(
                !emitted,
                LexerError+
                "trying to emit hint multiple times within the same token"s +
                LexerErrorEnd
            )

            emitted = true;
        }
        
        public:

        constexpr inline bool has_emitted_report()
        {
            return emitted;
        };

        inline void record_error(ErrorCode error_code)
        {
            on_emit();

            Error error;
            error.error_code = error_code;
            errors.push_back(error);

            original_token_type = ultimate_token_type;
            ultimate_token_type = TokenKind<ErrorToken>::value;
        };

        inline void record_number(NumberBase number_base, NumberType number_type)
        {
            on_emit();

            NumberHint number_hint;
            number_hint.number_base = number_base;
            number_hint.number_type = number_type;

            numbers.push_back(number_hint);

            original_token_type = ultimate_token_type;
            ultimate_token_type = TokenKind<NumericToken>::value;
        };

        inline void record_symbol(SymbolClassifier::SymbolKind symbol)
        {
            on_emit();

            symbols.push_back(symbol);

            original_token_type = ultimate_token_type;
            ultimate_token_type = TokenKind<SymbolToken>::value;
        };

        inline void record_identifier(std::string_view identifier)
        {
            on_emit();

            auto keyword_type = KeywordClassifier::get_keyword_type(identifier);

            keywords.push_back(keyword_type);

            original_token_type = ultimate_token_type;
            ultimate_token_type = TokenKind<IdentifierToken>::value;
        };
    };

    class Lexer
    {
        private:
        LexerContext lexer_context;

        public:
        Lexer() = default;
        Lexer(Util::Source& source)
        {
            lexer_context = LexerContext(source);
        };

        private:
        TokenGeneric get_next_token();
        
        public:
        TokenGeneric process_next_token()
        {
            lexer_context.token_enter();
            return get_next_token();
        };

        const Error get_last_error()
        {
            return lexer_context.errors.back();
        };
    };
}   
