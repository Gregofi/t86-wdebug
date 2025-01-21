#pragma once

#include "cpu/register.h"
#include "cpu/alu.h"
#include "instructions/operand.h"
#include "instructions/product.h"
#include "cpu/reservation_station.h"

#include <cstdint>
#include <utility>
#include <vector>
#include <stdexcept>
#include <functional>
#include <string>
#include <iostream>

class Parser;

namespace tiny::t86 {
    class Cpu;

    class Instruction {
    public:

        enum class Type {
            MOV,
            LEA,
            NOP,
            HALT,
            DBG,
            BREAK,
            ADD,
            SUB,
            INC,
            DEC,
            NEG,
            MUL,
            DIV,
            MOD,
            IMUL,
            IDIV,
            AND,
            OR,
            XOR,
            NOT,
            LSH,
            RSH,
//            LRL,
//            RRL,
            CLF,
            CMP,
            FCMP,
            JMP,
            LOOP,
            JZ,
            JNZ,
            JE,
            JNE,
            JG,
            JGE,
            JL,
            JLE,
            JA,
            JAE,
            JB,
            JBE,
            JO,
            JNO,
            JS,
            JNS,
            CALL,
            RET,
            PUSH,
            FPUSH,
            POP,
            FPOP,
            PUTCHAR,
            PUTNUM,
            GETCHAR,
            FADD,
            FSUB,
            FMUL,
            FDIV,
            EXT,
            NRW,
            BKPT,
        };

        struct Signature {
            Type type;
            std::vector<Operand::Type> operandTypes;

            bool operator<(const Signature& other) const;

            bool operator==(const Signature& other) const;

            std::string toString() const;
        };

        static std::string typeToString(Type type);

        class InvalidOperand;

        virtual ~Instruction() = default;

        virtual std::size_t length() const = 0;

        virtual bool needsAlu() const = 0;

        virtual void validate() const {}

        virtual void execute(ReservationStation::Entry& entry) const = 0;

        virtual std::vector<Operand> operands() const = 0;

        virtual void retire(ReservationStation::Entry&) const = 0;

        virtual std::vector<Product> produces() const = 0;

        Signature getSignature() const;

        virtual Instruction::Type type() const = 0;

        virtual std::vector<Operand> signatureOperands() const {
            return operands();
        };

        std::string toString() const;
    };

    /**
     * Represents invalid operand
     * For example this would arise if you called ADD PC 5 - PC should not be modified by "non-jump" instruction
     */
    class Instruction::InvalidOperand : public std::runtime_error {
    public:
        InvalidOperand(Register reg) : std::runtime_error("Invalid use of register " + reg.toString()) {}
    };

    class BinaryArithmeticInstruction : public Instruction {
    public:
        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register reg, Register val)
                : op_(std::move(op)), dest_(reg), reg_(reg), val_(val) {}

        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register reg, RegisterOffset regDisp)
                : op_(std::move(op)), dest_(reg), reg_(reg), val_(regDisp) {}

        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register reg, int64_t val)
                : op_(std::move(op)), dest_(reg), reg_(reg), val_(val) {}

        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register reg, Memory::Immediate val)
                : op_(std::move(op)), dest_(reg), reg_(reg), val_(val) {}

        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register reg, Memory::Register val)
                : op_(std::move(op)), dest_(reg), reg_(reg), val_(val) {}

        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register reg, Memory::RegisterOffset val)
                : op_(std::move(op)), dest_(reg), reg_(reg), val_(val) {}

        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register reg, Operand val)
                : op_(std::move(op)), dest_(reg), reg_(reg), val_(val) {}

        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register dest, Register reg, int64_t val)
                : op_(std::move(op)), riscLike_(true), dest_(dest), reg_(reg), val_(val) {}

        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register dest, Register reg, Register val)
                : op_(std::move(op)), riscLike_(true), dest_(dest), reg_(reg), val_(val) {}

        BinaryArithmeticInstruction(std::function<Alu::Result(int64_t, int64_t)> op, Register dest, Register reg, Operand val)
                : op_(std::move(op)), riscLike_(true), dest_(dest), reg_(reg), val_(val) {}

        bool needsAlu() const override {
            return true;
        }

        void validate() const override;

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry&) const override {}

        std::vector<Operand> signatureOperands() const override {
            if (riscLike_) {
                return {dest_, reg_, val_};
            } else {
                return {reg_, val_};
            }
        }

        std::vector<Operand> operands() const override {
            return {reg_, val_};
        }

        std::vector<Product> produces() const override {
            return {dest_, Register::Flags()};
        }

    protected:
        std::function<Alu::Result(int64_t, int64_t)> op_;

        bool riscLike_ = false;

        Register dest_;

        Register reg_;

        Operand val_;
    };

