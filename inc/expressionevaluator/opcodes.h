
#pragma once

#include <QObject>

namespace ExpressionEvaluator {
Q_NAMESPACE

/**
 * @brief ProbeScope implements a stack machine and uses that to evaluate expressions. Expressions are parsed by a
 * tree-sitter based parser, and then an instruction stream for the stack machine is generated. The stack machine works
 * with the symbol backend and instructs the backend to read data.
 *
 * This bytecode virtual machine has:
 * - A stack that operates on 64-bit integer words
 * - A "BaseType" Type register that represents the type of the symbolic "Base" variable
 * - A "BaseScope" Scope register that represents a scope object in which "Base" variable can be loaded from
 * - A "Type" Type register that represents a type object that is in the construction progress
 * - A "TypeScope" Scope register that represents a scope object in which "TempType" can be loaded from
 * - A "Flags" register that remembers state of current evaluation.
 *
 * BaseType can be cast into TempType, certain rules apply. Some operations that require dereferencing pointers will
 * have to block until the memory access completes.
 */
enum Opcode {
    Nop = 0,
    LoadI16 = 1, // IMM: INT $a. Push an SignExtend(I16) encoded in instruction stream onto stack.
    LoadU16,     // IMM: INT $a. Push an ZeroExtend(U16) encoded in instruction stream onto stack.
    LoadI32,     // IMM: INT $a. Push an SignExtend(I32) onto stack
    LoadU32,     // IMM: INT $a. Push an ZeroExtend(U32) onto stack
    LoadI64,     // IMM: INT $a. Push an I64 onto stack
    LoadU64,     // IMM: INT $a. Push an U64 onto stack

    LoadBase = 0x10, // IMM: STR $a. Find a variable named $a, and push its address onstack, store its type in BaseType.
    BaseResetScope,  // IMM: NONE. Reset BaseScope to the root level scope.
    BaseLoadScope,   // IMM: STR $a. Find a scope named $a in current BaseScope and replace BaseScope with this scope.
    BaseCast,        // IMM: NONE. Replace BaseType with Type. Certain rules apply: Pointer <-> Array;
                     // Integer <-> Pointer; Bare structural types are non castable.
    BaseDeref, // IMM: NONE. Apply one dereference operation on the BaseType and replaces it with the dereferenced type.
               // If current BaseType is not Pointer or Array modified, an error is raised.
    BaseMember, // IMM: STR $a. Pop one element, add the offset of BaseType's member $a, and push back onto stack, and
                // replace BaseType with BaseType's member $a's type. For bitfields, the member value extraction code is
                // also pushed and the base type is changed to bitfield type, this is because we cannot represent
                // bitfield types in this system (and there's no need to do so either, if you've reached a bitfield
                // member you can't do anything further on it) so we automatically does the BaseMemberEval.
    BaseEval,   // IMM: NONE. When BaseMember was executed we only have the address of the member on the stack and its
                // type in BaseType register. With this opcode we'll evaluate the member and get its value. Note that if
                // BaseMember access got a bitfield type, BaseEval is automatically performed in static optimization.

    TypeResetScope = 0x20, // IMM: NONE. Reset TypeScope to the root level scope.
    TypeLoadScope, // IMM: STR $a. Find a scope named $a in current TypeScope and replace TypeScope with this scope.
    TypeLoadType,  // IMM: STR $a. Find a type named $a in current TypeScope and replace Type with this type.
    TypeModifyPtr, // IMM: NONE. Add a layer of Pointer Modifier to the current Type.
    TypeModifyArr, // IMM: NONE. Add a layer of Array Modifier to the current Type.

    TypeLoadU8 = 0x30, // IMM: NONE. Replace Type with internal integer type Uint8.
    TypeLoadU16,       // IMM: NONE. Replace Type with internal integer type Uint16.
    TypeLoadU32,       // IMM: NONE. Replace Type with internal integer type Uint32.
    TypeLoadU64,       // IMM: NONE. Replace Type with internal integer type Uint64.
    TypeLoadI8,        // IMM: NONE. Replace Type with internal integer type Sint8.
    TypeLoadI16,       // IMM: NONE. Replace Type with internal integer type Sint16.
    TypeLoadI32,       // IMM: NONE. Replace Type with internal integer type Sint32.
    TypeLoadI64,       // IMM: NONE. Replace Type with internal integer type Sint64.
    TypeLoadF32,       // IMM: NONE. Replace Type with internal integer type Float32.
    TypeLoadF64,       // IMM: NONE. Replace Type with internal integer type Float64.

