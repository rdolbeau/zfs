/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright (c) 2005, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright (c) 2012, 2014 by Delphix. All rights reserved.
 * Copyright (c) 2015 AtoS <romain.dolbeau@atos.net>. All rights reserved.
 */

#if defined(_KERNEL) && defined(__x86_64__)
#include <asm/i387.h>
#include <asm/cpufeature.h>
#endif
#include <sys/zfs_context.h>
#include <sys/vdev_impl.h>
#include <sys/fs/zfs.h>
#include <sys/fm/fs/zfs.h>

#include <sys/vdev_raidz.h>

#if defined(__x86_64__) && defined(_KERNEL) && defined(CONFIG_AS_AVX512)
#define	MAKE_CST32_AVX512(regx, regz, val)				\
	asm volatile("vmovd %0,%%"#regx : : "r"(val));			\
	asm volatile("vpbroadcastd %"#regx",%"#regz)

#define	MAKE_CST8_AVX512BW(regz, val)				\
	asm volatile("vpbroadcastb %0,%%"#regz : : "r"(val))

#define	LOAD32_SRC_AVX512(src)						\
	asm volatile("vmovdqa64 %0,%%zmm0" : : "m" (*(src+0)));		\
	asm volatile("vmovdqa64 %0,%%zmm4" : : "m" (*(src+8)));		\
	asm volatile("vmovdqa64 %0,%%zmm8" : : "m" (*(src+16)));	\
	asm volatile("vmovdqa64 %0,%%zmm12" : : "m" (*(src+24)))

#define	LOAD32_SRC_AVX512BW(src)		\
	LOAD32_SRC_AVX512(src)

#define	COMPUTE32_P_AVX512(p)						\
	asm volatile("vmovdqa64 %0,%%zmm1" : : "m" (*(p+0)));		\
	asm volatile("vmovdqa64 %0,%%zmm5" : : "m" (*(p+8)));		\
	asm volatile("vmovdqa64 %0,%%zmm9" : : "m" (*(p+16)));		\
	asm volatile("vmovdqa64 %0,%%zmm13" : : "m" (*(p+24)));		\
	asm volatile("vpxorq %zmm0,%zmm1,%zmm1");			\
	asm volatile("vpxorq %zmm4,%zmm5,%zmm5");			\
	asm volatile("vpxorq %zmm8,%zmm9,%zmm9");			\
	asm volatile("vpxorq %zmm12,%zmm13,%zmm13");			\
	asm volatile("vmovdqa64 %%zmm1,%0" : "=m" (*(p+0)));		\
	asm volatile("vmovdqa64 %%zmm5,%0" : "=m" (*(p+8)));		\
	asm volatile("vmovdqa64 %%zmm9,%0" : "=m" (*(p+16)));		\
	asm volatile("vmovdqa64 %%zmm13,%0" : "=m" (*(p+24)))

#define	COMPUTE32_P_AVX512BW(p)		\
	COMPUTE32_P_AVX512(p)

#define	COMPUTE32_Q_AVX512(q)						\
	asm volatile("vmovdqa64 %0,%%zmm1" : : "m" (*(q+0)));		\
	asm volatile("vmovdqa64 %0,%%zmm5" : : "m" (*(q+8)));		\
	asm volatile("vmovdqa64 %0,%%zmm9" : : "m" (*(q+16)));		\
	asm volatile("vmovdqa64 %0,%%zmm13" : : "m" (*(q+24)));		\
	MAKE_CST32_AVX512(xmm3, zmm3, 0x80808080);			\
	asm volatile("vpandq %zmm3,%zmm1,%zmm2");			\
	asm volatile("vpandq %zmm3,%zmm5,%zmm6");			\
	asm volatile("vpandq %zmm3,%zmm9,%zmm10");			\
	asm volatile("vpandq %zmm3,%zmm13,%zmm14");			\
	asm volatile("vpsrlq $7,%zmm2,%zmm3");				\
	asm volatile("vpsrlq $7,%zmm6,%zmm7");				\
	asm volatile("vpsrlq $7,%zmm10,%zmm11");			\
	asm volatile("vpsrlq $7,%zmm14,%zmm15");			\
	asm volatile("vpsllq $1,%zmm2,%zmm2");				\
	asm volatile("vpsllq $1,%zmm6,%zmm6");				\
	asm volatile("vpsllq $1,%zmm10,%zmm10");			\
	asm volatile("vpsllq $1,%zmm14,%zmm14");			\
	asm volatile("vpsubq %zmm3,%zmm2,%zmm2");			\
	asm volatile("vpsubq %zmm7,%zmm6,%zmm6");			\
	asm volatile("vpsubq %zmm11,%zmm10,%zmm10");			\
	asm volatile("vpsubq %zmm15,%zmm14,%zmm14");			\
	asm volatile("vpsllq $1,%zmm1,%zmm1");				\
	asm volatile("vpsllq $1,%zmm5,%zmm5");				\
	asm volatile("vpsllq $1,%zmm9,%zmm9");				\
	asm volatile("vpsllq $1,%zmm13,%zmm13");			\
	MAKE_CST32_AVX512(xmm3, zmm3, 0x1d1d1d1d);			\
	MAKE_CST32_AVX512(xmm7, zmm7, 0xfefefefe);			\
	asm volatile("vpternlogd $0x6c,%zmm7,%zmm0,%zmm1");		\
	asm volatile("vpternlogd $0x6c,%zmm7,%zmm4,%zmm5");		\
	asm volatile("vpternlogd $0x6c,%zmm7,%zmm8,%zmm9");		\
	asm volatile("vpternlogd $0x6c,%zmm7,%zmm12,%zmm13");		\
	asm volatile("vpternlogd $0x78,%zmm3,%zmm2,%zmm1");		\
	asm volatile("vpternlogd $0x78,%zmm3,%zmm6,%zmm5");		\
	asm volatile("vpternlogd $0x78,%zmm3,%zmm10,%zmm9");		\
	asm volatile("vpternlogd $0x78,%zmm3,%zmm14,%zmm13");		\
	asm volatile("vmovdqa64 %%zmm1,%0" : "=m" (*(q+0)));		\
	asm volatile("vmovdqa64 %%zmm5,%0" : "=m" (*(q+8)));		\
	asm volatile("vmovdqa64 %%zmm9,%0" : "=m" (*(q+16)));		\
	asm volatile("vmovdqa64 %%zmm13,%0" : "=m" (*(q+24)))

/*
 * We can't avoid the vpmovm2b unfortunately; to be able to re-use
 * the mask directly in a vpxorb instead of a vpternlogd, we'd need
 * vpxorb... but it doesn't exist, only vpxord and vpxorq exist.
 * We could use vpblendmb, but then we'd have vpblendmb+vpxorq
 * instead of vpmovm2+vpternlogd, not a huge benefit
 */
#define	COMPUTE32_Q_AVX512BW(q)						\
	asm volatile("vmovdqa64 %0,%%zmm1" : : "m" (*(q+0)));		\
	asm volatile("vmovdqa64 %0,%%zmm5" : : "m" (*(q+8)));		\
	asm volatile("vmovdqa64 %0,%%zmm9" : : "m" (*(q+16)));		\
	asm volatile("vmovdqa64 %0,%%zmm13" : : "m" (*(q+24)));		\
	asm volatile("vpxorq %zmm11,%zmm11,%zmm11");			\
	asm volatile("vpcmpb $6,%zmm1,%zmm11,%k1");			\
	asm volatile("vpcmpb $6,%zmm5,%zmm11,%k2");			\
	asm volatile("vpcmpb $6,%zmm9,%zmm11,%k3");			\
	asm volatile("vpcmpb $6,%zmm13,%zmm11,%k4");			\
	asm volatile("vpmovm2b %k1,%zmm2");				\
	asm volatile("vpmovm2b %k2,%zmm6");				\
	asm volatile("vpmovm2b %k3,%zmm10");				\
	asm volatile("vpmovm2b %k4,%zmm14");				\
	asm volatile("vpaddb %zmm1,%zmm1,%zmm1");			\
	asm volatile("vpaddb %zmm5,%zmm5,%zmm5");			\
	asm volatile("vpaddb %zmm9,%zmm9,%zmm9");			\
	asm volatile("vpaddb %zmm13,%zmm13,%zmm13");			\
	MAKE_CST8_AVX512BW(zmm3, 0x1d);					\
	asm volatile("vpternlogd $0x78,%zmm3,%zmm2,%zmm1");		\
	asm volatile("vpternlogd $0x78,%zmm3,%zmm6,%zmm5");		\
	asm volatile("vpternlogd $0x78,%zmm3,%zmm10,%zmm9");		\
	asm volatile("vpternlogd $0x78,%zmm3,%zmm14,%zmm13");		\
	asm volatile("vpxorq %zmm0,%zmm1,%zmm1");			\
	asm volatile("vpxorq %zmm4,%zmm5,%zmm5");			\
	asm volatile("vpxorq %zmm8,%zmm9,%zmm9");			\
	asm volatile("vpxorq %zmm12,%zmm13,%zmm13");			\
	asm volatile("vmovdqa64 %%zmm1,%0" : "=m" (*(q+0)));		\
	asm volatile("vmovdqa64 %%zmm5,%0" : "=m" (*(q+8)));		\
	asm volatile("vmovdqa64 %%zmm9,%0" : "=m" (*(q+16)));		\
	asm volatile("vmovdqa64 %%zmm13,%0" : "=m" (*(q+24)))


#define	COMPUTE32_R_AVX512(r)						\
	asm volatile("vmovdqa64 %0,%%zmm1" : : "m" (*(r+0)));		\
	asm volatile("vmovdqa64 %0,%%zmm5" : : "m" (*(r+8)));		\
	asm volatile("vmovdqa64 %0,%%zmm9" : : "m" (*(r+16)));		\
	asm volatile("vmovdqa64 %0,%%zmm13" : : "m" (*(r+24)));		\
	for (j = 0; j < 2; j++) {					\
		MAKE_CST32_AVX512(xmm3, zmm3, 0x80808080);		\
		asm volatile("vpandq %zmm3,%zmm1,%zmm2");		\
		asm volatile("vpandq %zmm3,%zmm5,%zmm6");		\
		asm volatile("vpandq %zmm3,%zmm9,%zmm10");		\
		asm volatile("vpandq %zmm3,%zmm13,%zmm14");		\
		asm volatile("vpsrlq $7,%zmm2,%zmm3");			\
		asm volatile("vpsrlq $7,%zmm6,%zmm7");			\
		asm volatile("vpsrlq $7,%zmm10,%zmm11");		\
		asm volatile("vpsrlq $7,%zmm14,%zmm15");		\
		asm volatile("vpsllq $1,%zmm2,%zmm2");			\
		asm volatile("vpsllq $1,%zmm6,%zmm6");			\
		asm volatile("vpsllq $1,%zmm10,%zmm10");		\
		asm volatile("vpsllq $1,%zmm14,%zmm14");		\
		asm volatile("vpsubq %zmm3,%zmm2,%zmm2");		\
		asm volatile("vpsubq %zmm7,%zmm6,%zmm6");		\
		asm volatile("vpsubq %zmm11,%zmm10,%zmm10");		\
		asm volatile("vpsubq %zmm15,%zmm14,%zmm14");		\
		asm volatile("vpsllq $1,%zmm1,%zmm1");			\
		asm volatile("vpsllq $1,%zmm5,%zmm5");			\
		asm volatile("vpsllq $1,%zmm9,%zmm9");			\
		asm volatile("vpsllq $1,%zmm13,%zmm13");		\
		MAKE_CST32_AVX512(xmm3, zmm3, 0xfefefefe);		\
		asm volatile("vpandq %zmm3,%zmm1,%zmm1");		\
		asm volatile("vpandq %zmm3,%zmm5,%zmm5");		\
		asm volatile("vpandq %zmm3,%zmm9,%zmm9");		\
		asm volatile("vpandq %zmm3,%zmm13,%zmm13");		\
		MAKE_CST32_AVX512(xmm3, zmm3, 0x1d1d1d1d);		\
		asm volatile("vpternlogd $0x78,%zmm3,%zmm2,%zmm1");	\
		asm volatile("vpternlogd $0x78,%zmm3,%zmm6,%zmm5");	\
		asm volatile("vpternlogd $0x78,%zmm3,%zmm10,%zmm9");	\
		asm volatile("vpternlogd $0x78,%zmm3,%zmm14,%zmm13");	\
	}								\
	asm volatile("vpxorq %zmm0,%zmm1,%zmm1");			\
	asm volatile("vpxorq %zmm4,%zmm5,%zmm5");			\
	asm volatile("vpxorq %zmm8,%zmm9,%zmm9");			\
	asm volatile("vpxorq %zmm12,%zmm13,%zmm13");			\
	asm volatile("vmovdqa64 %%zmm1,%0" : "=m" (*(r+0)));		\
	asm volatile("vmovdqa64 %%zmm5,%0" : "=m" (*(r+8)));		\
	asm volatile("vmovdqa64 %%zmm9,%0" : "=m" (*(r+16)));		\
	asm volatile("vmovdqa64 %%zmm13,%0" : "=m" (*(r+24)))

#define	COMPUTE32_R_AVX512BW(r)						\
	asm volatile("vmovdqa64 %0,%%zmm1" : : "m" (*(r+0)));		\
	asm volatile("vmovdqa64 %0,%%zmm5" : : "m" (*(r+8)));		\
	asm volatile("vmovdqa64 %0,%%zmm9" : : "m" (*(r+16)));		\
	asm volatile("vmovdqa64 %0,%%zmm13" : : "m" (*(r+24)));		\
	for (j = 0; j < 2; j++) {					\
		asm volatile("vpxorq %zmm11,%zmm11,%zmm11");		\
		asm volatile("vpcmpb $6,%zmm1,%zmm11,%k1");		\
		asm volatile("vpcmpb $6,%zmm5,%zmm11,%k2");		\
		asm volatile("vpcmpb $6,%zmm9,%zmm11,%k3");		\
		asm volatile("vpcmpb $6,%zmm13,%zmm11,%k4");		\
		asm volatile("vpmovm2b %k1,%zmm2");			\
		asm volatile("vpmovm2b %k2,%zmm6");			\
		asm volatile("vpmovm2b %k3,%zmm10");			\
		asm volatile("vpmovm2b %k4,%zmm14");			\
		asm volatile("vpaddb %zmm1,%zmm1,%zmm1");		\
		asm volatile("vpaddb %zmm5,%zmm5,%zmm5");		\
		asm volatile("vpaddb %zmm9,%zmm9,%zmm9");		\
		asm volatile("vpaddb %zmm13,%zmm13,%zmm13");		\
		MAKE_CST8_AVX512BW(zmm3, 0x1d);				\
		asm volatile("vpternlogd $0x78,%zmm3,%zmm2,%zmm1");	\
		asm volatile("vpternlogd $0x78,%zmm3,%zmm6,%zmm5");	\
		asm volatile("vpternlogd $0x78,%zmm3,%zmm10,%zmm9");	\
		asm volatile("vpternlogd $0x78,%zmm3,%zmm14,%zmm13");	\
	}								\
	asm volatile("vpxorq %zmm0,%zmm1,%zmm1");			\
	asm volatile("vpxorq %zmm4,%zmm5,%zmm5");			\
	asm volatile("vpxorq %zmm8,%zmm9,%zmm9");			\
	asm volatile("vpxorq %zmm12,%zmm13,%zmm13");			\
	asm volatile("vmovdqa64 %%zmm1,%0" : "=m" (*(r+0)));		\
	asm volatile("vmovdqa64 %%zmm5,%0" : "=m" (*(r+8)));		\
	asm volatile("vmovdqa64 %%zmm9,%0" : "=m" (*(r+16)));		\
	asm volatile("vmovdqa64 %%zmm13,%0" : "=m" (*(r+24)))

static int raidz_parity_have_avx512(void) {
	/* fixme (boot_cpu_has(X86_FEATURE_AVX512)); */
	return (1);
}
static int raidz_parity_have_avx512bw(void) {
	/* fixme (boot_cpu_has(X86_FEATURE_AVX512BW)); */
	return (1);
}


#define	MAKE_P_FUN(suf, SUF, m, M)                  \
	int \
	vdev_raidz_p_##suf(const void *buf, uint64_t size, void *private)\
	{\
		struct pqr_struct *pqr = private;\
		const uint64_t *src = buf;\
		int i, cnt = size / sizeof (src[0]);\
\
		ASSERT(pqr->p && !pqr->q && !pqr->r);\
		kfpu_begin();\
		i = 0;\
		for (; i < cnt-m; i += M, src += M, pqr->p += M) {\
			LOAD##M##_SRC_##SUF(src);\
			COMPUTE##M##_P_##SUF(pqr->p);\
		}\
		for (; i < cnt; i++, src++, pqr->p++)\
			*pqr->p ^= *src;\
		kfpu_end();\
		return (0);\
	}\
\
	const struct raidz_parity_calls raidz1_##suf = {\
		vdev_raidz_p_##suf,\
		raidz_parity_have_##suf,\
		"p_"#suf\
	};\



#define	MAKE_Q_FUN(suf, SUF, m, M)                  \
	int \
	vdev_raidz_pq_##suf(const void *buf, uint64_t size, void *private)\
	{\
		struct pqr_struct *pqr = private;\
		const uint64_t *src = buf;\
		uint64_t mask;\
		int i, cnt = size / sizeof (src[0]);\
\
		ASSERT(pqr->p && pqr->q && !pqr->r);\
		kfpu_begin();\
		i = 0;\
		for (; i < cnt-m; i += M, src += M, pqr->p += M, pqr->q += M) {\
			LOAD##M##_SRC_##SUF(src);\
			COMPUTE##M##_P_##SUF(pqr->p);\
			COMPUTE##M##_Q_##SUF(pqr->q);\
		}\
		for (; i < cnt; i++, src++, pqr->p++, pqr->q++) {\
			*pqr->p ^= *src;\
			VDEV_RAIDZ_64MUL_2(*pqr->q, mask);\
			*pqr->q ^= *src;\
		}\
		kfpu_end();\
		return (0);\
	}\
\
	const struct raidz_parity_calls raidz2_##suf = {\
		vdev_raidz_pq_##suf,\
		raidz_parity_have_##suf,\
		"pq_"#suf\
	};\



#define	MAKE_R_FUN(suf, SUF, m, M)                  \
	int \
	vdev_raidz_pqr_##suf(const void *buf, uint64_t size, void *private)\
	{\
		struct pqr_struct *pqr = private;\
		const uint64_t *src = buf;\
		uint64_t mask;\
		int i, j, cnt = size / sizeof (src[0]);\
\
		ASSERT(pqr->p && pqr->q && pqr->r);\
		kfpu_begin();\
		i = 0;\
		for (; i < cnt-m; i += M, src += M, pqr->p += M,\
				pqr->q += M, pqr->r += M) {\
			LOAD##M##_SRC_##SUF(src);\
			COMPUTE##M##_P_##SUF(pqr->p);\
			COMPUTE##M##_Q_##SUF(pqr->q);\
			COMPUTE##M##_R_##SUF(pqr->r);\
		}\
		for (; i < cnt; i++, src++, pqr->p++, pqr->q++, pqr->r++) {\
			*pqr->p ^= *src;\
			VDEV_RAIDZ_64MUL_2(*pqr->q, mask);\
			*pqr->q ^= *src;\
			VDEV_RAIDZ_64MUL_4(*pqr->r, mask);\
			*pqr->r ^= *src;\
		}\
		kfpu_end();\
		return (0);\
	}\
\
	const struct raidz_parity_calls raidz3_##suf = {\
		vdev_raidz_pqr_##suf,\
		raidz_parity_have_##suf,\
		"pqr_"#suf\
	};\


#define	MAKE_FUNS(suf, SUF, m, M)			\
	MAKE_P_FUN(suf, SUF, m, M)			\
	MAKE_Q_FUN(suf, SUF, m, M)			\
	MAKE_R_FUN(suf, SUF, m, M)

#ifdef __AVX512F__
MAKE_FUNS(avx512, AVX512, 31, 32)
#ifdef __AVX512BW__
MAKE_FUNS(avx512bw, AVX512BW, 31, 32)
#endif
#endif

#endif
