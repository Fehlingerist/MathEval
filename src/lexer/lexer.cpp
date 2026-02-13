#include <lexer/lexer.hpp>

#include <string>
#include <array>

namespace Util { 
   using namespace std::string_literals;

   enum class CharacterType : uint8_t {
      Letter,
      Unicode,
      Numeric,
      Symbol, 
      Whitespace,
      NewLine,
      EndOfFile,
      Error,
   };

   namespace TypeClassificator {
      inline bool is_neutral_char_type(CharacterType char_type)
      {
         switch (char_type)
         {
         case CharacterType::Whitespace: case CharacterType::NewLine: case CharacterType::EndOfFile:
            return true;
         default:
            return false;
         }
      };

      inline bool is_numeric_char(char numeric_char)
      {
         return numeric_char >= '0' && numeric_char <= '9';
      };
 
      inline bool is_letter_char(char letter_char)
      {
         return (letter_char >= 'A' && letter_char <= 'Z') || (letter_char >= 'a' && letter_char <= 'z') || letter_char == '_';
      };

      inline bool is_special_char(char special_char)
      {
         return ((special_char >= '!' && special_char <= '~') && !is_numeric_char(special_char) && !is_letter_char(special_char));
      };

      inline bool is_newline_char(char new_line_char)
      {
         return new_line_char == '\n';
      };

      inline bool is_whitespace_char(char whitespace_char)
      {
         return whitespace_char == ' ' || whitespace_char == '\t' || whitespace_char == '\r';
      }; //it was perhaps a mistake that \n is treated as a whitespace instead of a special symbol?

      inline bool is_unicode(char unicode_char)
      {
         return static_cast<unsigned char>(unicode_char) >= 0b10000000;
      };

      inline bool is_hex_code(char hex_code_char)
      {
        return is_numeric_char(hex_code_char) || (hex_code_char >= 'a' && hex_code_char <= 'f') || (hex_code_char >= 'A' && hex_code_char <= 'F');
      };

      inline bool is_bin_code(char bin_code_char)
      {
         return bin_code_char == '0' || bin_code_char == '1';
      };

      inline bool is_valid_char(char unknown_char)
      {
         return (unknown_char >= ' ' && unknown_char <= '~') || is_whitespace_char(unknown_char) || is_unicode(unknown_char) || unknown_char == '\0' || is_newline_char(unknown_char);
      };
   };

   static const auto character_map = [](){
      using namespace TypeClassificator;
      std::array<CharacterType,256> character_map;

      for (int character_index = 0;character_index <= 255;character_index++)
      {
         unsigned char current_character = static_cast<unsigned char>(character_index);
         if(!is_valid_char(current_character))
         {
            character_map[current_character] = CharacterType::Error;
            continue;
         }
         else if (is_numeric_char(current_character))
         {
            character_map[current_character] = CharacterType::Numeric;
            continue;
         }
         else if (is_letter_char(current_character))
         {
            character_map[current_character] = CharacterType::Letter;      
            continue;
         }
         else if (is_whitespace_char(current_character))
         {
            character_map[current_character] = CharacterType::Whitespace;
            continue;
         }
         else if(is_newline_char(current_character))
         {
            character_map[current_character] = CharacterType::NewLine;
            continue;
         }  
         else if (is_unicode(current_character))
         {
            character_map[current_character] = CharacterType::Unicode;
            continue;
         };
         character_map[current_character] = CharacterType::Symbol;
      };

      character_map['\0'] = CharacterType::EndOfFile;

      return character_map;
   }();

   inline void test_char_type(unsigned char index_char, CharacterType expected_type)
   {
    
      CharacterType actual_type = character_map[index_char];

      Assert(
         actual_type == expected_type,
         LexerError + 
         "expected not to give an error"s +
         "Character mapping mismatch for char '"s + std::string(1,static_cast<char>(index_char)) + 
         "' (code: "s + std::to_string(index_char) + "). "s +
         "Expected type "s + std::to_string((int)(expected_type)) + 
         ", but map returneds " + std::to_string((int)(actual_type)) + 
         LexerErrorEnd
      );
   };

