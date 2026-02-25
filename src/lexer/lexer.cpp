#include <lexer/lexer.hpp>

#include <string>
#include <array>

namespace Util { 
   using namespace std::string_literals;

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

      if (!TypeClassificator::is_hex_code(current_char))
      {
         return lexer_context.record_error(ErrorCode::MalformedNumber);
      };

      size_t length = 0;

      while (TypeClassificator::is_hex_code(current_char))
      {
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
         length++;
      }

      if (!TypeClassificator::is_number_compapitable_char_type(character_map[current_char]))
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

      if(!TypeClassificator::is_bin_code(current_char))
      {
         return lexer_context.record_error(ErrorCode::MalformedNumber);
      };

      size_t length = 0;

      while (TypeClassificator::is_bin_code(current_char))
      {
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
         length++;
      }

      if (!TypeClassificator::is_number_compapitable_char_type(character_map[current_char]))
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
      auto first_char = current_char;

      if (current_char == '.')
      {
         //consume if .[numbers] case 
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
      
         Assert(
            character_map[current_char] == CharacterType::Numeric,
            LexerError + 
            "unexpected behaviour: token guesser misslcassified token type"s + 
            LexerErrorEnd
         )
      };

      consume_numbers(lexer_context);

      if(first_char == '.')
      {
         //.[numbers]
         return lexer_context.record_number(NumberBase::Decimal,NumberType::Float);
      };

      auto middle_char = lexer_context.source.see_current();
      auto middle_char_type = character_map[middle_char];

      if (middle_char == '.')
      {
         //both cases:
         //[numbers](consume_index).[numbers]
         //[numbers](consume_index).
         lexer_context.source.consume();
         consume_numbers(lexer_context);
      } else if (TypeClassificator::is_number_compapitable_char_type(middle_char_type)) [[likely]]
      {
         //[numbers]
         return lexer_context.record_number(NumberBase::Decimal,NumberType::Integer);
      } 
      else {
         return lexer_context.record_error(ErrorCode::MalformedNumber);
      };

      auto end_char = lexer_context.source.see_current();
      auto end_char_type = character_map[end_char];

