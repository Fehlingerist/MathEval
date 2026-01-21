#include <math-ast/math-ast.hpp>
#include <lexer/lexer.hpp>
#include <iostream>
#include <string>

int main()
{
    using namespace AST;
    std::cout << "Write a math expression: " << std::endl;
    
    std::string input;
    
    if (!std::getline(std::cin, input) || input.empty()) {
        std::cerr << "No input provided." << std::endl;
        return 1;
    }

    Util::Source source((unsigned char *)input.data(),input.length());

    MathEvaluator math_eval;
    math_eval.eval(source);

    return 0;
}