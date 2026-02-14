#include <lexer/lexer.hpp>
#include <iostream>
#include <string>

int main()
{
    std::cout << "Write some expression: " << std::endl;
    
    std::string input;
    
    if (!std::getline(std::cin, input) || input.empty()) {
        std::cerr << "No input provided." << std::endl;
        return 1;
    }

    Util::Source source(reinterpret_cast<unsigned char*>(input.data()),input.length());

    Util::Lexer lexer(source);

    auto current_token = lexer.process_next_token();

    while (current_token.token_type != Util::TokenType::EndOfFile)
    {
        if (current_token.token_type == Util::TokenType::Error)
        {
            std::cout << "error enocuntered while interpreting the file" << std::endl;
            std::cout << "error code: " << (unsigned)lexer.get_last_error().error_code << std::endl;
        };

        std::cout << "Token Type: " << (unsigned)current_token.token_type << " " << std::string_view(input.data() + current_token.offset,current_token.length) << std::endl;
        current_token = lexer.process_next_token();
    }

    return 0;
}