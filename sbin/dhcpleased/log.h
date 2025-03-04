/*	$OpenBSD: log.h,v 1.4 2024/06/03 17:58:33 deraadt Exp $ */

/*
 * Copyright (c) 2003, 2004 Henning Brauer <henning@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdlib.h>

#ifndef	SMALL
void	log_init(int, int);
void	log_procinit(const char *);
void	log_setverbose(int);
int	log_getverbose(void);
void	log_warn(const char *, ...)
	    __attribute__((__format__ (printf, 1, 2)));
void	log_warnx(const char *, ...)
	    __attribute__((__format__ (printf, 1, 2)));
void	log_info(const char *, ...)
	    __attribute__((__format__ (printf, 1, 2)));
void	log_debug(const char *, ...)
	    __attribute__((__format__ (printf, 1, 2)));
void	logit(int, const char *, ...)
	    __attribute__((__format__ (printf, 2, 3)));
void	vlog(int, const char *, va_list)
	    __attribute__((__format__ (printf, 2, 0)));
__dead void fatal(const char *, ...)
	    __attribute__((__format__ (printf, 1, 2)));
__dead void fatalx(const char *, ...)
	    __attribute__((__format__ (printf, 1, 2)));
#else
#define log_init(x...)		do {} while (0)
#define log_procinit(x...)	do {} while (0)
#define log_setverbose(x...)	do {} while (0)
#define log_getverbose(x...)	(0)
#define log_warn(x...)		do {} while (0)
#define log_warnx(x...)		do {} while (0)
#define log_info(x...)		do {} while (0)
#define log_debug(x...)		do {} while (0)
#define logit(x...)		do {} while (0)
#define vlog(x...)		do {} while (0)
#define fatal(x...)		exit(1)
#define fatalx(x...)		exit(1)
#endif	/* SMALL */

#endif /* LOG_H */
