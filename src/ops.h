
/*
 *  ops.h
 *  part of Atto :: https://github.com/deveah/atto
 */

#pragma once

/*  flow control */
#define ATTO_VM_OP_NOP    0x00
#define ATTO_VM_OP_CALL   0x01
#define ATTO_VM_OP_RET    0x02
#define ATTO_VM_OP_B      0x03
#define ATTO_VM_OP_BT     0x04
#define ATTO_VM_OP_BF     0x05
#define ATTO_VM_OP_CLOSE  0x06
#define ATTO_VM_OP_STOP   0x07

/*  arithmetic operations */
#define ATTO_VM_OP_ADD    0x10
#define ATTO_VM_OP_SUB    0x11
#define ATTO_VM_OP_MUL    0x12
#define ATTO_VM_OP_DIV    0x13
#define ATTO_VM_OP_ISEQ   0x18
#define ATTO_VM_OP_ISLT   0x19
#define ATTO_VM_OP_ISLET  0x1a
#define ATTO_VM_OP_ISGT   0x1b
#define ATTO_VM_OP_ISGET  0x1c

/*  boolean/symbolic operations */
#define ATTO_VM_OP_ISSEQ  0x20
#define ATTO_VM_OP_NOT    0x21
#define ATTO_VM_OP_OR     0x22
#define ATTO_VM_OP_AND    0x23
#define ATTO_VM_OP_ISNULL 0x24

/*  list operations */
#define ATTO_VM_OP_CAR    0x30
#define ATTO_VM_OP_CDR    0x31
#define ATTO_VM_OP_CONS   0x32

/*  stack operations */
#define ATTO_VM_OP_PUSHN  0x40
#define ATTO_VM_OP_PUSHS  0x41
#define ATTO_VM_OP_PUSHL  0x42
#define ATTO_VM_OP_PUSHZ  0x43

#define ATTO_VM_OP_DUP    0x48
#define ATTO_VM_OP_DROP   0x49
#define ATTO_VM_OP_SWAP   0x4a

#define ATTO_VM_OP_GETGL  0x50
#define ATTO_VM_OP_GETLC  0x51
#define ATTO_VM_OP_GETAG  0x52