    AddI16 = 0x40, // IMM: INT $a. Pop one element and add with SignExtendTo64($a) encoded in instruction stream, and
                   // push result onto the stack.
    AddI32,        // IMM: INT $a. Pop one element and add with SignExtendTo64($a), and push result onto the stack.
    AddI64,        // IMM: INT $a. Pop one element and add with $a, and push result onto the stack.
    MulI16, // IMM: INT $a. Pop one element and multiply with SignExtendTo64($a) encoded in instruction stream, and push
            // result onto the stack.
    MulI32, // IMM: INT $a. Pop one element and multiply with SignExtendTo64($a), and push result onto the stack.
    MulI64, // IMM: INT $a. Pop one element and multiply with $a, and push result onto the stack.
    OffsetI16, // IMM: INT $a. Pop one element and encoded-in-instruction-stream offset $a * siezof(Deref(BaseType)),
               // and push result onto the stack. If BaseType is not pointer or array type, an error is raised.
    OffsetI32, // IMM: INT $a. Pop one element and offset $a * siezof(Deref(BaseType)), and push result onto the stack.
               // If BaseType is not pointer or array type, an error is raised.
    OffsetI64, // IMM: INT $a. Pop one element and offset $a * siezof(Deref(BaseType)), and push result onto the stack.
               // If BaseType is not pointer or array type, an error is raised.

    Deref8 = 0x80, // IMM: NONE. Pop one element as address, read 1 byte of device memory there and push it onto stack.
    Deref16,       // IMM: NONE. Pop one element as address, read 2 bytes of device memory there and push it onto stack.
    Deref32,       // IMM: NONE. Pop one element as address, read 4 bytes of device memory there and push it onto stack.
    Deref64,       // IMM: NONE. Pop one element as address, read 8 bytes of device memory there and push it onto stack.

    ReturnAsBase = 0xD0, // IMM: NONE. Pop one element and end execution with this element as result in BaseType.
    ReturnU8,  // IMM: NONE. Pop one element and end execution with this element as the execution result in U8.
    ReturnU16, // IMM: NONE. Pop one element and end execution with this element as the execution result in U16.
    ReturnU32, // IMM: NONE. Pop one element and end execution with this element as the execution result in U32.
    ReturnU64, // IMM: NONE. Pop one element and end execution with this element as the execution result in U64.
    ReturnI8,  // IMM: NONE. Pop one element and end execution with this element as the execution result in I8.
    ReturnI16, // IMM: NONE. Pop one element and end execution with this element as the execution result in I16.
    ReturnI32, // IMM: NONE. Pop one element and end execution with this element as the execution result in I32.
    ReturnI64, // IMM: NONE. Pop one element and end execution with this element as the execution result in I64.
    ReturnF32, // IMM: NONE. Pop one element and end execution with this element as the execution result in F32.
    ReturnF64, // IMM: NONE. Pop one element and end execution with this element as the execution result in F64.

    SingleEvalBegin = 0xE0, // IMM: NONE. Marks the beginning of Single Evaluation Block.
    SingleEvalEnd,          // IMM: NONE. Marks the ending of Single Evaluation Block.
    LogicalShiftRight,      // IMM: INT $a. Pop one element, logical shift right $a bits, and push back onto stack.
    MaskBitsZeroExtend, // IMM: INT $a. Pop one element, mask out all bits other than lowest $a bits and push back onto
                        // stack.
    MaskBitsSignExtend, // IMM: INT $a. Pop one element, mask out all bits other than lowest $a bits and sign extend to
                        // 64-bit, and push back onto stack.

    MaxOpcodes = 0x100, // No opcodes can be allocated beyond 0xFF
    MetaLoadInt,        // IMM: INT $a. Meta-opcode used when calling Bytecode::pushInstruction.
    MetaAddInt,         // IMM: INT $a. Meta-opcode used when calling Bytecode::pushInstruction.
    MetaMulInt,         // IMM: INT $a. Meta-opcode used when calling Bytecode::pushInstruction.
    MetaOffsetInt,      // IMM: INT $a. Meta-opcode used when calling Bytecode::pushInstruction.
};

Q_ENUM_NS(Opcode);

} // namespace ExpressionEvaluator