#define BIN_ARITH_INS_DECL(INS_NAME)                           \
    class INS_NAME : public BinaryArithmeticInstruction {      \
    public:                                                    \
        INS_NAME(Register reg, Register val);                  \
        INS_NAME(Register reg, RegisterOffset regDisp);        \
        INS_NAME(Register reg, int64_t val);                   \
        INS_NAME(Register reg, Memory::Immediate val);         \
        INS_NAME(Register reg, Memory::Register val);          \
        INS_NAME(Register reg, Memory::RegisterOffset val);    \
        INS_NAME(Register reg, Operand val);                   \
        INS_NAME(Register dest, Register reg, int64_t val);    \
        INS_NAME(Register dest, Register reg, Register val);   \
        Type type() const override { return Type::INS_NAME; }  \
        std::size_t length() const override;                   \
    };

    BIN_ARITH_INS_DECL(MOD)

    BIN_ARITH_INS_DECL(ADD)

    BIN_ARITH_INS_DECL(SUB)

    BIN_ARITH_INS_DECL(MUL)

    BIN_ARITH_INS_DECL(DIV)

    BIN_ARITH_INS_DECL(IMUL)

    BIN_ARITH_INS_DECL(IDIV)

    BIN_ARITH_INS_DECL(AND)

    BIN_ARITH_INS_DECL(OR)

    BIN_ARITH_INS_DECL(XOR)

    BIN_ARITH_INS_DECL(LSH)

    BIN_ARITH_INS_DECL(RSH)

    //BIN_ARITH_INS_DECL(LRL)

    //BIN_ARITH_INS_DECL(RRL)


    class FloatBinaryArithmeticInstruction : public Instruction {
    public:
        FloatBinaryArithmeticInstruction(std::function<Alu::FloatResult(double, double)> op, FloatRegister fReg, FloatRegister val)
            : op_(std::move(op)), fReg_(fReg), val_(val) {}

        FloatBinaryArithmeticInstruction(std::function<Alu::FloatResult(double, double)> op, FloatRegister fReg, double val)
            : op_(std::move(op)), fReg_(fReg), val_(val) {}

        bool needsAlu() const override {
            return true;
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry&) const override {}

        std::vector<Operand> operands() const override {
            return { fReg_, val_ };
        }

        std::vector<Product> produces() const override {
            return { fReg_, Register::Flags() };
        }

    protected:
        friend class ::Parser;
        FloatBinaryArithmeticInstruction(std::function<Alu::FloatResult(double, double)> op, FloatRegister fReg, Operand val)
            : op_(std::move(op)), fReg_(fReg), val_(val) {}

        std::function<Alu::FloatResult(double, double)> op_;

        FloatRegister fReg_;

        Operand val_;
    };

#define FLOAT_BIN_ARITH_INS_DECL(INS_NAME)                     \
    class INS_NAME : public FloatBinaryArithmeticInstruction { \
    public:                                                    \
        INS_NAME(FloatRegister reg, FloatRegister val);        \
        INS_NAME(FloatRegister reg, double val);               \
        INS_NAME(FloatRegister reg, Operand val);              \
        Type type() const override { return Type::INS_NAME; }  \
        std::size_t length() const override;                   \
    };

    FLOAT_BIN_ARITH_INS_DECL(FADD);
    FLOAT_BIN_ARITH_INS_DECL(FSUB);
    FLOAT_BIN_ARITH_INS_DECL(FMUL);
    FLOAT_BIN_ARITH_INS_DECL(FDIV);

    class UnaryArithmeticInstruction : public Instruction {
    public:
        UnaryArithmeticInstruction(std::function<Alu::Result(int64_t)> op, Register reg)
                : op_(std::move(op)), reg_(reg) {}

        bool needsAlu() const override {
            return true;
        }

        void validate() const override;

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry&) const override {}

        std::vector<Operand> operands() const override {
            return {reg_};
        }

        std::vector<Product> produces() const override {
            return {reg_, Register::Flags()};
        }

    private:
        std::function<Alu::Result(int64_t)> op_;

        Register reg_;
    };

