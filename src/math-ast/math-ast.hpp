#pragma once

#include <lexer/lexer.hpp>
#include "token_normalizer/token_normalizer.hpp"

static_assert(
    sizeof(size_t) == sizeof(uintptr_t),
    "uintptr_t are different in size than size_t, this might lead to unexpected behaviour in code"
);

namespace AST {
    enum class MathError {
        MissingOperand,
        UnexpectedOperator,
        UnmatchedParentheses,
        EmptyExpression
    };

    enum class OperationType: uint8_t {
        None,
        Add,
        Subtract,
        Multiply,
        Divide
    };

    enum class VariableType: uint8_t {
        Undefined,
        NumberLiteral,
        UnaryExpression,
        BinaryExpression
    };

    struct Variable {
        size_t variable_ptr = 0;
        VariableType variable_type = VariableType::Undefined;
    };

    struct UnaryExpression {
        Variable operand_a;
        OperationType operation = OperationType::None;
    };

    struct BinaryExpression {
        Variable operand_a;
        Variable operand_b;

        OperationType operation = OperationType::None;
    };

    class MathEvaluator{
        public:
        Util::Source source;

        void construct_ast();
        void eval(Util::Source& source);
    };
}