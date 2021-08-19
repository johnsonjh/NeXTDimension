	.file	"alloca.s"
	.text
	.align	4
// 
//	char *alloca(size)
//
//	Allocate size bytes in the calling function's stack frame (will 
//	be automatically cleaned up when that function exits).  The area
//	(and the stack pointer) will be mod 16 aligned on return.
//	
//	WARNING: the calling function must not access local variables
//	off the stack pointer after this call - it must use the frame
//	pointer instead.
//
	.globl	_alloca
_alloca: 
	subs	sp,r16,r16
	andnot	15,r16,r16
	bri	r1
	mov	r16,sp