#define UNARY_ARITH_INS_DECL(INS_NAME)                    \
class INS_NAME : public UnaryArithmeticInstruction {      \
  public:                                                 \
    INS_NAME(Register reg);                               \
    Type type() const override { return Type::INS_NAME; } \
    std::size_t length() const override;                  \
};

    UNARY_ARITH_INS_DECL(INC)

    UNARY_ARITH_INS_DECL(DEC)

    UNARY_ARITH_INS_DECL(NEG)

    UNARY_ARITH_INS_DECL(NOT)

    class NoAluInstruction : public Instruction {
    public:
        bool needsAlu() const override {
            return false;
        }
    };

    class NoOpInstruction : public NoAluInstruction {
    public:
        void execute(ReservationStation::Entry&) const override {}

        std::vector<Operand> operands() const override {
            return {};
        }

        std::vector<Product> produces() const override {
            return {};
        }
    };

    class NOP : public NoOpInstruction {
    public:
        std::size_t length() const override;

        void retire(ReservationStation::Entry&) const override {}

        Type type() const override { return Type::NOP; }
    };

    class HALT : public NoOpInstruction {
    public:
        std::size_t length() const override;

        void retire(ReservationStation::Entry& entry) const override;

        Type type() const override { return Type::HALT; }
    };

    class MOV : public Instruction {
    public:
        MOV(Register destination, int64_t value) : destination_(destination), value_(value) {}

        MOV(Register destination, Register value) : destination_(destination), value_(value) {}

        MOV(Register destination, Memory::Immediate value) : destination_(destination), value_(value) {}

        MOV(Register destination, Memory::Register value) : destination_(destination), value_(value) {}

        MOV(Register destination, Memory::RegisterOffset value) : destination_(destination), value_(value) {}

        MOV(Register destination, Memory::RegisterScaled value) : destination_(destination), value_(value) {}

        MOV(Register destination, Memory::RegisterRegister value) : destination_(destination), value_(value) {}

        MOV(Register destination, Memory::RegisterOffsetRegister value) : destination_(destination), value_(value) {}

        MOV(Register destination, Memory::RegisterRegisterScaled value) : destination_(destination), value_(value) {}

        MOV(Register destination, Memory::RegisterOffsetRegisterScaled value) : destination_(destination), value_(value) {}

        MOV(Memory::Immediate destination, int64_t value) : destination_(destination), value_(value) {}

        MOV(Memory::Immediate destination, Register value) : destination_(destination), value_(value) {}

        MOV(Memory::Register destination, int64_t value) : destination_(destination), value_(value) {}

        MOV(Memory::Register destination, Register value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterOffset destination, int64_t value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterOffset destination, Register value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterScaled destination, int64_t value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterScaled destination, Register value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterRegister destination, int64_t value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterRegister destination, Register value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterOffsetRegister destination, int64_t value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterOffsetRegister destination, Register value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterRegisterScaled destination, int64_t value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterRegisterScaled destination, Register value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterOffsetRegisterScaled destination, int64_t value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterOffsetRegisterScaled destination, Register value) : destination_(destination), value_(value) {}

        // FLOAT

        MOV(Register destination, FloatRegister value) : destination_(destination), value_(value) {}

        MOV(FloatRegister destination, double value) : destination_(destination), value_(value) {}

        MOV(FloatRegister destination, FloatRegister value) : destination_(destination), value_(value) {}

        MOV(FloatRegister destination, Register value) : destination_(destination), value_(value) {}

        MOV(FloatRegister destination, Memory::Immediate value) : destination_(destination), value_(value) {}

        MOV(FloatRegister destination, Memory::Register value) : destination_(destination), value_(value) {}

        MOV(Memory::Immediate destination, FloatRegister value) : destination_(destination), value_(value) {}

        MOV(Memory::Register destination, FloatRegister value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterOffset destination, FloatRegister value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterScaled destination, FloatRegister value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterRegister destination, FloatRegister value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterRegisterScaled destination, FloatRegister value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterOffsetRegister destination, FloatRegister value) : destination_(destination), value_(value) {}

        MOV(Memory::RegisterOffsetRegisterScaled destination, FloatRegister value) : destination_(destination), value_(value) {}

    public:
        Type type() const override { return Type::MOV; }

        std::size_t length() const override;

        void validate() const override;

        bool needsAlu() const override {
            return false;
        }

        void execute(ReservationStation::Entry& entry) const override;

        std::vector<Operand> operands() const override;

        std::vector<Operand> signatureOperands() const override {
            return {destination_, value_};
        }

        std::vector<Product> produces() const override {
            return {Product::fromOperand(destination_)};
        }

        void retire(ReservationStation::Entry& entry) const override;

    protected:

        friend class ::Parser;
        MOV(Operand destination, Operand value) : destination_(std::move(destination)), value_(std::move(value)) {}

        Operand destination_;
        Operand value_;
    };

    class CLF : public NoAluInstruction {
    public:
        Type type() const override { return Type::CLF; }

        std::size_t length() const override;

        std::vector<Operand> operands() const override {
            return {};
        }

        std::vector<Product> produces() const override {
            return { Register::Flags() };
        }

        void execute(ReservationStation::Entry&) const override {}

        void retire(ReservationStation::Entry& entry) const override;
    };

    class CMP : public Instruction {
    public:
        CMP(Register reg, int64_t value)
                : reg_(reg), value_(value) {}

        CMP(Register reg, Register value)
                : reg_(reg), value_(value) {}

        CMP(Register reg, Memory::Immediate value)
                : reg_(reg), value_(value) {}

        CMP(Register reg, Memory::Register value)
                : reg_(reg), value_(value) {}

        CMP(Register reg, Memory::RegisterOffset value)
                : reg_(reg), value_(value) {}

        bool needsAlu() const override {
            return true;
        }

        Type type() const override { return Type::CMP; }

        std::size_t length() const override;

        void execute(ReservationStation::Entry& entry) const override;

        std::vector<Operand> operands() const override {
            return { reg_, value_ };
        }

        std::vector<Product> produces() const override {
            return { Register::Flags() };
        }

        void retire(ReservationStation::Entry&) const override {}

    protected:
        friend class ::Parser;
        CMP(Register reg, Operand value)
                : reg_(reg), value_(value) {}

        Register reg_;
        Operand value_;
    };

    class FCMP : public Instruction {
    public:
        FCMP(FloatRegister fReg, double value) : fReg_(fReg), value_(value) {}

        FCMP(FloatRegister fReg, FloatRegister value) : fReg_(fReg), value_(value) {}

        bool needsAlu() const override {
            return true;
        }

        Type type() const override { return Type::FCMP; }

        std::size_t length() const override;

        void execute(ReservationStation::Entry& entry) const override;

        std::vector<Operand> operands() const override {
            return { fReg_, value_ };
        }

        std::vector<Product> produces() const override {
            return { Register::Flags() };
        }

        void retire(ReservationStation::Entry&) const override {}

    protected:
        friend class ::Parser;
        FCMP(FloatRegister fReg, Operand value) : fReg_(fReg), value_(value) {}

        FloatRegister fReg_;
        Operand value_;
    };

    class PUSH : public NoAluInstruction {
    public:
        PUSH(int64_t val) : val_(val) {}

        PUSH(Register val) : val_(val) {}

        Type type() const override { return Type::PUSH; }

        std::size_t length() const override;

        std::vector<Operand> operands() const override {
            return { val_, Register::StackPointer() };
        }

        std::vector<Operand> signatureOperands() const override {
            return { val_ };
        }

        std::vector<Product> produces() const override {
            return { Memory::Register(Register::StackPointer()), Register::StackPointer() };
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry& entry) const override;

    protected:
        friend class ::Parser;
        PUSH(Operand val) : val_(val) {}

        Operand val_;
    };

    class FPUSH : public NoAluInstruction {
    public:
        FPUSH(double val) : val_(val) {}

        FPUSH(FloatRegister fReg) : val_(fReg) {}

        Type type() const override { return Type::FPUSH; }

        std::size_t length() const override;

        std::vector<Operand> operands() const override {
            return { val_, Register::StackPointer() };
        }

        std::vector<Operand> signatureOperands() const override {
            return { val_ };
        }

        std::vector<Product> produces() const override {
            return { Memory::Register(Register::StackPointer()), Register::StackPointer() };
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry& entry) const override;

    protected:
        friend class ::Parser;
        FPUSH(Operand fReg) : val_(fReg) {}

        Operand val_;
    };

    class POP : public NoAluInstruction {
    public:
        POP(Register reg) : reg_(reg) {}

        Type type() const override { return Type::POP; }

        std::size_t length() const override;

        std::vector<Operand> operands() const override {
            return { Memory::Register{Register::StackPointer()}, Register::StackPointer() };
        }

        std::vector<Operand> signatureOperands() const override {
            return { reg_ };
        }

        std::vector<Product> produces() const override {
            return { reg_, Register::StackPointer() };
        }

        void validate() const override;

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry& entry) const override;

    private:
        Register reg_;
    };

    class FPOP : public NoAluInstruction {
    public:
        FPOP(FloatRegister fReg) : fReg_(fReg) {}

        Type type() const override { return Type::FPOP; }

        std::size_t length() const override;

        std::vector<Operand> operands() const override {
            return { Memory::Register{Register::StackPointer()}, Register::StackPointer() };
        }

        std::vector<Operand> signatureOperands() const override {
            return { fReg_ };
        }

        std::vector<Product> produces() const override {
            return { fReg_, Register::StackPointer() };
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry& entry) const override;

    protected:
        FloatRegister fReg_;
    };

    class BREAK : public NoOpInstruction {
    public:
        Type type() const override { return Type::BREAK; }

        std::size_t length() const override;

        void retire(ReservationStation::Entry& entry) const override;
    };

    class DBG : public NoOpInstruction {
    public:
        DBG(std::function<void(Cpu&)> debugFunction)
                : debugFunction_(std::move(debugFunction)) {}

        Type type() const override { return Type::DBG; }

        std::size_t length() const override;

        void retire(ReservationStation::Entry& entry) const override;

    protected:
        std::function<void(Cpu&)> debugFunction_;
    };

    class BKPT : public NoOpInstruction {
    public:
        BKPT() {}

        Type type() const override { return Type::BKPT; }

        std::size_t length() const override { return 1; }

        void retire(ReservationStation::Entry& entry) const override;
    };

    class JumpInstruction : public NoAluInstruction {
    public:
        virtual Operand getDestination() const = 0;

    };

    class PatchableJumpInstruction : public JumpInstruction {
    public:
        PatchableJumpInstruction(uint64_t address) : address_(static_cast<int64_t>(address)) {}

        PatchableJumpInstruction(Register address) : address_(address) {}

        PatchableJumpInstruction(Memory::Immediate address) : address_(address) {}

        PatchableJumpInstruction(Memory::Register address) : address_(address) {}

        PatchableJumpInstruction(Memory::RegisterOffset address) : address_(address) {}

        void setDestination(uint64_t address);

        Operand getDestination() const override {
            return address_;
        }

        std::vector<Product> produces() const override {
            return {Register::ProgramCounter()};
        }

    protected:
        friend class ::Parser;
        PatchableJumpInstruction(Operand address) : address_(address) {}

        Operand address_;
    };

    class ConditionalJumpInstruction : public PatchableJumpInstruction {
    public:
        ConditionalJumpInstruction(std::function<bool(Alu::Flags)> condition, uint64_t address)
                : PatchableJumpInstruction{address}, condition_{std::move(condition)} {}

        ConditionalJumpInstruction(std::function<bool(Alu::Flags)> condition, Register address)
                : PatchableJumpInstruction{address}, condition_{std::move(condition)} {}

        ConditionalJumpInstruction(std::function<bool(Alu::Flags)> condition, Memory::Immediate address)
                : PatchableJumpInstruction{address}, condition_{std::move(condition)} {}

        ConditionalJumpInstruction(std::function<bool(Alu::Flags)> condition, Memory::Register address)
                : PatchableJumpInstruction{address}, condition_{std::move(condition)} {}

        ConditionalJumpInstruction(std::function<bool(Alu::Flags)> condition, Memory::RegisterOffset address)
                : PatchableJumpInstruction{address}, condition_{std::move(condition)} {}

        std::vector<Operand> operands() const override {
            return { address_, Register::Flags() };
        }

        std::vector<Operand> signatureOperands() const override {
            return { address_ };
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry& entry) const override;

    protected:
        friend class ::Parser;
        ConditionalJumpInstruction(std::function<bool(Alu::Flags)> condition, Operand address)
                : PatchableJumpInstruction{address}, condition_{std::move(condition)} {}

        std::function<bool(Alu::Flags)> condition_;
    };

    class JMP : public PatchableJumpInstruction {
    public:
        JMP(Register address) : PatchableJumpInstruction(address) {}

        JMP(uint64_t address) : PatchableJumpInstruction(address) {}

        Type type() const override { return Type::JMP; }

        std::size_t length() const override;

        std::vector<Operand> operands() const override {
            return { address_ };
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry& entry) const override;
    protected:
        friend class ::Parser;
        JMP(Operand operand) : PatchableJumpInstruction(operand) {}
    };

