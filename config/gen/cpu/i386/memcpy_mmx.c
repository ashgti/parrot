/*
 * DO NOT EDIT THIS FILE
 *
 * Generated from memcpy_mmx_in.c via memcpy_mmx_in.s
 * by '../../../../tools/dev/as2c.pl memcpy_mmx_in'
 */

/*
 * GAS LISTING /tmp/ccgXFVbl.s 			page 1
 * 
 * 
 *    1              		.file	"memcpy_mmx_in.c"
 *    2              		.version	"01.01"
 *    3              	gcc2_compiled.:
 *    4              	.text
 *    5              		.align 16
 *    6              	.globl Parrot_memcpy_aligned_mmx
 *
 */
static const char Parrot_memcpy_aligned_mmx_code[] = {
                           /* Parrot_memcpy_aligned_mmx: */
    0x55,                       /* pushl %ebp */
    0x89, 0xE5,                 /* movl %esp,%ebp */
    0x83, 0xEC, 0x1C,           /* subl $28,%esp */
    0x57,                       /* pushl %edi */
    0x56,                       /* pushl %esi */
    0x53,                       /* pushl %ebx */
    0x8B, 0x5D, 0x08,           /* movl 8(%ebp),%ebx */
    0x8B, 0x55, 0x0C,           /* movl 12(%ebp),%edx */
    0x8B, 0x45, 0x10,           /* movl 16(%ebp),%eax */
    0xDD, 0x5D, 0xF8,           /* fstpl -8(%ebp) */
    0x89, 0xDF,                 /* mov %ebx, %edi */
    0x89, 0xD6,                 /* mov %edx, %esi */
    0x89, 0xC1,                 /* mov %eax, %ecx */
    0xC1, 0xE9, 0x04,           /* shr $4, %ecx */
    0x89, 0xF6,                 /* .p2align 3 */
                           /* 1: */
    0x0F, 0x6F, 0x46, 0x00,     /* movq 0(%esi), %mm0 */
    0x0F, 0x6F, 0x4E, 0x08,     /* movq 8(%esi), %mm1 */
    0x83, 0xC6, 0x10,           /* add $16, %esi */
    0x0F, 0x7F, 0x47, 0x00,     /* movq %mm0, 0(%edi) */
    0x0F, 0x7F, 0x4F, 0x08,     /* movq %mm1, 8(%edi) */
    0x83, 0xC7, 0x10,           /* add $16, %edi */
    0x49,                       /* dec %ecx */
    0x75, 0xE7,                 /* jnz 1b */
    0x0F, 0x77,                 /* emms */
    0xDD, 0x45, 0xF8,           /* fldl -8(%ebp) */
    0x89, 0xD8,                 /* movl %ebx,%eax */
    0x5B,                       /* popl %ebx */
    0x5E,                       /* popl %esi */
    0x5F,                       /* popl %edi */
    0x89, 0xEC,                 /* movl %ebp,%esp */
    0x5D,                       /* popl %ebp */
    0xC3,                       /* ret */
    0x00
};

#include <stdlib.h>
typedef void* (*Parrot_memcpy_aligned_mmx_t)(void *dest, void *src, size_t);
Parrot_memcpy_aligned_mmx_t Parrot_memcpy_aligned_mmx =
    (Parrot_memcpy_aligned_mmx_t) Parrot_memcpy_aligned_mmx_code;

#ifdef PARROT_TEST
#include <string.h>
#include <stdio.h>
int main(int argc, char *argv[]) {
    unsigned char *s, *d;
    size_t i, l;

    l = 640;	/* sizeof(reg_store) */

    s = malloc(l);
    for (i = 0; i < l; ++i)
	s[i] = i & 0xff;
    d = malloc(l);
    for (i = 0; i < l; ++i)
	d[i] = 0xff;
    Parrot_memcpy_aligned_mmx(d, s, l);
    for (i = 0; i < l; ++i)
	if (d[i] != (i & 0xff)) {
	    printf("error s[%d] = %d d = %d\n", i, s[i], d[i]);
	    exit(1);
	}
    puts("ok");
    return 0;
}
#endif /* PARROT_TEST */
