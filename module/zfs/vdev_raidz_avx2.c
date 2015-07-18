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

#if defined(__x86_64__) && defined(_KERNEL) && defined(CONFIG_AS_AVX2)
#define	MAKE_CST32_AVX2(regx, regy, val)		\
	asm volatile("vmovd %0,%%"#regx : : "r"(val));	\
	asm volatile("vpbroadcastd %"#regx",%"#regy)

#define	LOAD16_SRC_AVX2(src)                                    \
	asm volatile("vmovdqa %0,%%ymm0" : : "m" (*(src+0)));	\
	asm volatile("vmovdqa %0,%%ymm4" : : "m" (*(src+4)));	\
	asm volatile("vmovdqa %0,%%ymm8" : : "m" (*(src+8)));	\
	asm volatile("vmovdqa %0,%%ymm12" : : "m" (*(src+12)))

#define	COMPUTE16_P_AVX2(p)					\
	asm volatile("vmovdqa %0,%%ymm1" : : "m" (*(p+0)));	\
	asm volatile("vmovdqa %0,%%ymm5" : : "m" (*(p+4)));	\
	asm volatile("vmovdqa %0,%%ymm9" : : "m" (*(p+8)));	\
	asm volatile("vmovdqa %0,%%ymm13" : : "m" (*(p+12)));	\
	asm volatile("vpxor %ymm0,%ymm1,%ymm1");		\
	asm volatile("vpxor %ymm4,%ymm5,%ymm5");		\
	asm volatile("vpxor %ymm8,%ymm9,%ymm9");		\
	asm volatile("vpxor %ymm12,%ymm13,%ymm13");		\
	asm volatile("vmovdqa %%ymm1,%0" : "=m" (*(p+0)));	\
	asm volatile("vmovdqa %%ymm5,%0" : "=m" (*(p+4)));	\
	asm volatile("vmovdqa %%ymm9,%0" : "=m" (*(p+8)));	\
	asm volatile("vmovdqa %%ymm13,%0" : "=m" (*(p+12)))

#define	COMPUTE16_Q_AVX2(q)						\
	asm volatile("vmovdqa %0,%%ymm1" : : "m" (*(q+0)));		\
	asm volatile("vmovdqa %0,%%ymm5" : : "m" (*(q+4)));		\
	asm volatile("vmovdqa %0,%%ymm9" : : "m" (*(q+8)));		\
	asm volatile("vmovdqa %0,%%ymm13" : : "m" (*(q+12)));		\
	MAKE_CST32_AVX2(xmm3, ymm3, 0x1d1d1d1d);			\
	asm volatile("vpxor %ymm14, %ymm14, %ymm14");			\
	asm volatile("vpcmpgtb %ymm1, %ymm14, %ymm2");			\
	asm volatile("vpcmpgtb %ymm5, %ymm14, %ymm6");			\
	asm volatile("vpcmpgtb %ymm9, %ymm14, %ymm10");			\
	asm volatile("vpcmpgtb %ymm13, %ymm14, %ymm14");		\
	asm volatile("vpaddb %ymm1,%ymm1,%ymm1");			\
	asm volatile("vpaddb %ymm5,%ymm5,%ymm5");			\
	asm volatile("vpaddb %ymm9,%ymm9,%ymm9");			\
	asm volatile("vpaddb %ymm13,%ymm13,%ymm13");			\
	asm volatile("vpand %ymm3,%ymm2,%ymm2");			\
	asm volatile("vpand %ymm3,%ymm6,%ymm6");			\
	asm volatile("vpand %ymm3,%ymm10,%ymm10");			\
	asm volatile("vpand %ymm3,%ymm14,%ymm14");			\
	asm volatile("vpxor %ymm2,%ymm1,%ymm1");			\
	asm volatile("vpxor %ymm6,%ymm5,%ymm5");			\
	asm volatile("vpxor %ymm10,%ymm9,%ymm9");			\
	asm volatile("vpxor %ymm14,%ymm13,%ymm13");			\
	asm volatile("vpxor %ymm0,%ymm1,%ymm1");			\
	asm volatile("vpxor %ymm4,%ymm5,%ymm5");			\
	asm volatile("vpxor %ymm8,%ymm9,%ymm9");			\
	asm volatile("vpxor %ymm12,%ymm13,%ymm13");			\
	asm volatile("vmovdqa %%ymm1,%0" : "=m" (*(q+0)));		\
	asm volatile("vmovdqa %%ymm5,%0" : "=m" (*(q+4)));		\
	asm volatile("vmovdqa %%ymm9,%0" : "=m" (*(q+8)));		\
	asm volatile("vmovdqa %%ymm13,%0" : "=m" (*(q+12)))

#define	COMPUTE16_R_AVX2(r)						\
	asm volatile("vmovdqa %0,%%ymm1" : : "m" (*(r+0)));		\
	asm volatile("vmovdqa %0,%%ymm5" : : "m" (*(r+4)));		\
	asm volatile("vmovdqa %0,%%ymm9" : : "m" (*(r+8)));		\
	asm volatile("vmovdqa %0,%%ymm13" : : "m" (*(r+12)));		\
	MAKE_CST32_AVX2(xmm3, ymm3, 0x1d1d1d1d);			\
	asm volatile("vpxor %ymm11, %ymm11, %ymm11");			\
	for (j = 0; j < 2; j++) {					\
		asm volatile("vpcmpgtb %ymm1, %ymm11, %ymm2");		\
		asm volatile("vpcmpgtb %ymm5, %ymm11, %ymm6");		\
		asm volatile("vpcmpgtb %ymm9, %ymm11, %ymm10");		\
		asm volatile("vpcmpgtb %ymm13, %ymm11, %ymm14");	\
		asm volatile("vpaddb %ymm1,%ymm1,%ymm1");		\
		asm volatile("vpaddb %ymm5,%ymm5,%ymm5");		\
		asm volatile("vpaddb %ymm9,%ymm9,%ymm9");		\
		asm volatile("vpaddb %ymm13,%ymm13,%ymm13");		\
		asm volatile("vpand %ymm3,%ymm2,%ymm2");		\
		asm volatile("vpand %ymm3,%ymm6,%ymm6");		\
		asm volatile("vpand %ymm3,%ymm10,%ymm10");		\
		asm volatile("vpand %ymm3,%ymm14,%ymm14");		\
		asm volatile("vpxor %ymm2,%ymm1,%ymm1");		\
		asm volatile("vpxor %ymm6,%ymm5,%ymm5");		\
		asm volatile("vpxor %ymm10,%ymm9,%ymm9");		\
		asm volatile("vpxor %ymm14,%ymm13,%ymm13");		\
	}								\
	asm volatile("vpxor %ymm0,%ymm1,%ymm1");			\
	asm volatile("vpxor %ymm4,%ymm5,%ymm5");			\
	asm volatile("vpxor %ymm8,%ymm9,%ymm9");			\
	asm volatile("vpxor %ymm12,%ymm13,%ymm13");			\
	asm volatile("vmovdqa %%ymm1,%0" : "=m" (*(r+0)));		\
	asm volatile("vmovdqa %%ymm5,%0" : "=m" (*(r+4)));		\
	asm volatile("vmovdqa %%ymm9,%0" : "=m" (*(r+8)));		\
	asm volatile("vmovdqa %%ymm13,%0" : "=m" (*(r+12)))


static int raidz_parity_have_avx2(void) {
	return (boot_cpu_has(X86_FEATURE_AVX2));
}

int
vdev_raidz_p_avx2(const void *buf, uint64_t size, void *private)
{
	struct pqr_struct *pqr = private;
	const uint64_t *src = buf;
	int i, cnt = size / sizeof (src[0]);

	ASSERT(pqr->p && !pqr->q && !pqr->r);
	kfpu_begin();
	i = 0;
	for (; i < cnt-15; i += 16, src += 16, pqr->p += 16) {
		LOAD16_SRC_AVX2(src);
		COMPUTE16_P_AVX2(pqr->p);
	}
	for (; i < cnt; i++, src++, pqr->p++)
		*pqr->p ^= *src;
	kfpu_end();
	return (0);
}
const struct raidz_parity_calls raidz1_avx2 = {
	vdev_raidz_p_avx2,
	raidz_parity_have_avx2,
	"p_avx2"
};

int
vdev_raidz_pq_avx2(const void *buf, uint64_t size, void *private)
{
	struct pqr_struct *pqr = private;
	const uint64_t *src = buf;
	uint64_t mask;
	int i, cnt = size / sizeof (src[0]);

	ASSERT(pqr->p && pqr->q && !pqr->r);
	kfpu_begin();
	i = 0;
	for (; i < cnt-15; i += 16, src += 16, pqr->p += 16, pqr->q += 16) {
		LOAD16_SRC_AVX2(src);
		COMPUTE16_P_AVX2(pqr->p);
		COMPUTE16_Q_AVX2(pqr->q);
	}
	for (; i < cnt; i++, src++, pqr->p++, pqr->q++) {
		*pqr->p ^= *src;
		VDEV_RAIDZ_64MUL_2(*pqr->q, mask);
		*pqr->q ^= *src;
	}
	kfpu_end();
	return (0);
}
const struct raidz_parity_calls raidz2_avx2 = {
	vdev_raidz_pq_avx2,
	raidz_parity_have_avx2,
	"pq_avx2"
};

int
vdev_raidz_pqr_avx2(const void *buf, uint64_t size, void *private)
{
	struct pqr_struct *pqr = private;
	const uint64_t *src = buf;
	uint64_t mask;
	int i, j, cnt = size / sizeof (src[0]);

	ASSERT(pqr->p && pqr->q && pqr->r);
	kfpu_begin();
	i = 0;
	for (; i < cnt-15; i += 16, src += 16, pqr->p += 16,
				pqr->q += 16, pqr->r += 16) {
		LOAD16_SRC_AVX2(src);
		COMPUTE16_P_AVX2(pqr->p);
		COMPUTE16_Q_AVX2(pqr->q);
		COMPUTE16_R_AVX2(pqr->r);
	}
	for (; i < cnt; i++, src++, pqr->p++, pqr->q++, pqr->r++) {
		*pqr->p ^= *src;
		VDEV_RAIDZ_64MUL_2(*pqr->q, mask);
		*pqr->q ^= *src;
		VDEV_RAIDZ_64MUL_4(*pqr->r, mask);
		*pqr->r ^= *src;
	}
	kfpu_end();
	return (0);
}
const struct raidz_parity_calls raidz3_avx2 = {
	vdev_raidz_pqr_avx2,
	raidz_parity_have_avx2,
	"pqr_avx2"
};


#endif
