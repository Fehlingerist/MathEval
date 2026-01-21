#include "lexer.hpp"

#include <string>
#include <array>

/*
As a clarification here, the number parser here will be dumb and no number
validation will be performed here. It will be done at an AST level, because the number type might here be actually inside 
of a string or a comment and this makes the error irrelevant
*/

#define LexerError "Lexer Error: "
#define LexerErrorEnd "\n"

namespace Util { 
 
 inline bool is_numeric_char(char numeric_char)
 {
    return numeric_char >= '0' && numeric_char <= '9';
 };
 
 inline bool is_letter_char(char letter_char)
 {
    return (letter_char >= 'A' && letter_char <= 'Z') || (letter_char >= 'a' && letter_char <= 'z');
 };

 inline bool is_special_char(char special_char)
 {
    return ((special_char >= '!' && special_char <= '~') && !is_numeric_char(special_char) && !is_letter_char(special_char));
 };

 inline bool is_whitespace_char(char whitespace_char)
 {
    return whitespace_char == ' ' || whitespace_char == '\t' || whitespace_char == '\r' || whitespace_char == '\n';
 };

 inline bool is_unicode(char unicode_char)
 {
   return static_cast<unsigned char>(unicode_char) >= 0b10000000;
 };

 inline bool is_valid_char(char unknown_char)
 {
    return (unknown_char >= ' ' && unknown_char <= '~') || is_whitespace_char(unknown_char) || is_unicode(unknown_char) || unknown_char == '\0';
 };

 enum class CharacterType : uint8_t {
   Error,
   Letter,
   Unicode,
   Numeric,
   Whitespace,
   EndOfFile,
   Symbol, //synonymous to special character!!@#Aaaadsdąąś
 };

