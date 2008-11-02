/*
 * freq - 中文GBK字符字频统计
 * Copyright (c) 2008, wenxichang@163.com
 * All rights reserved.
 *
 * 本文件使用BSDL协议发布
 */

#include <stdio.h>
#include <stdlib.h>

int freq[65536];

int isgbk(int ch)
{
	unsigned char chr[2];
	chr[0] = (ch & 0xff00) >> 8;
	chr[1] = (ch & 0xff);
	if (chr[0] <= 0x80 || chr[1] < 0x40) return 0;
	if (chr[1] == 0x7f || chr[1] == 0xff) return 0;
	return 1;
}

int getchgbk(FILE *fp)
{
	int chr = fgetc(fp);
	if (chr > 0x80) {
		int r = fgetc(fp);
		return (r & 0xff) + ((chr & 0xff) << 8);
	}
	return chr;
}

int main(int argc, char **argv)
{
	int i;
	FILE *fp;
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file1> [file2 ...]\n", argv[0]);
		exit(0);
	}
	for (i = 1; i < argc; i++) {
		fp = fopen(argv[i], "r");
		if (!fp) {
			fprintf(stderr, "failed to open %s\n", argv[i]);
			exit(0);
		}
		
		while(!feof(fp)) {
			int chr = getchgbk(fp);
			if (chr >= 0 && chr < 65536)
				freq[chr] ++;
		}
		fclose(fp);
		fprintf(stderr, "%s finished\n", argv[i]);
	}
	
	for (i = 0; i < 32; ++i) {
		printf("%X\t%d\tNotGBK\n", i, freq[i]);
	}
	for (i = 32; i < 256; ++i) {
		printf("%X\t%d\tNotGBK\t%c\n", i, freq[i], i);
	}
	
	for (i = 256; i < 65536; ++i) {
		char ss[3];
		ss[0] = (i & 0xff00) >> 8;
		ss[1] = (i & 0xff);
		ss[2] = 0;
		if (isgbk(i)) {
			printf("%X\t%d\tGBK\t%s\n", i, freq[i], ss);
		} else {
			printf("%X\t%d\tNotGBK\n", i, freq[i]);
		}
	}
	return 0;
}

