.code

; ============================================================
; Procedure: ToneMapAVX2
; Author: Jakub Hanusiak
; Date: 5 sem, 2026-01-21
; Topic: Tone Mappping
;
; Description:
; Performs HDR tone mapping using the Extended Reinhard operator
; on a planar RGB float buffer. The procedure uses AVX2 vector
; instructions for processing 8 pixels at a time, with a scalar
; fallback loop for remaining pixels.
;
; The tone mapping is luminance-based and preserves chroma.
;
; ------------------------------------------------------------
; Input parameters (Windows x64 calling convention):
;
; rcx  - Pointer to combined planar float buffer:
;        [RRR... GGG... BBB...]
;
; rdx  - Number of pixels (n)
;        Range: n > 0
;
; xmm2 - Exposure value
;        Range: > 0.0
;
; xmm3 - White point value
;        Range: > 0.0
;
; ------------------------------------------------------------
; Output parameters:
;
; The RGB data is modified in place inside the input buffer.
;
; ============================================================

ToneMapAVX2 PROC

    ; --------------------------------------------------
    ; Prologue
    ; --------------------------------------------------
    ; Save non-volatile registers used by this procedure
    push rbx
    push rsi
    push rdi

    ; Allocate stack space (shadow space / alignment)
    sub  rsp, 32

    ; Initialize pixel index counter
    xor  rbx, rbx              ; rbx = 0

    ; --------------------------------------------------
    ; Save scalar parameters for scalar tail loop
    ; --------------------------------------------------
    movss DWORD PTR [ExposureConst], xmm2     ; store exposure
    movss DWORD PTR [WhitePointConst], xmm3   ; store whitePoint

    ; --------------------------------------------------
    ; Compute planar channel pointers
    ; --------------------------------------------------
    mov     rsi, rcx            ; rsi -> R channel base
    lea     rdi, [rcx + rdx*4]  ; rdi -> G channel base
    lea     r10, [rdi + rdx*4]  ; r10 -> B channel base

    ; --------------------------------------------------
    ; Broadcast constants to YMM registers
    ; --------------------------------------------------
    vbroadcastss ymm10, xmm2                ; exposure
    vbroadcastss ymm11, xmm3                ; whitePoint
    vbroadcastss ymm12, DWORD PTR [LumaR]   ; luminance R weight
    vbroadcastss ymm13, DWORD PTR [LumaG]   ; luminance G weight
    vbroadcastss ymm14, DWORD PTR [LumaB]   ; luminance B weight
    vbroadcastss ymm15, DWORD PTR [One]     ; constant 1.0
    vbroadcastss ymm9,  DWORD PTR [EPS]     ; epsilon

    ; Compute squared white point: wp²
    vmaxps ymm11, ymm11, ymm9               ; clamp whitePoint
    vmulps ymm11, ymm11, ymm11              ; wp²