      if (TypeClassificator::is_number_compapitable_char_type(end_char_type)) [[likely]] 
      {
         //[numbers].[numbers]
         //[numbers].
         return lexer_context.record_number(NumberBase::Decimal,NumberType::Float);
      } else {
         return lexer_context.record_error(ErrorCode::MalformedNumber);
      }; 
   }

   void consume_numeric_token(LexerContext& lexer_context)
   {
      auto current_char = lexer_context.source.see_current();

      if (current_char != '.')
      {
       test_char_type(current_char,CharacterType::Numeric);
      }

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

      auto symbol_kind = SymbolKind::Unknown;
      
      while (char_type == CharacterType::Symbol)
      {
         auto length = lexer_context.source.index - start + 1; 
         auto next_symbol = get_symbol_from_buffer(start_ptr,length);
      
         if (next_symbol == SymbolKind::Unknown)
         {
            return lexer_context.record_symbol(symbol_kind);
         }

         symbol_kind = next_symbol;
         lexer_context.source.consume();
         current_char = lexer_context.source.see_current();
         char_type = character_map[current_char];
      }

      if (symbol_kind == SymbolKind::Unknown)
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

      /*
         if (character_map[inline_char] == CharacterType::EndOfFile)
         {
            return lexer_context.record_error(ErrorCode::UnclosedComment);
         };
      */

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
            lexer_context.source.consume(); // skip '\' unless nullptr
            auto next_char = lexer_context.source.see_current();
            if (character_map[next_char] == CharacterType::EndOfFile)
            {
               return lexer_context.record_error(ErrorCode::UnclosedString);
            };
            continue;
         }
      } while (current_char != '"');
      
      lexer_context.source.consume(); //consume '"'      
      return;
   };

   void consume_char_token(LexerContext& lexer_context) {
      Assert(
         lexer_context.source.see_current() == '\'',
         LexerError +
         "expected string to begin with \"\'\", got something else instead"s +
         LexerErrorEnd
      )

      lexer_context.source.consume(); 
      auto current_char = lexer_context.source.see_current();

      size_t counter = 0;
      while (current_char != '\'')
      {
         if (current_char == '\0')
         {
            return lexer_context.record_error(ErrorCode::UnclosedChar);
         };

         if (current_char == '\\') {
            lexer_context.source.consume(); 

            char next = lexer_context.source.see_current();
            if (next == '\0')
            {
               return lexer_context.record_error(ErrorCode::UnclosedChar);
            }
         }  
         lexer_context.source.consume(); 

         counter++;
         current_char = lexer_context.source.see_current();
      }

      lexer_context.source.consume(); 

      if (counter == 0)
      {
         return lexer_context.record_error(ErrorCode::InvalidCharCode);
      } else if (counter == 1){
         return;
      } else if(counter > 1)
      {
         return lexer_context.record_error(ErrorCode::TooLongChar);
      };
      
      return;
   }
   
   namespace CLua {
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
               } else if (current_char == '.' && character_map[next_char] == CharacterType::Numeric)
               {
                  return TokenKind<NumericToken>::value;
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
               lexer_context.switch_consumer_mode(ConsumerMode::LuaUCapture);
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

   namespace LuaUCapture {
      using CLua::guess_token_type;

      void get_next_token(LexerContext& lexer_context, TokenType token_type){
         switch (token_type)
         {
         case TokenType::Symbol:
         {
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
         }
         default:
            CLua::get_next_token(lexer_context,token_type);
         }

         if (lexer_context.luau_capture_state.brace_balance == 0 && lexer_context.luau_capture_state.met_first_brace)
         {
            lexer_context.switch_consumer_mode(ConsumerMode::LuaU);
         };
      }
   };

   namespace LuaUCode {

      bool is_valid_lua_block(LexerContext& lexer_context,size_t peek_offset)
      {
         auto current_char = lexer_context.source.peek(peek_offset);

         while (current_char == '=')
         {
            peek_offset++;
            current_char = lexer_context.source.peek(peek_offset);
         };

         return current_char == '[';
      };

      bool is_lua_string(LexerContext& lexer_context)
      {
         auto current_char = lexer_context.source.see_current();
         auto is_not_string_start = current_char != '\'' && current_char != '"' && current_char != '[' && current_char != '`';

         if (is_not_string_start)
         {
            return false;
         };

         auto next_char = lexer_context.source.peek();

         if (current_char == '[' && is_valid_lua_block(lexer_context,1)) 
         {
            return true; 
         }  else if(current_char != '[') [[likely]] {
            return true;
         };

         return false;
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
         Other,
         Error,
         EndOfFile,
         None
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

         luau_char_type_map[static_cast<unsigned char>('{')] = LuaUCharType::LBracket;
         luau_char_type_map[static_cast<unsigned char>('}')] = LuaUCharType::RBracket;

         luau_char_type_map[static_cast<unsigned char>('\0')] = LuaUCharType::EndOfFile;
         
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

         return LuaUTokenType::None;
      };

      /// @brief it also consumes while asserting on the note but I don't know how to call this function then otherwise
      /// @param lexer_context 
      /// @return 
      void assert_is_lua_comment(LexerContext &lexer_context)
      {
         Assert(
            lexer_context.source.see_current() == '-' && lexer_context.source.peek() == '-',
            LexerError +
            "expected comment mode character, got something else"s +
            LexerErrorEnd
         )
         lexer_context.source.consume(2);
      };    

      bool process_end_of_lua_block_token(LexerContext& lexer_context,size_t equal_sign_count)
      {
         size_t equal_signs_in_row = 0;

         Assert(
            lexer_context.source.see_current() == ']',
            LexerError +
            "assertion broken, expected comment closing character, got something else instead"s +
            LexerErrorEnd
         )

         lexer_context.source.consume();

         auto current_char = lexer_context.source.see_current();
         while (current_char == '=')
         {
            equal_signs_in_row++;
            lexer_context.source.consume();
            current_char = lexer_context.source.see_current();
         };

         if (current_char != ']')
         {
            return false;
         }

         lexer_context.source.consume();
         return equal_signs_in_row == equal_sign_count;
      };

      bool process_is_lua_block(LexerContext& lexer_context,size_t& equal_sign_count)
      {  
         if (lexer_context.source.see_current() != '[')
         {
            return false;
         };

         while (lexer_context.source.peek(equal_sign_count + 1) == '='){
            equal_sign_count++;
         };

         lexer_context.source.consume(equal_sign_count + 1);
         
         if (lexer_context.source.see_current() != '[')
         {
            return false;
         };

         lexer_context.source.consume();
         return true;
      };

      void consume_lua_block_token(LexerContext& lexer_context,size_t equal_sign_count)
      {

         auto current_char = lexer_context.source.see_current();
         auto char_type = character_map[current_char];

         while (char_type != CharacterType::EndOfFile)
         {
            if (current_char == ']' && process_end_of_lua_block_token(lexer_context,equal_sign_count))
            {
               return;
            } else {
               lexer_context.source.consume();
            };

            current_char = lexer_context.source.see_current();
            char_type = character_map[current_char];
         };
         
         return lexer_context.record_error(ErrorCode::UnclosedLuaBlock);
      };

      void consume_lua_basic_string_token(LexerContext& lexer_context)
      {
         auto start_char = lexer_context.source.see_current();

         Assert(
            start_char == '\'' ||
            start_char == '"' ||
            start_char == '`',
            LexerError + 
            "assertion broken, the tested char is not a string start char"s +
            LexerErrorEnd
         )

         lexer_context.source.consume();
         auto current_char = lexer_context.source.see_current();
         while (character_map[current_char] != CharacterType::EndOfFile)
         {
            if (current_char == start_char)
            {
               lexer_context.source.consume();
               break;
            };
            if (current_char == '\\')
            {
               lexer_context.source.consume();
               auto next_char = lexer_context.source.see_current(); 
               if (character_map[next_char] == CharacterType::EndOfFile)
               {
                  return lexer_context.record_error(ErrorCode::UnclosedLuaBlock);
                  //more specifically string is not closed properly but it's LuaU's VM problem
               };
            };
            lexer_context.source.consume();
            current_char = lexer_context.source.see_current();
         };
      };

      void consume_lua_string_token(LexerContext& lexer_context)
      {
         size_t equal_sign_count = 0;
         if (process_is_lua_block(lexer_context,equal_sign_count))
         {
            return consume_lua_block_token(lexer_context,equal_sign_count);
         } else {
            return consume_lua_basic_string_token(lexer_context);
         };
      };

      void consume_lua_inline_comment_token(LexerContext& lexer_context)
      {
         auto char_type = character_map[lexer_context.source.see_current()];
         while (char_type != CharacterType::NewLine)
         {
            if (char_type == CharacterType::EndOfFile)
            {
               return lexer_context.record_error(ErrorCode::UnclosedLuaBlock);
            };
            lexer_context.source.consume();
         };
      };

      void consume_lua_comment_token(LexerContext& lexer_context)
      {
         size_t equal_sign_count = 0;
         assert_is_lua_comment(lexer_context);
         if (process_is_lua_block(lexer_context,equal_sign_count))
         {
            return consume_lua_block_token(lexer_context,equal_sign_count);
         } else {
            /*
               Assert(
                  equal_sign_count == 0,
                  LexerError +
                  "unexpected state where equal sign count is non-zero, where it should be"s +  
                  LexerErrorEnd
               )

               //ignore byproduct as it's not harmful
            */
            return consume_lua_inline_comment_token(lexer_context);
         }
      };

      void consume_lua_other_token(LexerContext& lexer_context)
      {
         lexer_context.source.consume();
      };

      void consume_l_bracket(LexerContext& lexer_context)
      {
         Assert(
            lexer_context.source.see_current() == '{',
            LexerError +
            "expected { token, got something else instead"s + 
            LexerErrorEnd
         )

         auto& brace_balance = lexer_context.luau_code_state.brace_balance;
         brace_balance++;

         lexer_context.source.consume();
      };

      void consume_r_bracket(LexerContext& lexer_context)
      {
         Assert(
            lexer_context.source.see_current() == '}',
            LexerError +
            "expected } token, got something else instead"s + 
            LexerErrorEnd
         )

         auto& brace_balance = lexer_context.luau_code_state.brace_balance;
         auto& met_first_brace = lexer_context.luau_code_state.met_first_brace;
         if (brace_balance <= 0)
         {
            //invalid state, error should be reported
            return lexer_context.record_error(ErrorCode::UnexpectedTokenType); //could be something more specific but no ideas   
         } 
         brace_balance--;   
         lexer_context.source.consume();
      };

      void consume_lua_block(LexerContext& lexer_context)
      {
         auto& brace_balance = lexer_context.luau_code_state.brace_balance;
         do {
            auto luau_token_type = guess_luau_token_type(lexer_context);

            switch (luau_token_type)
            {
            case LuaUTokenType::Comment:
               consume_lua_comment_token(lexer_context);
               break;
            case LuaUTokenType::String:
               consume_lua_string_token(lexer_context);
               break;
            case LuaUTokenType::LBracket:
               consume_l_bracket(lexer_context);
               break;
            case LuaUTokenType::RBracket:
               consume_r_bracket(lexer_context);
               break;
            case LuaUTokenType::Other:
               consume_lua_other_token(lexer_context);
               break;
            case LuaUTokenType::Error:
               consume_error_token(lexer_context);
            case LuaUTokenType::EndOfFile:
               return lexer_context.record_error(ErrorCode::UnclosedLuaBlock);
            default:
               consume_unexpected_token(lexer_context);
               break;
            }
         } while (brace_balance != 0);

         return;
      };

      TokenType guess_token_type(LexerContext& lexer_context)
      {
         auto current_char = static_cast<unsigned char>(lexer_context.source.see_current());
         auto character_type = character_map[current_char];

         switch (character_type)
         {
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
            default:
               return CLua::guess_token_type(lexer_context);
         };

         return TokenKind<NoToken>::value;
      };

      TokenType process_next_token(LexerContext& lexer_context){
         TokenType token_type = guess_token_type(lexer_context);
         lexer_context.original_token_type = token_type;
         lexer_context.ultimate_token_type = token_type;
         
         switch (token_type)
         {
         case TokenType::LuaBlock:
            consume_lua_block(lexer_context);
            lexer_context.switch_consumer_mode(ConsumerMode::CLua);
            break;
         case TokenType::Whitespace:
            CLua::get_next_token(lexer_context,TokenType::Whitespace);
            break;
         case TokenType::None:
            CLua::get_next_token(lexer_context,TokenType::None);
         case TokenType::Error:
            CLua::get_next_token(lexer_context,TokenType::Error);
            break;
         default:
            consume_unexpected_token(lexer_context);
            break;
         };

         return token_type;
      };
   };

   TokenGeneric Lexer::get_next_token()
   {
      auto token_type = TokenType::None;
      
      size_t start = lexer_context.source.index;
      
      switch (lexer_context.see_current_consumer_mode())
      {
      case ConsumerMode::CLua:
         token_type = CLua::guess_token_type(lexer_context);
         lexer_context.original_token_type = token_type;
         lexer_context.ultimate_token_type = token_type;
         CLua::get_next_token(lexer_context,token_type);
         break;
      case ConsumerMode::LuaUCapture:
         token_type = LuaUCapture::guess_token_type(lexer_context);
         lexer_context.original_token_type = token_type;
         lexer_context.ultimate_token_type = token_type;
         LuaUCapture::get_next_token(lexer_context,token_type);
         break;
      case ConsumerMode::LuaU:
         token_type = LuaUCode::process_next_token(lexer_context);
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