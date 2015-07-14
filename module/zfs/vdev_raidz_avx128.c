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

#if defined(__x86_64__) && defined(_KERNEL) && defined(CONFIG_AS_AVX)
#define	MAKE_CST32_AVX128(reg, val)			\
	asm volatile("vmovd %0,%%"#reg : : "r"(val));	\
	asm volatile("vpshufd $0,%"#reg",%"#reg)

#define	LOAD8_SRC_AVX128(src)					\
	asm volatile("vmovdqa %0,%%xmm0" : : "m" (*(src+0)));	\
	asm volatile("vmovdqa %0,%%xmm4" : : "m" (*(src+2)));	\
	asm volatile("vmovdqa %0,%%xmm8" : : "m" (*(src+4)));	\
	asm volatile("vmovdqa %0,%%xmm12" : : "m" (*(src+6)))

#define	COMPUTE8_P_AVX128(p)					\
	asm volatile("vmovdqa %0,%%xmm1" : : "m" (*(p+0)));	\
	asm volatile("vmovdqa %0,%%xmm5" : : "m" (*(p+2)));	\
	asm volatile("vmovdqa %0,%%xmm9" : : "m" (*(p+4)));	\
	asm volatile("vmovdqa %0,%%xmm13" : : "m" (*(p+6)));	\
	asm volatile("vpxor %xmm0,%xmm1,%xmm1");		\
	asm volatile("vpxor %xmm4,%xmm5,%xmm5");		\
	asm volatile("vpxor %xmm8,%xmm9,%xmm9");		\
	asm volatile("vpxor %xmm12,%xmm13,%xmm13");		\
	asm volatile("vmovdqa %%xmm1,%0" : "=m" (*(p+0)));	\
	asm volatile("vmovdqa %%xmm5,%0" : "=m" (*(p+2)));	\
	asm volatile("vmovdqa %%xmm9,%0" : "=m" (*(p+4)));	\
	asm volatile("vmovdqa %%xmm13,%0" : "=m" (*(p+6)))

#define	COMPUTE8_Q_AVX128(q)						\
	asm volatile("vmovdqa %0,%%xmm1" : : "m" (*(q+0)));		\
	asm volatile("vmovdqa %0,%%xmm5" : : "m" (*(q+2)));		\
	asm volatile("vmovdqa %0,%%xmm9" : : "m" (*(q+4)));		\
	asm volatile("vmovdqa %0,%%xmm13" : : "m" (*(q+6)));		\
	MAKE_CST32_AVX128(xmm3, 0x1d1d1d1d);				\
	asm volatile("vpxor %xmm14, %xmm14, %xmm14");			\
	asm volatile("vpcmpgtb %xmm1, %xmm14, %xmm2");			\
	asm volatile("vpcmpgtb %xmm5, %xmm14, %xmm6");			\
	asm volatile("vpcmpgtb %xmm9, %xmm14, %xmm10");			\
	asm volatile("vpcmpgtb %xmm13, %xmm14, %xmm14");		\
	asm volatile("vpaddb %xmm1,%xmm1,%xmm1");			\
	asm volatile("vpaddb %xmm5,%xmm5,%xmm5");			\
	asm volatile("vpaddb %xmm9,%xmm9,%xmm9");			\
	asm volatile("vpaddb %xmm13,%xmm13,%xmm13");			\
	asm volatile("vpand %xmm3,%xmm2,%xmm2");			\
	asm volatile("vpand %xmm3,%xmm6,%xmm6");			\
	asm volatile("vpand %xmm3,%xmm10,%xmm10");			\
	asm volatile("vpand %xmm3,%xmm14,%xmm14");			\
	asm volatile("vpxor %xmm2,%xmm1,%xmm1");			\
	asm volatile("vpxor %xmm6,%xmm5,%xmm5");			\
	asm volatile("vpxor %xmm10,%xmm9,%xmm9");			\
	asm volatile("vpxor %xmm14,%xmm13,%xmm13");			\
	asm volatile("vpxor %xmm0,%xmm1,%xmm1");			\
	asm volatile("vpxor %xmm4,%xmm5,%xmm5");			\
	asm volatile("vpxor %xmm8,%xmm9,%xmm9");			\
	asm volatile("vpxor %xmm12,%xmm13,%xmm13");			\
	asm volatile("vmovdqa %%xmm1,%0" : "=m" (*(q+0)));		\
	asm volatile("vmovdqa %%xmm5,%0" : "=m" (*(q+2)));		\
	asm volatile("vmovdqa %%xmm9,%0" : "=m" (*(q+4)));		\
	asm volatile("vmovdqa %%xmm13,%0" : "=m" (*(q+6)))

#define	COMPUTE8_R_AVX128(r)						\
	asm volatile("vmovdqa %0,%%xmm1" : : "m" (*(r+0)));		\
	asm volatile("vmovdqa %0,%%xmm5" : : "m" (*(r+2)));		\
	asm volatile("vmovdqa %0,%%xmm9" : : "m" (*(r+4)));		\
	asm volatile("vmovdqa %0,%%xmm13" : : "m" (*(r+6)));		\
	MAKE_CST32_AVX128(xmm3, 0x1d1d1d1d);				\
	for (j = 0; j < 2; j++) {					\
		asm volatile("vpxor %xmm14, %xmm14, %xmm14");		\
		asm volatile("vpcmpgtb %xmm1, %xmm14, %xmm2");		\
		asm volatile("vpcmpgtb %xmm5, %xmm14, %xmm6");		\
		asm volatile("vpcmpgtb %xmm9, %xmm14, %xmm10");		\
		asm volatile("vpcmpgtb %xmm13, %xmm14, %xmm14");	\
		asm volatile("vpaddb %xmm1,%xmm1,%xmm1");		\
		asm volatile("vpaddb %xmm5,%xmm5,%xmm5");		\
		asm volatile("vpaddb %xmm9,%xmm9,%xmm9");		\
		asm volatile("vpaddb %xmm13,%xmm13,%xmm13");		\
		asm volatile("vpand %xmm3,%xmm2,%xmm2");		\
		asm volatile("vpand %xmm3,%xmm6,%xmm6");		\
		asm volatile("vpand %xmm3,%xmm10,%xmm10");		\
		asm volatile("vpand %xmm3,%xmm14,%xmm14");		\
		asm volatile("vpxor %xmm2,%xmm1,%xmm1");		\
		asm volatile("vpxor %xmm6,%xmm5,%xmm5");		\
		asm volatile("vpxor %xmm10,%xmm9,%xmm9");		\
		asm volatile("vpxor %xmm14,%xmm13,%xmm13");		\
	}								\
	asm volatile("vpxor %xmm0,%xmm1,%xmm1");			\
	asm volatile("vpxor %xmm4,%xmm5,%xmm5");			\
	asm volatile("vpxor %xmm8,%xmm9,%xmm9");			\
	asm volatile("vpxor %xmm12,%xmm13,%xmm13");			\
	asm volatile("vmovdqa %%xmm1,%0" : "=m" (*(r+0)));		\
	asm volatile("vmovdqa %%xmm5,%0" : "=m" (*(r+2)));		\
	asm volatile("vmovdqa %%xmm9,%0" : "=m" (*(r+4)));		\
	asm volatile("vmovdqa %%xmm13,%0" : "=m" (*(r+6)))

int
vdev_raidz_p_avx128(const void *buf, uint64_t size, void *private)
{
	struct pqr_struct *pqr = private;
	const uint64_t *src = buf;
	int i, cnt = size / sizeof (src[0]);

	ASSERT(pqr->p && !pqr->q && !pqr->r);
	kfpu_begin();
	i = 0;
	for (; i < cnt-7; i += 8, src += 8, pqr->p += 8) {
		LOAD8_SRC_AVX128(src);
		COMPUTE8_P_AVX128(pqr->p);
	}
	for (; i < cnt; i++, src++, pqr->p++)
		*pqr->p ^= *src;
	kfpu_end();
	return (0);
}

int
vdev_raidz_pq_avx128(const void *buf, uint64_t size, void *private)
{
	struct pqr_struct *pqr = private;
	const uint64_t *src = buf;
	uint64_t mask;
	int i, cnt = size / sizeof (src[0]);

	ASSERT(pqr->p && pqr->q && !pqr->r);
	kfpu_begin();
	i = 0;
	for (; i < cnt-7; i += 8, src += 8, pqr->p += 8, pqr->q += 8) {
		LOAD8_SRC_AVX128(src);
		COMPUTE8_P_AVX128(pqr->p);
		COMPUTE8_Q_AVX128(pqr->q);
	}
	for (; i < cnt; i++, src++, pqr->p++, pqr->q++) {
		*pqr->p ^= *src;
		VDEV_RAIDZ_64MUL_2(*pqr->q, mask);
		*pqr->q ^= *src;
	}
	kfpu_end();
	return (0);
}

int
vdev_raidz_pqr_avx128(const void *buf, uint64_t size, void *private)
{
	struct pqr_struct *pqr = private;
	const uint64_t *src = buf;
	uint64_t mask;
	int i, j, cnt = size / sizeof (src[0]);

	ASSERT(pqr->p && pqr->q && pqr->r);
	kfpu_begin();
	i = 0;
	for (; i < cnt-7; i += 8, src += 8, pqr->p += 8,
				pqr->q += 8, pqr->r += 8) {
		LOAD8_SRC_AVX128(src);
		COMPUTE8_P_AVX128(pqr->p);
		COMPUTE8_Q_AVX128(pqr->q);
		COMPUTE8_R_AVX128(pqr->r);
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


#endif
