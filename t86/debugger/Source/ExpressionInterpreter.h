#pragma once
#include <string>
#include <cstdint>
#include <variant>
#include <stack>
#include <vector>
#include <string>
#include <map>
#include "debugger/Native.h"
#include "debugger/Source/LocExpr.h"

class InterpretError : public std::exception {
public:
    InterpretError(std::string message) : message(std::move(message)) { }
    const char* what() const noexcept { return message.c_str(); }
private:
    std::string message;
};

class ExpressionInterpreter {
public:
    /// Interprets the location program and returns
    /// resulting location;
    static expr::Operand Interpret(const std::vector<expr::LocExpr>& exprs,
                                   Native& native,
                                   std::string frame_base_reg_name = "BP");
private:
    void Interpret();
    ExpressionInterpreter(const std::vector<expr::LocExpr>& exprs,
                          Native& native,
                          std::string frame_base_reg_name):
                exprs(exprs), native(native),
                frame_base_reg_name(std::move(frame_base_reg_name)) { }

    expr::Operand AddOperands(const expr::Operand& o1,
                              const expr::Operand& o2) const;

    std::stack<expr::Operand> s;
    const std::vector<expr::LocExpr>& exprs;
    Native& native;
    std::string frame_base_reg_name;
};