; ==================================================
; Vector loop (processes 8 pixels per iteration)
; ==================================================
MainLoop:
    ; Check remaining pixels
    mov  rax, rdx
    sub  rax, rbx
    cmp  rax, 8
    jb   ScalarTail

    ; --------------------------------------------------
    ; Load R, G, B values
    ; --------------------------------------------------
    vmovups ymm0, [rsi + rbx*4]   ; R
    vmovups ymm1, [rdi + rbx*4]   ; G
    vmovups ymm2, [r10 + rbx*4]   ; B

    ; --------------------------------------------------
    ; Apply exposure
    ; --------------------------------------------------
    vmulps ymm0, ymm0, ymm10
    vmulps ymm1, ymm1, ymm10
    vmulps ymm2, ymm2, ymm10

    ; --------------------------------------------------
    ; Compute luminance L'
    ; L' = R*0.2126 + G*0.7152 + B*0.0722
    ; --------------------------------------------------
    vmulps ymm3, ymm0, ymm12
    vfmadd231ps ymm3, ymm1, ymm13
    vfmadd231ps ymm3, ymm2, ymm14

    vmovaps ymm5, ymm3           ; save L'

    ; --------------------------------------------------
    ; Extended Reinhard tone mapping
    ; Lmapped = L' * (1 + L'/wp²) / (1 + L')
    ; --------------------------------------------------
    vdivps ymm4, ymm3, ymm11     ; L'/wp²
    vaddps ymm4, ymm4, ymm15     ; 1 + L'/wp²
    vmulps ymm4, ymm4, ymm3      ; L' * (1 + L'/wp²)
    vaddps ymm3, ymm3, ymm15     ; 1 + L'
    vdivps ymm4, ymm4, ymm3      ; Lmapped

    ; --------------------------------------------------
    ; Compute scale factor
    ; scale = Lmapped / max(L', eps)
    ; --------------------------------------------------
    vmaxps ymm5, ymm5, ymm9
    vdivps ymm4, ymm4, ymm5

    ; --------------------------------------------------
    ; Apply scale to RGB
    ; --------------------------------------------------
    vmulps ymm0, ymm0, ymm4
    vmulps ymm1, ymm1, ymm4
    vmulps ymm2, ymm2, ymm4

    ; Clamp to epsilon
    vmaxps ymm0, ymm0, ymm9
    vmaxps ymm1, ymm1, ymm9
    vmaxps ymm2, ymm2, ymm9

    ; Store results
    vmovups [rsi + rbx*4], ymm0
    vmovups [rdi + rbx*4], ymm1
    vmovups [r10 + rbx*4], ymm2

    add rbx, 8
    jmp MainLoop

; ==================================================
; Scalar tail loop (remaining pixels)
; ==================================================
ScalarTail:
    cmp rbx, rdx
    jae Done

    ; Load R, G, B
    movss xmm0, DWORD PTR [rsi + rbx*4]
    movss xmm1, DWORD PTR [rdi + rbx*4]
    movss xmm2, DWORD PTR [r10 + rbx*4]

    ; Apply exposure
    movss xmm6, DWORD PTR [ExposureConst]
    mulss xmm0, xmm6
    mulss xmm1, xmm6
    mulss xmm2, xmm6

    ; Compute luminance
    movss xmm3, xmm0
    mulss xmm3, DWORD PTR [LumaR]
    movss xmm4, xmm1
    mulss xmm4, DWORD PTR [LumaG]
    addss xmm3, xmm4
    movss xmm4, xmm2
    mulss xmm4, DWORD PTR [LumaB]
    addss xmm3, xmm4              ; L'

    movss xmm7, xmm3              ; save L'

    ; Compute wp²
    movss xmm4, DWORD PTR [WhitePointConst]
    maxss xmm4, DWORD PTR [EPS]
    mulss xmm4, xmm4

    ; Extended Reinhard
    movss xmm5, xmm3
    divss xmm5, xmm4
    addss xmm5, DWORD PTR [One]
    mulss xmm5, xmm3
    addss xmm3, DWORD PTR [One]
    divss xmm5, xmm3

    ; Scale factor
    maxss xmm7, DWORD PTR [EPS]
    divss xmm5, xmm7

    ; Apply scale
    mulss xmm0, xmm5
    mulss xmm1, xmm5
    mulss xmm2, xmm5

    ; Clamp
    maxss xmm0, DWORD PTR [EPS]
    maxss xmm1, DWORD PTR [EPS]
    maxss xmm2, DWORD PTR [EPS]

    ; Store results
    movss DWORD PTR [rsi + rbx*4], xmm0
    movss DWORD PTR [rdi + rbx*4], xmm1
    movss DWORD PTR [r10 + rbx*4], xmm2

    inc rbx
    jmp ScalarTail

; ==================================================
; Procedure exit
; ==================================================
Done:
    add rsp, 32
    pop rdi
    pop rsi
    pop rbx
    ret

ToneMapAVX2 ENDP

; ==================================================
; Constants and static data
; ==================================================
.data
ALIGN 16

; Rec.709 luminance coefficients
LumaR REAL4 0.2126
LumaG REAL4 0.7152
LumaB REAL4 0.0722

; Small epsilon to avoid division by zero
EPS   REAL4 0.0001

; Constant value 1.0
One   REAL4 1.0

; Saved scalar exposure value
ExposureConst   REAL4 0.0

; Saved scalar white point value
WhitePointConst REAL4 0.0

END
