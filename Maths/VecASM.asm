.code

; float DotASM(const float* a, const float* b)
; a -> rcx  (adresse du Vec3 a : [rcx+0]=x, [rcx+4]=y, [rcx+8]=z)
; b -> rdx  (adresse du Vec3 b : [rdx+0]=x, [rdx+4]=y, [rdx+8]=z)
; retour -> xmm0
DotASM PROC

    movss xmm0, dword ptr [rcx]
    movss xmm1, dword ptr [rcx+4]
    movss xmm2, dword ptr [rcx+8]

    movss xmm3, dword ptr [rdx]
    movss xmm4, dword ptr [rdx+4]
    movss xmm5, dword ptr [rdx+8]

    mulss xmm0, xmm3
    mulss xmm1, xmm4
    mulss xmm2, xmm5

    addss xmm0, xmm1
    addss xmm0, xmm2
    ret

DotASM ENDP

END
