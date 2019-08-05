/*
 * UNG's Not GNU
 *
 * Copyright (c) 2011-2017, Jakob Kaivo <jkk@ung.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
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
 *
 */

#define _POSIX_C_SOURCE 2
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

FILE *fileopen(const char *path)
{
	if (!strcmp(path, "-")) {
		return stdin;
	}

	FILE *f = fopen(path, "rb");
	if (f == NULL) {
		fprintf(stderr, "cmp: %s: %s\n", path, strerror(errno));
		exit(1);
	}

	return f;
}

int main(int argc, char *argv[])
{
	int c;
	enum { FIRSTONLY, ALL, SILENT } output = FIRSTONLY;

	while ((c = getopt(argc, argv, "ls")) != EOF) {
		switch (c) {
		case 'l':
			output = ALL;
			break;

		case 's':
			output = SILENT;
			break;

		default:
			return 1;
		}
	}

	if (optind != argc - 2) {
		fprintf(stderr, "cmp: missing operand\n");
		return 1;
	}

	char *file1 = argv[optind++];
	char *file2 = argv[optind];
	FILE *f1 = fileopen(file1);
	FILE *f2 = fileopen(file2);

	if (f1 == stdin && f2 == stdin) {
		fprintf(stderr, "cmp: Comparing stdin to itself is undefined\n");
		return 1;
	}

	struct stat st1;
	fstat(fileno(f1), &st1);
	if (S_ISBLK(st1.st_mode) || S_ISCHR(st1.st_mode) || S_ISFIFO(st1.st_mode)) {
		struct stat st2;
		fstat(fileno(f2), &st2);
		if (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino) {
			fprintf(stderr, "cmp: Comparing special file to itself is undefined\n");
			return 1;
		}
	}

	intmax_t byte = 0;
	intmax_t line = 1;
	while (++byte) {
		int c1 = fgetc(f1);
		int c2 = fgetc(f2);

		if (c1 == c2) {
			if (c1 == EOF) {
				return 0;
			}

			if (c1 == '\n') {
				line++;
			}

			continue;
		}

		if (c1 == EOF && output != SILENT) {
			fprintf(stderr, "cmp: EOF on %s\n", file1);
		} else if (c2 == EOF && output != SILENT) {
			fprintf(stderr, "cmp: EOF on %s\n", file2);
		} else if (output == ALL) {
			printf("%zd %o %o\n", byte, c1, c2);
			continue;
		} else if (output != SILENT) {
			printf("%s %s differ: char %zd, line %zd\n",
				file1, file2, byte, line);
		}
		return 1;
	}

	return 0;
}
