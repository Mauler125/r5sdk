#ifndef X86DEFS_H
#define X86DEFS_H

#define SETNZ 0x0F // 0F 95
#define PUSH  0x68
#define PUSH2 0x6A
#define TEST  0x85
#define EAX   0x87
#define MOV0  0x88
#define MOV1  0x89
#define MOV2  0x8B
#define LEA   0x8D
#define ECX   0x8F
#define MOV3  0xC7
#define EBX   0xCB
#define CALL  0xE8
#define MOVSS 0xF3 // F3 0F
#define NOP   0x90

#endif // X86DEFS_H
