#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <lexer/lexer.hpp>

#include <keyword_classifier.hpp>
#include <symbol_classifier.hpp>
#include <DebuggerAssets/debugger/debugger.hpp>

#define NormalizerError "Normalizer Error: "
#define NormalizerErrorEnd "\n"

namespace Normalizer{
    enum class NormalErrorCode{
        None,
        CommentWithoutClosure,
        UnexpectedToken,
        UnknowSymbol,
        MalformedNumber
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

    enum class NumberBase {Integer, Decimal, Binary, Hex, Unknown };

    struct NumberInfo {
        NumberBase base = NumberBase::Unknown;
        size_t start_index = 0;
        size_t length = 0;
        // optionally store value later, but not required
    };

    /*
        Token Normalizer is language aware at some point.
        It detects basic expressions like comments and distinguishes number literals from each other.
    */
    class TokenStreamReader {   
        private:
        static inline Util::Token EOF_TOKEN = (Util::Token)[](){
            Util::Token TOKEN = Util::Token();
            TOKEN.token_type = Util::TokenType::EndOfFile;
            return TOKEN;
        }();
        public:

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
        std::vector<KeywordClassifier::Keyword> keyword_list;
        std::vector<SymbolClassifier::SymbolKind> symbol_list;
        std::vector<NumberInfo> number_list;  


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

        void emit_keyword(KeywordClassifier::Keyword keyword)
        {
            keyword_list.push_back(keyword);

            ultimate_token_type = NormalTokenType::KeywordIdentifier;
        };

        void emit_symbol(SymbolClassifier::SymbolKind symbol) 
        {
            symbol_list.push_back(symbol);

            //ultimate_token_type = NormalTokenType::Symbol; this already is a symbol so setting the ultimate type is redundant
        };

        void emit_number(NumberBase base, size_t start, size_t length) {
            NumberInfo info;
            info.base = base;
            info.start_index = start;
            info.length = length;
            number_list.push_back(info);
            ultimate_token_type = NormalTokenType::Number;
        }
    };

    class NormalTokenStream {

        NormalTokenStreamContext normal_token_stream_context;

        NormalTokenStream(Util::Source& source): normal_token_stream_context(source)
        {};  

        inline NormalToken process_next_token();

        public:
        inline NormalToken get_next_token()
        {
            return process_next_token();
        };
    };
}

#undef NormalizerError
#undef NormalizerErrorEnd