 static const auto character_map = [](){
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

 static const auto utf_len_map = [](){
   std::array<uint8_t, 128> map;
    
   for (int i = 0; i < 128; ++i) {
        int char_id = i + 0x80; 
        
        uint8_t length = 0;

        if (char_id >= 0xC2 && char_id <= 0xDF) {
            length = 2;
        } else if (char_id >= 0xE0 && char_id <= 0xEF) {
            length = 3;
        } else if (char_id >= 0xF0 && char_id <= 0xF4) {
            length = 4;
        }
        
        map[i] = length;
    }
    return map;
 }();

 char to_lower_case(char ascii_char)
 {
    const auto offset = 'a' - 'A';
    if (ascii_char >= 'A' && ascii_char <= 'Z')
    {
        return ascii_char + offset;
    }
    return ascii_char;
 };
 
 TokenType get_token_type(LexerContext& lexer_context)
 {
   auto current_char = static_cast<unsigned char>(lexer_context.source.see_current());
   auto character_type = character_map[current_char];

   switch (character_type)
   {
      case CharacterType::Error:
        return TokenType::Error;
      case CharacterType::Unicode:
         return TokenType::UnicodeSequence;
      case CharacterType::Letter: 
         return TokenType::Identifier;
      case CharacterType::Numeric:
         return TokenType::Numeric;
      case CharacterType::Symbol:
         return TokenType::SpecialChar;
      case CharacterType::Whitespace:
         return TokenType::Whitespace;
      case CharacterType::EndOfFile:
         return TokenType::EndOfFile;
      default:
        return TokenType::Error;
   };

   return TokenType::Error;
 };

 void consume_eof_token(LexerContext& lexer_context)
 {
   Assert(
      lexer_context.source.see_current() == '\0',
      LexerError
      "no EOF Token when expected one."
      LexerErrorEnd
   )
   Assert(
      lexer_context.source.can_consume_sentinel(),
      LexerError
      "trying to consume data outside of the buffer"
      LexerErrorEnd
   )
   lexer_context.source.consume();
 };

 void consume_error_token(LexerContext& lexer_context)
 {
   lexer_context.emit_error(ErrorCode::UnexpectedCharacter); //potentially I could add a message, but it can be derived from token of error type and displayed for the user
   lexer_context.source.consume(); 
 };

 void consume_unicode_character(LexerContext& lexer_context)
 {
   auto first = lexer_context.source.see_current();

   Assert(
      is_unicode(first),
      LexerError
      "expected unicode character but got something else instead"
      LexerErrorEnd
   )

   int length_code = utf_len_map[(unsigned int)(first - 128)]; //in release this will (should) crash if the is_unicode(first) is not true
   lexer_context.source.consume();

   if (length_code == 0)
   {
      lexer_context.emit_error(ErrorCode::InvalidByte);
      return;
   }

   for (int index = 0;index < length_code-1;index++)
   {
      if (!lexer_context.source.can_consume())
      {
         lexer_context.emit_error(ErrorCode::TruncatedSequence);
         return;
      };
      auto current_char = lexer_context.source.see_current();
      if ((current_char & 0xC0) != 0x80)
      {
         lexer_context.emit_error(ErrorCode::InvalidByte);
         return;
      }
      lexer_context.source.consume();
   };
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

   Assert(
      is_letter_char(current_char),
      LexerError
      "expected letter char, got something else instead"
      LexerErrorEnd
   )

   consume_numbers_letters(lexer_context);
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

 void consume_numeric_token_non_default_base(LexerContext& lexer_context)
 {
   auto current_char = lexer_context.source.see_current();
   auto next_char = lexer_context.source.peek();
   
   auto char_type = character_map[next_char]; 
   if (current_char == '0' && char_type == CharacterType::Letter)
   {
      lexer_context.source.consume(); // 0
      lexer_context.source.consume(); // [number base code]
      consume_numbers_letters(lexer_context);
   };
 };

 void consume_numeric_token(LexerContext& lexer_context)
 {
  auto current_char = lexer_context.source.see_current();

  Assert(is_numeric_char(current_char),
   LexerError
   "consume_numeric_token function called when current_char is not numeric"
   LexerErrorEnd
  )

  if (current_char == '0')
  {
   consume_numeric_token_non_default_base(lexer_context); //consumes the code if it exists
  };
  consume_numbers(lexer_context); 
 };

 void consume_special_token(LexerContext& lexer_context)
 {
   auto current_char = lexer_context.source.see_current();

   Assert(
      is_special_char(current_char),
      LexerError
      "expected special char"
      LexerErrorEnd
   )
   
   lexer_context.source.consume();
 };

 void consume_whitespace_token(LexerContext& lexer_context)
 {
   auto current_char = lexer_context.source.see_current();
   auto char_type = character_map[current_char]; 
   while (char_type == CharacterType::Whitespace)
   {
      lexer_context.source.consume();
      current_char = lexer_context.source.see_current();
      char_type = character_map[current_char];
   };
 };

 void consume_unicode_token(LexerContext& lexer_context)
 {
   auto current_char = lexer_context.source.see_current();
   auto char_type = character_map[current_char];
   while (char_type == CharacterType::Unicode)
   {
      consume_unicode_character(lexer_context);

      current_char = lexer_context.source.see_current();
      char_type = character_map[current_char];
   };
 };

 Token Lexer::get_next_token()
 {
   auto token_type = get_token_type(lexer_context);
   lexer_context.ultimate_token_type = token_type;
   lexer_context.original_token_type = token_type;
   size_t start = lexer_context.source.index;

   switch (token_type)
   {
   case TokenType::EndOfFile:
      consume_eof_token(lexer_context);
      break;
   case TokenType::Error:
      consume_error_token(lexer_context);
      break;
   case TokenType::Identifier:
      consume_identifier_token(lexer_context);
      break;
   case TokenType::UnicodeSequence:
      consume_unicode_token(lexer_context);
      break;
   case TokenType::Numeric:
      consume_numeric_token(lexer_context);
      break;
   case TokenType::SpecialChar:
      consume_special_token(lexer_context);
      break;
   case TokenType::Whitespace:
      consume_whitespace_token(lexer_context);
      break;
   default:
      consume_error_token(lexer_context);
      break;
   }

   size_t end = lexer_context.source.index;
   size_t length = end - start;
   Token token;
   token.token_type = lexer_context.ultimate_token_type;
   token.offset = start;
   token.length = length;

   return token;
 };
}

#undef LexerError
#undef LexerErrorEnd