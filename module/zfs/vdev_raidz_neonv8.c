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

#include <sys/zfs_context.h>
#include <sys/vdev_impl.h>
#include <sys/fs/zfs.h>
#include <sys/fm/fs/zfs.h>

#include <sys/vdev_raidz.h>

#if defined(__aarch64__)

#define	LOAD4128(a, b, c, d, X)						\
	asm volatile("ld1 {v"#a".4s},%0" : : "Q" (*(X+0)) : "v"#a);	\
	asm volatile("ld1 {v"#b".4s},%0" : : "Q" (*(X+2)) : "v"#b);	\
	asm volatile("ld1 {v"#c".4s},%0" : : "Q" (*(X+4)) : "v"#c);	\
	asm volatile("ld1 {v"#d".4s},%0" : : "Q" (*(X+6)) : "v"#d)

#define	STORE4128(a, b, c, d, X)					\
	asm volatile("st1 {v"#a".4s},%0" : "=Q" (*(X+0)));		\
	asm volatile("st1 {v"#b".4s},%0" : "=Q" (*(X+2)));		\
	asm volatile("st1 {v"#c".4s},%0" : "=Q" (*(X+4)));		\
	asm volatile("st1 {v"#d".4s},%0" : "=Q" (*(X+6)))

#define	LOAD464(a, b, c, d, X)						\
	asm volatile("ld1 {v"#a".2s},%0" : : "Q" (*(X+0)) : "v"#a);	\
	asm volatile("ld1 {v"#b".2s},%0" : : "Q" (*(X+1)) : "v"#b);	\
	asm volatile("ld1 {v"#c".2s},%0" : : "Q" (*(X+2)) : "v"#c);	\
	asm volatile("ld1 {v"#d".2s},%0" : : "Q" (*(X+3)) : "v"#d)

#define	STORE464(a, b, c, d, X)					\
	asm volatile("st1 {v"#a".2s},%0" : "=Q" (*(X+0)));	\
	asm volatile("st1 {v"#b".2s},%0" : "=Q" (*(X+1)));	\
	asm volatile("st1 {v"#c".2s},%0" : "=Q" (*(X+2)));	\
	asm volatile("st1 {v"#d".2s},%0" : "=Q" (*(X+3)))

#define	LOAD2128(a, b, X)						\
	asm volatile("ld1 {v"#a".4s},%0" : : "Q" (*(X+0)) : "v"#a);	\
	asm volatile("ld1 {v"#b".4s},%0" : : "Q" (*(X+2)) : "v"#b)

#define	STORE2128(a, b, X)					\
	asm volatile("st1 {v"#a".4s},%0" : "=Q" (*(X+0)));	\
	asm volatile("st1 {v"#b".4s},%0" : "=Q" (*(X+2)))

#define	MAKE_CST8_NEONV8(reg, val)				\
	asm volatile("movi "#reg".16b,#"#val : : : #reg)

/* V8 */

#define	LOAD8_SRC_NEONV8(src)			\
	LOAD4128(0, 1, 2, 3, src)

#define	COMPUTE8_P_NEONV8(p)					\
	LOAD4128(4, 5, 6, 7, p);				\
	asm volatile("eor v4.16b,v4.16b,v0.16b"  : : : "v4");	\
	asm volatile("eor v5.16b,v5.16b,v1.16b"  : : : "v5");	\
	asm volatile("eor v6.16b,v6.16b,v2.16b"  : : : "v6");	\
	asm volatile("eor v7.16b,v7.16b,v3.16b"  : : : "v7");	\
	STORE4128(4, 5, 6, 7, p)

#define	COMPUTE8_Q_NEONV8(q)						\
	LOAD4128(4, 5, 6, 7, q);					\
	MAKE_CST8_NEONV8(v12, 0x1d);					\
	asm volatile("eor v11.16b,v11.16b,v11.16b" : : : "v11");	\
	asm volatile("cmgt v8.16b,v11.16b,v4.16b" : : : "v8");		\
	asm volatile("cmgt v9.16b,v11.16b,v5.16b" : : : "v9");		\
	asm volatile("cmgt v10.16b,v11.16b,v6.16b" : : : "v10");	\
	asm volatile("cmgt v11.16b,v11.16b,v7.16b" : : : "v11");	\
	asm volatile("shl v4.16b,v4.16b,#1" : : : "v4");		\
	asm volatile("shl v5.16b,v5.16b,#1" : : : "v5");		\
	asm volatile("shl v6.16b,v6.16b,#1" : : : "v6");		\
	asm volatile("shl v7.16b,v7.16b,#1" : : : "v7");		\
	asm volatile("and v8.16b,v8.16b,v12.16b"  : : : "v8");		\
	asm volatile("and v9.16b,v9.16b,v12.16b"  : : : "v9");		\
	asm volatile("and v10.16b,v10.16b,v12.16b"  : : : "v10");	\
	asm volatile("and v11.16b,v11.16b,v12.16b"  : : : "v11");	\
	asm volatile("eor v4.16b,v4.16b,v8.16b"  : : : "v4");		\
	asm volatile("eor v5.16b,v5.16b,v9.16b"  : : : "v5");		\
	asm volatile("eor v6.16b,v6.16b,v10.16b"  : : : "v6");		\
	asm volatile("eor v7.16b,v7.16b,v11.16b"  : : : "v7");		\
	asm volatile("eor v4.16b,v4.16b,v0.16b"  : : : "v4");		\
	asm volatile("eor v5.16b,v5.16b,v1.16b"  : : : "v5");		\
	asm volatile("eor v6.16b,v6.16b,v2.16b"  : : : "v6");		\
	asm volatile("eor v7.16b,v7.16b,v3.16b"  : : : "v7");		\
	STORE4128(4, 5, 6, 7, q)


#define	COMPUTE8_R_NEONV8(r)						\
	LOAD4128(4, 5, 6, 7, r);					\
	MAKE_CST8_NEONV8(v12, 0x1d);					\
	for (j = 0; j < 2; j++) {					\
		asm volatile("eor v11.16b,v11.16b,v11.16b" : : : "v11"); \
		asm volatile("cmgt v8.16b,v11.16b,v4.16b" : : : "v8");	\
		asm volatile("cmgt v9.16b,v11.16b,v5.16b" : : : "v9");	\
		asm volatile("cmgt v10.16b,v11.16b,v6.16b" : : : "v10"); \
		asm volatile("cmgt v11.16b,v11.16b,v7.16b" : : : "v11"); \
		asm volatile("shl v4.16b,v4.16b,#1" : : : "v4");	\
		asm volatile("shl v5.16b,v5.16b,#1" : : : "v5");	\
		asm volatile("shl v6.16b,v6.16b,#1" : : : "v6");	\
		asm volatile("shl v7.16b,v7.16b,#1" : : : "v7");	\
		asm volatile("and v8.16b,v8.16b,v12.16b"  : : : "v8");	\
		asm volatile("and v9.16b,v9.16b,v12.16b"  : : : "v9");	\
		asm volatile("and v10.16b,v10.16b,v12.16b"  : : : "v10"); \
		asm volatile("and v11.16b,v11.16b,v12.16b"  : : : "v11"); \
		asm volatile("eor v4.16b,v4.16b,v8.16b"  : : : "v4");	\
		asm volatile("eor v5.16b,v5.16b,v9.16b"  : : : "v5");	\
		asm volatile("eor v6.16b,v6.16b,v10.16b"  : : : "v6");	\
		asm volatile("eor v7.16b,v7.16b,v11.16b"  : : : "v7");	\
	}								\
	asm volatile("eor v4.16b,v4.16b,v0.16b"  : : : "v4");		\
	asm volatile("eor v5.16b,v5.16b,v1.16b"  : : : "v5");		\
	asm volatile("eor v6.16b,v6.16b,v2.16b"  : : : "v6");		\
	asm volatile("eor v7.16b,v7.16b,v3.16b"  : : : "v7");		\
	STORE4128(4, 5, 6, 7, r)

/* V8DBL */

#define	LOAD16_SRC_NEONV8DBL(src)		\
	LOAD4128(0, 1, 2, 3, src);		\
	LOAD4128(16, 17, 18, 19, src+8)

#define	COMPUTE16_P_NEONV8DBL(p)					\
	LOAD4128(4, 5, 6, 7, p);					\
	LOAD4128(20, 21, 22, 23, p+8);					\
	asm volatile("eor v4.16b,v4.16b,v0.16b"  : : : "v4");		\
	asm volatile("eor v5.16b,v5.16b,v1.16b"  : : : "v5");		\
	asm volatile("eor v6.16b,v6.16b,v2.16b"  : : : "v6");		\
	asm volatile("eor v7.16b,v7.16b,v3.16b"  : : : "v7");		\
	asm volatile("eor v20.16b,v20.16b,v16.16b"  : : : "v20");	\
	asm volatile("eor v21.16b,v21.16b,v17.16b"  : : : "v21");	\
	asm volatile("eor v22.16b,v22.16b,v18.16b"  : : : "v22");	\
	asm volatile("eor v23.16b,v23.16b,v19.16b"  : : : "v23");	\
	STORE4128(4, 5, 6, 7, p);					\
	STORE4128(20, 21, 22, 23, p+8)

#define	COMPUTE16_Q_NEONV8DBL(q)					\
	LOAD4128(4, 5, 6, 7, q);					\
	LOAD4128(20, 21, 22, 23, q+8);					\
	MAKE_CST8_NEONV8(v12, 0x1d);					\
	asm volatile("eor v27.16b,v27.16b,v27.16b" : : : "v27");	\
	asm volatile("cmgt v8.16b,v27.16b,v4.16b" : : : "v8");		\
	asm volatile("cmgt v9.16b,v27.16b,v5.16b" : : : "v9");		\
	asm volatile("cmgt v10.16b,v27.16b,v6.16b" : : : "v10");	\
	asm volatile("cmgt v11.16b,v27.16b,v7.16b" : : : "v11");	\
	asm volatile("cmgt v24.16b,v27.16b,v20.16b" : : : "v24");	\
	asm volatile("cmgt v25.16b,v27.16b,v21.16b" : : : "v25");	\
	asm volatile("cmgt v26.16b,v27.16b,v22.16b" : : : "v26");	\
	asm volatile("cmgt v27.16b,v27.16b,v23.16b" : : : "v27");	\
	asm volatile("shl v4.16b,v4.16b,#1" : : : "v4");		\
	asm volatile("shl v5.16b,v5.16b,#1" : : : "v5");		\
	asm volatile("shl v6.16b,v6.16b,#1" : : : "v6");		\
	asm volatile("shl v7.16b,v7.16b,#1" : : : "v7");		\
	asm volatile("shl v20.16b,v20.16b,#1" : : : "v20");		\
	asm volatile("shl v21.16b,v21.16b,#1" : : : "v21");		\
	asm volatile("shl v22.16b,v22.16b,#1" : : : "v22");		\
	asm volatile("shl v23.16b,v23.16b,#1" : : : "v23");		\
	asm volatile("and v8.16b,v8.16b,v12.16b"  : : : "v8");		\
	asm volatile("and v9.16b,v9.16b,v12.16b"  : : : "v9");		\
	asm volatile("and v10.16b,v10.16b,v12.16b"  : : : "v10");	\
	asm volatile("and v11.16b,v11.16b,v12.16b"  : : : "v11");	\
	asm volatile("and v24.16b,v24.16b,v12.16b"  : : : "v24");	\
	asm volatile("and v25.16b,v25.16b,v12.16b"  : : : "v25");	\
	asm volatile("and v26.16b,v26.16b,v12.16b"  : : : "v26");	\
	asm volatile("and v27.16b,v27.16b,v12.16b"  : : : "v27");	\
	asm volatile("eor v4.16b,v4.16b,v8.16b"  : : : "v4");		\
	asm volatile("eor v5.16b,v5.16b,v9.16b"  : : : "v5");		\
	asm volatile("eor v6.16b,v6.16b,v10.16b"  : : : "v6");		\
	asm volatile("eor v7.16b,v7.16b,v11.16b"  : : : "v7");		\
	asm volatile("eor v20.16b,v20.16b,v24.16b"  : : : "v20");	\
	asm volatile("eor v21.16b,v21.16b,v25.16b"  : : : "v21");	\
	asm volatile("eor v22.16b,v22.16b,v26.16b"  : : : "v22");	\
	asm volatile("eor v23.16b,v23.16b,v27.16b"  : : : "v23");	\
	asm volatile("eor v4.16b,v4.16b,v0.16b"  : : : "v4");		\
	asm volatile("eor v5.16b,v5.16b,v1.16b"  : : : "v5");		\
	asm volatile("eor v6.16b,v6.16b,v2.16b"  : : : "v6");		\
	asm volatile("eor v7.16b,v7.16b,v3.16b"  : : : "v7");		\
	asm volatile("eor v20.16b,v20.16b,v16.16b"  : : : "v20");	\
	asm volatile("eor v21.16b,v21.16b,v17.16b"  : : : "v21");	\
	asm volatile("eor v22.16b,v22.16b,v18.16b"  : : : "v22");	\
	asm volatile("eor v23.16b,v23.16b,v19.16b"  : : : "v23");	\
	STORE4128(4, 5, 6, 7, q);					\
	STORE4128(20, 21, 22, 23, q+8)

#define	COMPUTE16_R_NEONV8DBL(r)					\
	LOAD4128(4, 5, 6, 7, r);					\
	LOAD4128(20, 21, 22, 23, r+8);					\
	MAKE_CST8_NEONV8(v12, 0x1d);					\
	for (j = 0; j < 2; j++) {					\
		asm volatile("eor v27.16b,v27.16b,v27.16b" : : : "v27"); \
		asm volatile("cmgt v8.16b,v27.16b,v4.16b" : : : "v8");	\
		asm volatile("cmgt v9.16b,v27.16b,v5.16b" : : : "v9");	\
		asm volatile("cmgt v10.16b,v27.16b,v6.16b" : : : "v10"); \
		asm volatile("cmgt v11.16b,v27.16b,v7.16b" : : : "v11"); \
		asm volatile("cmgt v24.16b,v27.16b,v20.16b" : : : "v24"); \
		asm volatile("cmgt v25.16b,v27.16b,v21.16b" : : : "v25"); \
		asm volatile("cmgt v26.16b,v27.16b,v22.16b" : : : "v26"); \
		asm volatile("cmgt v27.16b,v27.16b,v23.16b" : : : "v27"); \
		asm volatile("shl v4.16b,v4.16b,#1" : : : "v4");	\
		asm volatile("shl v5.16b,v5.16b,#1" : : : "v5");	\
		asm volatile("shl v6.16b,v6.16b,#1" : : : "v6");	\
		asm volatile("shl v7.16b,v7.16b,#1" : : : "v7");	\
		asm volatile("shl v20.16b,v20.16b,#1" : : : "v20");	\
		asm volatile("shl v21.16b,v21.16b,#1" : : : "v21");	\
		asm volatile("shl v22.16b,v22.16b,#1" : : : "v22");	\
		asm volatile("shl v23.16b,v23.16b,#1" : : : "v23");	\
		asm volatile("and v8.16b,v8.16b,v12.16b"  : : : "v8");	\
		asm volatile("and v9.16b,v9.16b,v12.16b"  : : : "v9");	\
		asm volatile("and v10.16b,v10.16b,v12.16b"  : : : "v10"); \
		asm volatile("and v11.16b,v11.16b,v12.16b"  : : : "v11"); \
		asm volatile("and v24.16b,v24.16b,v12.16b"  : : : "v24"); \
		asm volatile("and v25.16b,v25.16b,v12.16b"  : : : "v25"); \
		asm volatile("and v26.16b,v26.16b,v12.16b"  : : : "v26"); \
		asm volatile("and v27.16b,v27.16b,v12.16b"  : : : "v27"); \
		asm volatile("eor v4.16b,v4.16b,v8.16b"  : : : "v4");	\
		asm volatile("eor v5.16b,v5.16b,v9.16b"  : : : "v5");	\
		asm volatile("eor v6.16b,v6.16b,v10.16b"  : : : "v6");	\
		asm volatile("eor v7.16b,v7.16b,v11.16b"  : : : "v7");	\
		asm volatile("eor v20.16b,v20.16b,v24.16b"  : : : "v20"); \
		asm volatile("eor v21.16b,v21.16b,v25.16b"  : : : "v21"); \
		asm volatile("eor v22.16b,v22.16b,v26.16b"  : : : "v22"); \
		asm volatile("eor v23.16b,v23.16b,v27.16b"  : : : "v23"); \
	}								\
	asm volatile("eor v4.16b,v4.16b,v0.16b"  : : : "v4");		\
	asm volatile("eor v5.16b,v5.16b,v1.16b"  : : : "v5");		\
	asm volatile("eor v6.16b,v6.16b,v2.16b"  : : : "v6");		\
	asm volatile("eor v7.16b,v7.16b,v3.16b"  : : : "v7");		\
	asm volatile("eor v20.16b,v20.16b,v16.16b"  : : : "v20");	\
	asm volatile("eor v21.16b,v21.16b,v17.16b"  : : : "v21");	\
	asm volatile("eor v22.16b,v22.16b,v18.16b"  : : : "v22");	\
	asm volatile("eor v23.16b,v23.16b,v19.16b"  : : : "v23");	\
	STORE4128(4, 5, 6, 7, r);					\
	STORE4128(20, 21, 22, 23, r+8)

/* V8D */

#define	MAKE_CST8_NEONV8D(reg, val)			\
	asm volatile("movi "#reg".8b,#"#val : : : #reg)

#define	LOAD4_SRC_NEONV8D(src)			\
	LOAD464(0, 1, 2, 3, src)

#define	COMPUTE4_P_NEONV8D(p)			\
	LOAD464(4, 5, 6, 7, p);						\
	asm volatile("eor v4.8b,v4.8b,v0.8b"  : : : "v4");		\
	asm volatile("eor v5.8b,v5.8b,v1.8b"  : : : "v5");		\
	asm volatile("eor v6.8b,v6.8b,v2.8b"  : : : "v6");		\
	asm volatile("eor v7.8b,v7.8b,v3.8b"  : : : "v7");		\
	STORE464(4, 5, 6, 7, p)\

#define	COMPUTE4_Q_NEONV8D(q)					\
	LOAD464(4, 5, 6, 7, q);					\
	MAKE_CST8_NEONV8D(v12, 0x1d);				\
	asm volatile("eor v11.8b,v11.8b,v11.8b" : : : "v11");	\
	asm volatile("cmgt v8.8b,v11.8b,v4.8b" : : : "v8");	\
	asm volatile("cmgt v9.8b,v11.8b,v5.8b" : : : "v9");	\
	asm volatile("cmgt v10.8b,v11.8b,v6.8b" : : : "v10");	\
	asm volatile("cmgt v11.8b,v11.8b,v7.8b" : : : "v11");	\
	asm volatile("shl v4.8b,v4.8b,#1" : : : "v4");		\
	asm volatile("shl v5.8b,v5.8b,#1" : : : "v5");		\
	asm volatile("shl v6.8b,v6.8b,#1" : : : "v6");		\
	asm volatile("shl v7.8b,v7.8b,#1" : : : "v7");		\
	asm volatile("and v8.8b,v8.8b,v12.8b"  : : : "v8");	\
	asm volatile("and v9.8b,v9.8b,v12.8b"  : : : "v9");	\
	asm volatile("and v10.8b,v10.8b,v12.8b"  : : : "v10");	\
	asm volatile("and v11.8b,v11.8b,v12.8b"  : : : "v11");	\
	asm volatile("eor v4.8b,v4.8b,v8.8b"  : : : "v4");	\
	asm volatile("eor v5.8b,v5.8b,v9.8b"  : : : "v5");	\
	asm volatile("eor v6.8b,v6.8b,v10.8b"  : : : "v6");	\
	asm volatile("eor v7.8b,v7.8b,v11.8b"  : : : "v7");	\
	asm volatile("eor v4.8b,v4.8b,v0.8b"  : : : "v4");	\
	asm volatile("eor v5.8b,v5.8b,v1.8b"  : : : "v5");	\
	asm volatile("eor v6.8b,v6.8b,v2.8b"  : : : "v6");	\
	asm volatile("eor v7.8b,v7.8b,v3.8b"  : : : "v7");	\
	STORE464(4, 5, 6, 7, q)


#define	COMPUTE4_R_NEONV8D(r)						\
	LOAD464(4, 5, 6, 7, r);						\
	MAKE_CST8_NEONV8D(v12, 0x1d);					\
	for (j = 0; j < 2; j++) {					\
		asm volatile("eor v11.8b,v11.8b,v11.8b" : : : "v11");	\
		asm volatile("cmgt v8.8b,v11.8b,v4.8b" : : : "v8");	\
		asm volatile("cmgt v9.8b,v11.8b,v5.8b" : : : "v9");	\
		asm volatile("cmgt v10.8b,v11.8b,v6.8b" : : : "v10");	\
		asm volatile("cmgt v11.8b,v11.8b,v7.8b" : : : "v11");	\
		asm volatile("shl v4.8b,v4.8b,#1" : : : "v4");		\
		asm volatile("shl v5.8b,v5.8b,#1" : : : "v5");		\
		asm volatile("shl v6.8b,v6.8b,#1" : : : "v6");		\
		asm volatile("shl v7.8b,v7.8b,#1" : : : "v7");		\
		asm volatile("and v8.8b,v8.8b,v12.8b"  : : : "v8");	\
		asm volatile("and v9.8b,v9.8b,v12.8b"  : : : "v9");	\
		asm volatile("and v10.8b,v10.8b,v12.8b"  : : : "v10");	\
		asm volatile("and v11.8b,v11.8b,v12.8b"  : : : "v11");	\
		asm volatile("eor v4.8b,v4.8b,v8.8b"  : : : "v4");	\
		asm volatile("eor v5.8b,v5.8b,v9.8b"  : : : "v5");	\
		asm volatile("eor v6.8b,v6.8b,v10.8b"  : : : "v6");	\
		asm volatile("eor v7.8b,v7.8b,v11.8b"  : : : "v7");	\
	}								\
	asm volatile("eor v4.8b,v4.8b,v0.8b"  : : : "v4");		\
	asm volatile("eor v5.8b,v5.8b,v1.8b"  : : : "v5");		\
	asm volatile("eor v6.8b,v6.8b,v2.8b"  : : : "v6");		\
	asm volatile("eor v7.8b,v7.8b,v3.8b"  : : : "v7");		\
	STORE464(4, 5, 6, 7, r)

/* V8I */

#define	MAKE_CST8_NEONV8I(reg, val)				\
	asm volatile("movi "#reg".16b,#"#val : : : #reg)

#define	LOAD8_SRC_NEONV8I(src)			\
	LOAD4128(0, 1, 2, 3, src)

#define	COMPUTE8_P_NEONV8I(p)					\
	asm volatile("ld1 {v4.4s},%0" : : "Q" (*(p+0)) : "v4");	\
	asm volatile("eor v4.16b,v4.16b,v0.16b"  : : : "v4");	\
	asm volatile("st1 {v4.4s},%0" : "=Q" (*(p+0)));		\
	asm volatile("ld1 {v5.4s},%0" : : "Q" (*(p+2)) : "v5");	\
	asm volatile("eor v5.16b,v5.16b,v1.16b"  : : : "v5");	\
	asm volatile("st1 {v5.4s},%0" : "=Q" (*(p+2)));		\
	asm volatile("ld1 {v6.4s},%0" : : "Q" (*(p+4)) : "v6");	\
	asm volatile("eor v6.16b,v6.16b,v2.16b"  : : : "v6");	\
	asm volatile("st1 {v6.4s},%0" : "=Q" (*(p+4)));		\
	asm volatile("ld1 {v7.4s},%0" : : "Q" (*(p+6)) : "v7");	\
	asm volatile("eor v7.16b,v7.16b,v3.16b"  : : : "v7");	\
	asm volatile("st1 {v7.4s},%0" : "=Q" (*(p+6)))

#define	COMPUTE8_Q_NEONV8I(q)						\
	MAKE_CST8_NEONV8I(v12, 0x1d);					\
	asm volatile("eor v11.16b,v11.16b,v11.16b" : : : "v11");	\
	asm volatile("ld1 {v4.4s},%0" : : "Q" (*(q+0)) : "v4");		\
	asm volatile("cmgt v8.16b,v11.16b,v4.16b" : : : "v8");		\
	asm volatile("ld1 {v5.4s},%0" : : "Q" (*(q+2)) : "v5");		\
	asm volatile("cmgt v9.16b,v11.16b,v5.16b" : : : "v9");		\
	asm volatile("ld1 {v6.4s},%0" : : "Q" (*(q+4)) : "v6");		\
	asm volatile("cmgt v10.16b,v11.16b,v6.16b" : : : "v10");	\
	asm volatile("ld1 {v7.4s},%0" : : "Q" (*(q+6)) : "v7");		\
	asm volatile("cmgt v11.16b,v11.16b,v7.16b" : : : "v11");	\
	asm volatile("shl v4.16b,v4.16b,#1" : : : "v4");		\
	asm volatile("shl v5.16b,v5.16b,#1" : : : "v5");		\
	asm volatile("shl v6.16b,v6.16b,#1" : : : "v6");		\
	asm volatile("shl v7.16b,v7.16b,#1" : : : "v7");		\
	asm volatile("and v8.16b,v8.16b,v12.16b"  : : : "v8");		\
	asm volatile("and v9.16b,v9.16b,v12.16b"  : : : "v9");		\
	asm volatile("and v10.16b,v10.16b,v12.16b"  : : : "v10");	\
	asm volatile("and v11.16b,v11.16b,v12.16b"  : : : "v11");	\
	asm volatile("eor v4.16b,v4.16b,v8.16b"  : : : "v4");		\
	asm volatile("eor v5.16b,v5.16b,v9.16b"  : : : "v5");		\
	asm volatile("eor v6.16b,v6.16b,v10.16b"  : : : "v6");		\
	asm volatile("eor v7.16b,v7.16b,v11.16b"  : : : "v7");		\
	asm volatile("eor v4.16b,v4.16b,v0.16b"  : : : "v4");		\
	asm volatile("st1 {v4.4s},%0" : "=Q" (*(q+0)));			\
	asm volatile("eor v5.16b,v5.16b,v1.16b"  : : : "v5");		\
	asm volatile("st1 {v5.4s},%0" : "=Q" (*(q+2)));			\
	asm volatile("eor v6.16b,v6.16b,v2.16b"  : : : "v6");		\
	asm volatile("st1 {v6.4s},%0" : "=Q" (*(q+4)));			\
	asm volatile("eor v7.16b,v7.16b,v3.16b"  : : : "v7");		\
	asm volatile("st1 {v7.4s},%0" : "=Q" (*(q+6)))


#define	COMPUTE8_R_NEONV8I(r)						\
	MAKE_CST8_NEONV8I(v12, 0x1d);					\
	LOAD4128(4, 5, 6, 7, r);					\
	for (j = 0; j < 2; j++) {					\
		asm volatile("eor v11.16b,v11.16b,v11.16b" : : : "v11"); \
		asm volatile("cmgt v8.16b,v11.16b,v4.16b" : : : "v8");	\
		asm volatile("cmgt v9.16b,v11.16b,v5.16b" : : : "v9");	\
		asm volatile("cmgt v10.16b,v11.16b,v6.16b" : : : "v10"); \
		asm volatile("cmgt v11.16b,v11.16b,v7.16b" : : : "v11"); \
		asm volatile("shl v4.16b,v4.16b,#1" : : : "v4");	\
		asm volatile("shl v5.16b,v5.16b,#1" : : : "v5");	\
		asm volatile("shl v6.16b,v6.16b,#1" : : : "v6");	\
		asm volatile("shl v7.16b,v7.16b,#1" : : : "v7");	\
		asm volatile("and v8.16b,v8.16b,v12.16b"  : : : "v8");	\
		asm volatile("and v9.16b,v9.16b,v12.16b"  : : : "v9");	\
		asm volatile("and v10.16b,v10.16b,v12.16b"  : : : "v10"); \
		asm volatile("and v11.16b,v11.16b,v12.16b"  : : : "v11"); \
		asm volatile("eor v4.16b,v4.16b,v8.16b"  : : : "v4");	\
		asm volatile("eor v5.16b,v5.16b,v9.16b"  : : : "v5");	\
		asm volatile("eor v6.16b,v6.16b,v10.16b"  : : : "v6");	\
		asm volatile("eor v7.16b,v7.16b,v11.16b"  : : : "v7");	\
	}								\
	asm volatile("eor v4.16b,v4.16b,v0.16b"  : : : "v4");		\
	asm volatile("st1 {v4.4s},%0" : "=Q" (*(r+0)));			\
	asm volatile("eor v5.16b,v5.16b,v1.16b"  : : : "v5");		\
	asm volatile("st1 {v5.4s},%0" : "=Q" (*(r+2)));			\
	asm volatile("eor v6.16b,v6.16b,v2.16b"  : : : "v6");		\
	asm volatile("st1 {v6.4s},%0" : "=Q" (*(r+4)));			\
	asm volatile("eor v7.16b,v7.16b,v3.16b"  : : : "v7");		\
	asm volatile("st1 {v7.4s},%0" : "=Q" (*(r+6)))



static int raidz_parity_have_neonv8(void) {
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
		raidz_parity_have_neonv8,\
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
		raidz_parity_have_neonv8,\
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
		raidz_parity_have_neonv8,\
		"pqr_"#suf\
	};\


#define	MAKE_FUNS(suf, SUF, m, M)			\
	MAKE_P_FUN(suf, SUF, m, M)			\
	MAKE_Q_FUN(suf, SUF, m, M)			\
	MAKE_R_FUN(suf, SUF, m, M)

MAKE_FUNS(neonv8, NEONV8, 7, 8)
MAKE_FUNS(neonv8dbl, NEONV8DBL, 15, 16)
MAKE_FUNS(neonv8d, NEONV8D, 3, 4)
MAKE_FUNS(neonv8i, NEONV8I, 7, 8)

#endif