   void consume_numbers_letters(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();
      auto char_type = character_map[current_char]; 

      while (char_type == CharacterType::Numeric || char_type == CharacterType::Letter)
      {
         lexer_context.source.consume();

         current_char = lexer_context.source.see_current();
         char_type = character_map[current_char];
      };
   };

   void consume_identifier_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();

      test_char_type(current_char,CharacterType::Letter);

      size_t offset = lexer_context.source.index;
      consume_numbers_letters(lexer_context);
      size_t length = lexer_context.source.index - offset;
   
      std::string_view identifier_view = std::string_view(reinterpret_cast<char*>(lexer_context.source.get_source_buffer() + offset),length);

      lexer_context.record_identifier(identifier_view);
   };

   void consume_numbers(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();
      auto char_type = character_map[current_char]; 
      while (char_type == CharacterType::Numeric)
      {
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
         char_type = character_map[current_char];
      };
   };

   void consume_hex_numeric_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();
      auto next_char = lexer_context.source.peek();

      Assert(current_char == '0' && next_char == 'x',
         LexerError +
         "expected hex code number, got something else"s + 
         LexerErrorEnd
      )

      lexer_context.source.consume(2);

      current_char = lexer_context.source.see_current();

      size_t length = 0;

      while (TypeClassificator::is_hex_code(current_char))
      {
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
         length++;
      }

      if (!TypeClassificator::is_neutral_char_type(character_map[current_char]))
      {
         return lexer_context.record_error(ErrorCode::MalformedNumber);
      };

      if (length == 0)
      {
         return lexer_context.record_error(ErrorCode::TruncatedNumberSequence);
      };

      return lexer_context.record_number(NumberBase::Hexdecimal,NumberType::Integer);
   };

   void consume_bin_numeric_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();
      auto next_char = lexer_context.source.peek();

      Assert(current_char == '0' && next_char == 'b',
         LexerError +
         "expected hex code number, got something else"s + 
         LexerErrorEnd
      )

      lexer_context.source.consume(2);

      current_char = lexer_context.source.see_current();

      size_t length = 0;

      while (TypeClassificator::is_bin_code(current_char))
      {
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
         length++;
      }

      if (!TypeClassificator::is_neutral_char_type(character_map[current_char]))
      {
         return lexer_context.record_error(ErrorCode::MalformedNumber);
      };

      if (length == 0)
      {
         return lexer_context.record_error(ErrorCode::TruncatedNumberSequence);
      };

      return lexer_context.record_number(NumberBase::Binary, NumberType::Integer);
   }; 

   void consume_decimal_numeric_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();
      test_char_type(current_char,CharacterType::Numeric);

      consume_numbers(lexer_context);

      auto middle_char = lexer_context.source.see_current();
      auto middle_char_type = character_map[middle_char];
      auto is_symbol = middle_char_type == CharacterType::Symbol;

      if (middle_char == '.')
      {
         //most probably a float
         lexer_context.source.consume();
         consume_numbers(lexer_context);
      } else if (TypeClassificator::is_neutral_char_type(middle_char_type)|| is_symbol) [[likely]]
      {
         return lexer_context.record_number(NumberBase::Decimal,NumberType::Integer);
      } 
      else {
         return lexer_context.record_error(ErrorCode::MalformedNumber);
      };

      auto end_char = lexer_context.source.see_current();
      auto end_char_type = character_map[end_char];
      auto is_end_char_symbol = end_char_type == CharacterType::Symbol;

      //float path
      if (end_char == '.')
      {
         return lexer_context.record_error(ErrorCode::MalformedNumber);
      }
      else if (TypeClassificator::is_neutral_char_type(end_char_type) || is_end_char_symbol) [[likely]] 
      {
         return lexer_context.record_number(NumberBase::Decimal,NumberType::Float);
      } else {
         return lexer_context.record_error(ErrorCode::MalformedNumber);
      }; 
   }

   void consume_numeric_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();
      test_char_type(current_char,CharacterType::Numeric);
      auto next_char = lexer_context.source.peek();

      if (current_char == '0' && next_char == 'x')
      {
         return consume_hex_numeric_token(lexer_context);
      } else if(current_char == '0' && next_char == 'b')
      {
         return consume_bin_numeric_token(lexer_context);
      } else [[likely]] {
         return consume_decimal_numeric_token(lexer_context);
      };
   };

   void consume_eof_token(LexerContext& lexer_context)
   {
      test_char_type(lexer_context.source.see_current(),CharacterType::EndOfFile);
      lexer_context.source.consume();
   };

   void consume_error_token(LexerContext& lexer_context)
   {
      lexer_context.record_error(ErrorCode::UnexpectedCharacter);
      lexer_context.source.consume(); 
   };

   void consume_symbol_token(LexerContext& lexer_context)
   {
      using namespace SymbolClassifier;
      auto current_char = lexer_context.source.see_current();
      auto char_type = character_map[current_char];

      test_char_type(current_char,CharacterType::Symbol);

      auto start = lexer_context.source.index;
      auto start_ptr = reinterpret_cast<char*>(lexer_context.source.get_source_buffer() + start);

      auto symbol_kind = SymbolKind::UNKNOWN;
      
      while (char_type == CharacterType::Symbol)
      {
         auto length = lexer_context.source.index - start + 1; //include here
         auto next_symbol = get_symbol_from_buffer_fragment(start_ptr,length);
      
         if (next_symbol == SymbolKind::UNKNOWN)
         {
            return lexer_context.record_symbol(symbol_kind);
         }

         symbol_kind = next_symbol;
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
         char_type = character_map[current_char];
      }

      if (symbol_kind == SymbolKind::UNKNOWN)
      {
         return lexer_context.record_error(ErrorCode::UnknownSymbol);
      }

      return lexer_context.record_symbol(symbol_kind);
   };

   void consume_whitespace_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();

      test_char_type(current_char,CharacterType::Whitespace);

      auto char_type = character_map[current_char]; 
      while (char_type == CharacterType::Whitespace)
      {
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
         char_type = character_map[current_char];
      };
   };

   void consume_inline_comment(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();

      test_char_type(current_char,CharacterType::Symbol);

      auto next_char = lexer_context.source.peek();
      Assert(current_char == '/' && next_char == '/',
         LexerError + 
         "expected inline comment char start, got somethign else"
      );

      auto inline_char = current_char;
      while (character_map[inline_char] != CharacterType::NewLine && character_map[inline_char] != CharacterType::EndOfFile)
      {
         lexer_context.source.consume();
         inline_char = lexer_context.source.see_current();
      };

      if (character_map[inline_char] == CharacterType::EndOfFile)
      {
         return lexer_context.record_error(ErrorCode::UnclosedComment);
      };

      return;
   };

   void consume_block_comment(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();

      test_char_type(current_char,CharacterType::Symbol);

      auto next_char = lexer_context.source.peek();
      Assert(current_char == '/' && next_char == '*',
         LexerError + 
         "expected inline comment char start, got somethign else"
      );
      
      lexer_context.source.consume(2);

      auto inline_char = lexer_context.source.see_current();

      while (character_map[inline_char] != CharacterType::EndOfFile)
      {
         auto next_char = lexer_context.source.peek();

         if (inline_char == '*' && next_char == '/')
         {
            lexer_context.source.consume(2);
            return;
         };

         lexer_context.source.consume();
         inline_char = lexer_context.source.see_current();
      };

      return lexer_context.record_error(ErrorCode::UnclosedComment);
   };

   void consume_comment_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();
      test_char_type(current_char,CharacterType::Symbol);
      auto next_char = lexer_context.source.peek();

      Assert(current_char == '/' && (next_char == '/' || next_char == '*'), "expected comment symbol sequence, got something else")

      bool is_multiline_comment = next_char == '*';

      if (is_multiline_comment)
      {
         return consume_block_comment(lexer_context);
      } else {
         return consume_inline_comment(lexer_context);
      }
   };

   void consume_new_line_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();

      test_char_type(current_char,CharacterType::NewLine);
   
      lexer_context.source.consume();
   };

   void consume_string_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();
      auto char_type = character_map[current_char];

      Assert(
         current_char == '"',
         LexerError +
         "expected string to begin with \"\"\", got something else instead"s +
         LexerErrorEnd
      )

      do
      {
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
         char_type = character_map[current_char];
         
         if (char_type == CharacterType::EndOfFile) [[unlikely]]
         {
            return lexer_context.record_error(ErrorCode::UnclosedString);
         } else if (current_char == '\\') 
         {
            lexer_context.source.consume(); // skip '\'
            lexer_context.source.consume(); 
            continue;
         }
      } while (current_char != '"');
      
      lexer_context.source.consume(); //consume '"'      
      return;
   };

   void consume_char_token(LexerContext& lexer_context) {

      auto current_char = lexer_context.source.see_current();
      Assert(
         current_char == '\'',
         LexerError +
         "expected string to begin with \"\'\", got something else instead"s +
         LexerErrorEnd
      )

      lexer_context.source.consume(); 
      auto current_char = lexer_context.source.see_current();
      
      if (current_char == '\'') {
        return lexer_context.record_error(ErrorCode::InvalidCharCode);
      }

      if (current_char == '\\') {
        lexer_context.source.consume(); 
        auto escaped = lexer_context.source.see_current();
        
        if (escaped == 'n' || escaped == 't' || escaped == 'r' || 
            escaped == '0' || escaped == '\\' || escaped == '\'') {
            lexer_context.source.consume();
        } else {
            return lexer_context.record_error(ErrorCode::InvalidCharCode);
        }
      } else {
        lexer_context.source.consume(); 
      }

      if (lexer_context.source.see_current() != '\'') {
        return lexer_context.record_error(ErrorCode::UnclosedChar);
      }
      
      lexer_context.source.consume(); //closing '
      return;
   }
   
   namespace CLUA {
      TokenType guess_token_type(LexerContext& lexer_context)
      {
         auto current_char = static_cast<unsigned char>(lexer_context.source.see_current());
         auto character_type = character_map[current_char];

         switch (character_type)
         {
            case CharacterType::Error:
               return TokenKind<ErrorToken>::value;
            case CharacterType::Letter: 
               return TokenKind<IdentifierToken>::value;
            case CharacterType::Numeric:
               return TokenKind<NumericToken>::value;
            case CharacterType::Symbol:
            {
               auto next_char = static_cast<unsigned char>(lexer_context.source.peek());
               
               if (current_char == '/' && (next_char == '/' || next_char == '*')) [[unlikely]] 
               {  
                  return TokenKind<CommentToken>::value;
               } else if(current_char == '"')
               {
                  return TokenKind<StringToken>::value;
               } else if(current_char == '\'')
               {
                  return TokenKind<CharToken>::value;
               };

               return TokenKind<SymbolToken>::value;
            }
            case CharacterType::Whitespace:
               return TokenKind<WhitespaceToken>::value;
            case CharacterType::NewLine:
               return TokenKind<NewLineToken>::value;
            case CharacterType::EndOfFile:
               return TokenKind<EOFToken>::value;
            default:
               return TokenKind<ErrorToken>::value;
         };

         return TokenKind<NoToken>::value;
      };

      void get_next_token(LexerContext& lexer_context, TokenType token_type){
         switch (token_type)
         {
         case TokenType::Identifier:
            consume_identifier_token(lexer_context);
            break;
         case TokenType::Numeric:
            consume_numeric_token(lexer_context);
            break;
         case TokenType::Symbol:
            if (lexer_context.source.see_current() == '@')
            {
               lexer_context.switch_consumer_mode(ConsumerType::LuaUCapture);
            };
            consume_symbol_token(lexer_context);
            break;
         case TokenType::Whitespace:
            consume_whitespace_token(lexer_context);
            break;
         case TokenType::Comment:
            consume_comment_token(lexer_context);
            break;
         case TokenType::String:
            consume_string_token(lexer_context);
            break;
         case TokenType::Char:
            consume_char_token(lexer_context);
            break;
         case TokenType::NewLine:
            consume_new_line_token(lexer_context);
            break;
         case TokenType::EndOfFile:
            consume_eof_token(lexer_context);
            break;
         case TokenType::Error:
            consume_error_token(lexer_context);
            break;
         case TokenType::None:
            Assert(false,
               LexerError +
               "unexpected token type: got none"s +
               LexerErrorEnd  
            );
         default:
            Assert(false,
               LexerError +
               "unhandled token type: one at least has been forgotten"s +
               LexerErrorEnd  
            );
            break;
         }
      };
   }

   namespace LUAUCapture {
      using CLUA::guess_token_type;

      void get_next_token(LexerContext& lexer_context, TokenType token_type){
         switch (token_type)
         {
         case TokenType::Symbol:
            auto current_symbol = lexer_context.source.see_current();

            if (current_symbol == '[')
            {
               lexer_context.luau_capture_state.met_first_brace = true;
               lexer_context.luau_capture_state.brace_balance++;
            } else if (current_symbol == ']')
            {
               lexer_context.luau_capture_state.met_first_brace = true;
               lexer_context.luau_capture_state.brace_balance--;
            };

            consume_symbol_token(lexer_context);
            break;
         case TokenType::None:
            Assert(false,
               LexerError +
               "unexpected token type: got none"s +
               LexerErrorEnd  
            );
         default:
            CLUA::get_next_token(lexer_context,token_type);
         }

         if (lexer_context.luau_capture_state.brace_balance == 0 && lexer_context.luau_capture_state.met_first_brace)
         {
            lexer_context.switch_consumer_mode(ConsumerType::LuaU);
         };
      }
   };

   namespace LUAUCode {

      bool is_lua_string(LexerContext& lexer_context)
      {
         auto current_char = lexer_context.source.see_current();
         auto is_not_string_start = current_char != '\'' && current_char != '"' && current_char != '[' && current_char != '`';

         if (is_not_string_start)
         {
            return false;
         };

         auto next_char = lexer_context.source.peek();

         if (current_char == '[' && next_char == '=')
         {
            auto another_char = lexer_context.source.peek(2);
            return another_char == '[';
         } else if(current_char == '[' && next_char == '[')
         {
            return true;
         } else if (current_char == '[')
         {
            return false;
         } else {
            return true;
         };
      };

      bool is_lua_comment(LexerContext& lexer_context)
      {
         auto current_char = lexer_context.source.see_current();
         if (current_char != '-')
         {
            return false;
         };

         auto next_char = lexer_context.source.peek();
         if (next_char != '-')
         {
            return false;
         };

         return true;
      };

      void consume_unexpected_token(LexerContext& lexer_context)
      {
         lexer_context.record_error(ErrorCode::UnexpectedTokenType);
         lexer_context.source.consume();
         return;
      };

      enum class LuaUTokenType: uint8_t {
         LBracket,
         RBracket,
         String,
         Comment,
         Symbol,
         Whitespace,
         NewLine,
         Other,
         Error,
         EndOfFile,
         NoToken,
      };

      enum class LuaUCharType: uint8_t {
         Symbol,
         Other,
         EndOfFile,
         LBracket,
         RBracket,
         Error
      };

      auto luau_char_type_map = []()
      {
         std::array<LuaUCharType,256> luau_char_type_map;

         for (size_t char_code = 0; char_code < 256; char_code++)
         {
            auto real_char_code = static_cast<char>(char_code);
            
            if (!TypeClassificator::is_valid_char(real_char_code))
            {
               luau_char_type_map[char_code] = LuaUCharType::Error;
               continue;
            };

            if (TypeClassificator::is_special_char(real_char_code))
            {
               luau_char_type_map[char_code] = LuaUCharType::Symbol;
               continue;
            };

            luau_char_type_map[char_code] = LuaUCharType::Other;
         }

         luau_char_type_map['{'] = LuaUCharType::LBracket;
         luau_char_type_map['}'] = LuaUCharType::RBracket;

         luau_char_type_map['\0'] = LuaUCharType::EndOfFile;
         
         return luau_char_type_map;
      }();

      LuaUTokenType guess_luau_token_type(LexerContext& lexer_context)
      {
         auto current_char = lexer_context.source.see_current();
         auto character_type = luau_char_type_map[current_char];

         switch (character_type)
         {
            case LuaUCharType::Error:
               return LuaUTokenType::Error;
            case LuaUCharType::Other:
               return LuaUTokenType::Other;
            case LuaUCharType::Symbol:
            {
               
               if (is_lua_comment(lexer_context))
               {
                  return LuaUTokenType::Comment;
               } else if(is_lua_string(lexer_context))
               {
                  return LuaUTokenType::String;
               }

               return LuaUTokenType::Other;
            }
            case LuaUCharType::LBracket:
               return LuaUTokenType::LBracket;
            case LuaUCharType::RBracket:
               return LuaUTokenType::RBracket;
            case LuaUCharType::EndOfFile:
               return LuaUTokenType::EndOfFile;
            default:
               return LuaUTokenType::Error;
         };

         return LuaUTokenType::NoToken;
      };

      LuaUTokenType get_next_luau_token(LexerContext& lexer_context)
      {  
         auto luau_token_type = guess_luau_token_type(lexer_context);
         
         switch (luau_token_type)
         {
            case LuaUTokenType::LBracket:
               break;
            case LuaUTokenType::RBracket:
               break;
            default:
               break;
         }
      };

      void consume_lua_block(LexerContext& lexer_context)
      {
         
      };

      TokenType guess_token_type(LexerContext& lexer_context)
      {
         auto current_char = static_cast<unsigned char>(lexer_context.source.see_current());
         auto character_type = character_map[current_char];

         switch (character_type)
         {
            case CharacterType::Error:
               return TokenKind<ErrorToken>::value;
            case CharacterType::Letter: 
               return TokenKind<IdentifierToken>::value;
            case CharacterType::Numeric:
               return TokenKind<NumericToken>::value;
            case CharacterType::Symbol:
            {
               auto next_char = static_cast<unsigned char>(lexer_context.source.peek());
               if (current_char == '/' && (next_char == '/' || next_char == '*')) [[unlikely]] 
               {  
                  return TokenKind<CommentToken>::value;
               } else if (current_char == '{')
               {
                  return TokenKind<LuaBlockToken>::value;
               };
               return TokenKind<SymbolToken>::value;
            }
            case CharacterType::Whitespace:
               return TokenKind<WhitespaceToken>::value;
            case CharacterType::NewLine:
               return TokenKind<NewLineToken>::value;
            case CharacterType::EndOfFile:
               return TokenKind<EOFToken>::value;
            default:
               return TokenKind<ErrorToken>::value;
         };

         return TokenKind<NoToken>::value;
      };

      TokenType get_next_token(LexerContext& lexer_context){
         TokenType token_type = guess_token_type(lexer_context);
         lexer_context.original_token_type = token_type;
         switch (token_type)
         {
         case TokenType::LuaBlock:
            consume_lua_block(lexer_context);
            lexer_context.switch_consumer_mode(ConsumerType::CLua);
            break;
         case TokenType::Whitespace:
            CLUA::get_next_token(lexer_context,TokenType::Whitespace);
            break;
         case TokenType::None:
            CLUA::get_next_token(lexer_context,TokenType::None);
         case TokenType::Error:
            CLUA::get_next_token(lexer_context,TokenType::Error);
            break;
         default:
            consume_unexpected_token(lexer_context);
            break;
         };
      };
   };

   TokenGeneric Lexer::get_next_token()
   {
      auto token_type = TokenType::None;
      
      size_t start = lexer_context.source.index;
      
      switch (lexer_context.see_current_consumer_mode())
      {
      case ConsumerType::CLua:
         token_type = CLUA::guess_token_type(lexer_context);
         lexer_context.original_token_type = token_type;
         CLUA::get_next_token(lexer_context,token_type);
         break;
      case ConsumerType::LuaUCapture:
         token_type = LUAUCapture::guess_token_type(lexer_context);
         lexer_context.original_token_type = token_type;
         LUAUCapture::get_next_token(lexer_context,token_type);
         break;
      case ConsumerType::LuaU:
         token_type = LUAUCode::get_next_token(lexer_context);
         break;
      default:
         Assert(false,
            LexerError +
            "unhandled case for consumer type"s +
            LexerErrorEnd
         )
         break;
      }

      size_t end = lexer_context.source.index;
      size_t length = end - start;
      TokenGeneric token;
      token.token_type = lexer_context.ultimate_token_type;
      token.offset = start;
      token.length = length;

      return token;
   };
}