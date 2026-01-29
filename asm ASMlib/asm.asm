.code

ToneMapAVX2 PROC
    ; --------------------------------------------------
    ; Prologue
    ; --------------------------------------------------
    push rbx
    push rsi
    push rdi
    sub  rsp, 32

    xor  rbx, rbx              ; index = 0

    ; --------------------------------------------------
    ; Compute planar pointers
    ; --------------------------------------------------
    ; Parameters (Windows x64 Cdecl):
    ; rcx = combined (float*)
    ; rdx = size (n)
    ; r8  = start
    ; r9  = pixelCount
    ; xmm0 = exposure (float)
    ; xmm1 = whitePoint (float)

    lea rax, [r8*4]         ; startBytes
    lea r11, [rdx*4]        ; nBytes

    lea rsi, [rcx + rax]     ; R = base + start
    lea rdi, [rcx + r11]     ; G = base + n
    add rdi, rax             ; G += start
    lea r10, [rdi + r11]     ; B = G + n

    ; --------------------------------------------------
    ; Broadcast constants
    ; --------------------------------------------------
    vbroadcastss ymm12, DWORD PTR [LumaR]
    vbroadcastss ymm13, DWORD PTR [LumaG]
    vbroadcastss ymm14, DWORD PTR [LumaB]
    vbroadcastss ymm15, DWORD PTR [One]
    vbroadcastss ymm9,  DWORD PTR [EPS]

    ; Vector constants - exposure and whitePoint are passed in xmm0/xmm1
    vbroadcastss ymm10, xmm0        ; exposure
    vbroadcastss ymm11, xmm1        ; whitePoint
    vmaxps ymm11, ymm11, ymm9
    vmulps ymm11, ymm11, ymm11      ; wp?

; ==================================================
; Vector loop (8 pixels)
; ==================================================
MainLoop:
    mov  rax, r9        ; <-- use pixelCount (r9) as the loop bound, not total n (rdx)
    sub  rax, rbx
    cmp  rax, 8
    jb   ScalarTail

    ; Load R G B
    vmovups ymm0, [rsi + rbx*4]
    vmovups ymm1, [rdi + rbx*4]
    vmovups ymm2, [r10 + rbx*4]

    ; Exposure
    vmulps ymm0, ymm0, ymm10
    vmulps ymm1, ymm1, ymm10
    vmulps ymm2, ymm2, ymm10

    ; Luminance
    vmulps ymm3, ymm0, ymm12
    vfmadd231ps ymm3, ymm1, ymm13
    vfmadd231ps ymm3, ymm2, ymm14

    ; L' is in ymm3

    vmovaps ymm5, ymm3          ; save L'

    ; Lmapped = L' * (1 + L'/wp?) / (1 + L')
    vdivps ymm4, ymm3, ymm11
    vaddps ymm4, ymm4, ymm15
    vmulps ymm4, ymm4, ymm3
    vaddps ymm3, ymm3, ymm15
    vdivps ymm4, ymm4, ymm3     ; Lmapped

    ; scale = Lmapped / max(L', eps)
    vmaxps ymm5, ymm5, ymm9
    vdivps ymm4, ymm4, ymm5

    ; Apply scale
    vmulps ymm0, ymm0, ymm4
    vmulps ymm1, ymm1, ymm4
    vmulps ymm2, ymm2, ymm4

    ; Clamp to EPS
    vmaxps ymm0, ymm0, ymm9
    vmaxps ymm1, ymm1, ymm9
    vmaxps ymm2, ymm2, ymm9

    ; Store back
    vmovups [rsi + rbx*4], ymm0
    vmovups [rdi + rbx*4], ymm1
    vmovups [r10 + rbx*4], ymm2

    add rbx, 8
    jmp MainLoop

; ==================================================
; Scalar tail
; ==================================================
ScalarTail:
    cmp rbx, r9
    jae Done

    movss xmm0, DWORD PTR [rsi + rbx*4]
    movss xmm1, DWORD PTR [rdi + rbx*4]
    movss xmm2, DWORD PTR [r10 + rbx*4]

    ; exposure is in xmm10 (lower of ymm10), so move it into xmm3 for scalar ops
    movss xmm3, xmm10
    ; whitePoint square is in xmm11 (lower of ymm11)
    movss xmm4, xmm11

    ; exposure
    mulss xmm0, xmm3
    mulss xmm1, xmm3
    mulss xmm2, xmm3

    ; luminance
    movss xmm5, xmm0
    mulss xmm5, DWORD PTR [LumaR]
    movss xmm6, xmm1
    mulss xmm6, DWORD PTR [LumaG]
    addss xmm5, xmm6
    movss xmm6, xmm2
    mulss xmm6, DWORD PTR [LumaB]
    addss xmm5, xmm6      ; L'

    movss xmm7, xmm5      ; save L'

    ; wp?
    maxss xmm4, DWORD PTR [EPS]
    mulss xmm4, xmm4

    ; Reinhard
    divss xmm5, xmm4
    addss xmm5, DWORD PTR [One]
    mulss xmm5, xmm7
    addss xmm7, DWORD PTR [One]
    divss xmm5, xmm7      ; Lmapped

    maxss xmm7, DWORD PTR [EPS]
    divss xmm5, xmm7      ; scale

    mulss xmm0, xmm5
    mulss xmm1, xmm5
    mulss xmm2, xmm5

    movss DWORD PTR [rsi + rbx*4], xmm0
    movss DWORD PTR [rdi + rbx*4], xmm1
    movss DWORD PTR [r10 + rbx*4], xmm2

    inc rbx
    jmp ScalarTail

; ==================================================
; Exit
; ==================================================
Done:
    add rsp, 32
    pop rdi
    pop rsi
    pop rbx
    ret
ToneMapAVX2 ENDP

.data
ALIGN 16
LumaR REAL4 0.2126
LumaG REAL4 0.7152
LumaB REAL4 0.0722
EPS   REAL4 0.0001
One   REAL4 1.0
END