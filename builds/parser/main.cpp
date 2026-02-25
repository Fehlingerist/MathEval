#include <parser/parser.hpp>
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

    ASTParser::Parser parser(source);

    auto value = parser.eval_math();

    std::cout << value << std::endl;

    return 0;
}