#define COND_JMP_INS_DECL(INS_NAME)                           \
class INS_NAME : public ConditionalJumpInstruction {          \
    public:                                                   \
        INS_NAME(Register address);                           \
        INS_NAME(uint64_t address);                           \
        INS_NAME(Memory::Immediate address);                  \
        INS_NAME(Memory::Register address);                   \
        INS_NAME(Memory::RegisterOffset address);             \
        INS_NAME(Operand address);                            \
        Type type() const override { return Type::INS_NAME; } \
        std::size_t length() const override;                  \
};

    COND_JMP_INS_DECL(JZ)

    COND_JMP_INS_DECL(JNZ)

    COND_JMP_INS_DECL(JE)

    COND_JMP_INS_DECL(JNE)

    COND_JMP_INS_DECL(JG)

    COND_JMP_INS_DECL(JGE)

    COND_JMP_INS_DECL(JL)

    COND_JMP_INS_DECL(JLE)

    COND_JMP_INS_DECL(JA)

    COND_JMP_INS_DECL(JAE)

    COND_JMP_INS_DECL(JB)

    COND_JMP_INS_DECL(JBE)

    COND_JMP_INS_DECL(JO)

    COND_JMP_INS_DECL(JNO)

    COND_JMP_INS_DECL(JS)

    COND_JMP_INS_DECL(JNS)

    class LOOP : public PatchableJumpInstruction {
    public:
        LOOP(Register reg, Register address)
                : PatchableJumpInstruction{address}, reg_{reg} {}

        LOOP(Register reg, uint64_t address)
                : PatchableJumpInstruction{address}, reg_{reg} {}

        Type type() const override { return Type::LOOP; }

        std::size_t length() const override;

        bool needsAlu() const override {
            return true;
        }

        void execute(ReservationStation::Entry& entry) const override;

        std::vector<Operand> operands() const override {
            return { reg_, address_ };
        }

        std::vector<Product> produces() const override {
            return { reg_, Register::ProgramCounter(), Register::Flags() };
        }

        void retire(ReservationStation::Entry& entry) const override;
    protected:
        friend class ::Parser;
        LOOP(Operand reg, Operand address)
                : PatchableJumpInstruction{address}, reg_{reg.getRegister()} {}

        Register reg_;
    };

    class CALL : public PatchableJumpInstruction {
    public:
        CALL(Register address) : PatchableJumpInstruction{address} {}

        CALL(uint64_t address) : PatchableJumpInstruction{address} {}

        Type type() const override { return Type::CALL; }

        std::size_t length() const override;

        std::vector<Operand> operands() const override {
            return { address_, Register::ProgramCounter(), Register::StackPointer() };
        }

        std::vector<Operand> signatureOperands() const override {
            return { address_ };
        }

        std::vector<Product> produces() const override {
            return { Register::ProgramCounter(),
                     Register::StackPointer(),
                     Memory::Register{Register::StackPointer()} };
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry& entry) const override;
    protected:
        friend class ::Parser;
        CALL(Operand address) : PatchableJumpInstruction{address} {}
    };

    class RET : public JumpInstruction {
    public:
        Type type() const override { return Type::RET; }

        std::size_t length() const override;

        std::vector<Operand> operands() const override {
            return {Memory::Register{Register::StackPointer()}, Register::StackPointer()};
        }

        std::vector<Operand> signatureOperands() const override {
            return {};
        }

        std::vector<Product> produces() const override {
            return {Register::StackPointer(), Register::ProgramCounter()};
        }

        Operand getDestination() const override {
            return Memory::Register{Register::StackPointer()};
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry& entry) const override;
    };

    class LEA : public Instruction {
    public:
        LEA(Register reg, Memory::RegisterOffset mem)
            : reg_(reg), mem_(mem) {}

        LEA(Register reg, Memory::RegisterRegister mem)
            : reg_(reg), mem_(mem) {}

        LEA(Register reg, Memory::RegisterScaled mem)
            : reg_(reg), mem_(mem) {}

        LEA(Register reg, Memory::RegisterOffsetRegister mem)
            : reg_(reg), mem_(mem) {}

        LEA(Register reg, Memory::RegisterRegisterScaled mem)
            : reg_(reg), mem_(mem) {}

        LEA(Register reg, Memory::RegisterOffsetRegisterScaled mem)
            : reg_(reg), mem_(mem) {}

        Type type() const override { return Type::LEA; }

        std::size_t length() const override;

        bool needsAlu() const override {
            return false;
        }

        void validate() const override;

        std::vector<Operand> operands() const override;

        std::vector<Operand> signatureOperands() const override {
            return { reg_, mem_ };
        }

        std::vector<Product> produces() const override {
            return { reg_ };
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry&) const override {}

    protected:
        friend class ::Parser;
        LEA(Register reg, Operand mem)
            : reg_(reg), mem_(mem) {}

        Register reg_;
        Operand mem_;

    };

    class PUTCHAR : public Instruction {
    public:
        PUTCHAR(Register reg, std::ostream& os = std::cout) : reg_(reg), os_(os) {}

        Type type() const override { return Type::PUTCHAR; }

        std::size_t length() const override;

        bool needsAlu() const override {
            return false;
        }

        std::vector<Operand> operands() const override {
            return { reg_ };
        }

        std::vector<Product> produces() const override {
            return {};
        }

        void execute(ReservationStation::Entry&) const override {}

        void retire(ReservationStation::Entry& entry) const override;

    protected:
        Register reg_;

        std::ostream& os_;
    };
    
    class PUTNUM : public Instruction {
    public:
        PUTNUM(Register reg, std::ostream& os = std::cout) : reg_(reg), os_(os) {}

        Type type() const override { return Type::PUTNUM; }

        std::size_t length() const override;

        bool needsAlu() const override {
            return false;
        }

        std::vector<Operand> operands() const override {
            return { reg_ };
        }

        std::vector<Product> produces() const override {
            return {};
        }

        void execute(ReservationStation::Entry&) const override {}

        void retire(ReservationStation::Entry& entry) const override;

    protected:
        Register reg_;

        std::ostream& os_;
    };

    class GETCHAR : public Instruction {
    public:
        GETCHAR(Register reg, std::istream& is = std::cin) : reg_(reg), is_(is) {
            is_ >> std::noskipws;
        }

        Type type() const override { return Type::GETCHAR; }

        std::size_t length() const override;

        bool needsAlu() const override {
            return false;
        }

        std::vector<Operand> operands() const override {
            return {};
        }

        std::vector<Operand> signatureOperands() const override {
            return { reg_ };
        }

        std::vector<Product> produces() const override {
            return { reg_ };
        }

        void execute(ReservationStation::Entry&) const override {}

        void retire(ReservationStation::Entry& entry) const override;

    protected:
        Register reg_;

        std::istream& is_;
    };

    class EXT : public Instruction {
    public:
        EXT(FloatRegister fReg, Register reg) : fReg_(fReg), reg_(reg) {}

        Type type() const override { return Type::EXT; }

        std::size_t length() const override;

        bool needsAlu() const override {
            return true;
        }

        std::vector<Operand> operands() const override {
            return { reg_ };
        }

        std::vector<Operand> signatureOperands() const override {
            return { fReg_, reg_ };
        }

        std::vector<Product> produces() const override {
            return { fReg_ };
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry&) const override {}

    protected:
        FloatRegister fReg_;
        Register reg_;
    };

    class NRW : public Instruction {
    public:
        NRW(Register reg, FloatRegister fReg) : reg_(reg), fReg_(fReg) {}

        Type type() const override { return Type::NRW; }

        std::size_t length() const override;

        bool needsAlu() const override {
            return true;
        }

        std::vector<Operand> operands() const override {
            return { fReg_ };
        }

        std::vector<Operand> signatureOperands() const override {
            return { reg_, fReg_ };
        }

        std::vector<Product> produces() const override {
            return { reg_ };
        }

        void execute(ReservationStation::Entry& entry) const override;

        void retire(ReservationStation::Entry&) const override {}

    protected:
        Register reg_;
        FloatRegister fReg_;
    };
}
