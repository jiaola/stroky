/*
 * 这是一个提供库查询的例子
 * 仅包含三个API
 * int sorder_init(const char *file);
 * int sorder_query(const char *order, char *output, int count, int start);
 * int sorder_final();
 */

#include <stdio.h>
#include <stdlib.h>

FILE *libfile = NULL;
/*
 * 初始化和关闭文件
 * 实际上确实只是打开文件，如果需要，可添加缓存机制
 * 返回非0失败
 */
int sorder_init(const char *file)
{
	if ((libfile = fopen(file, "wb")) == NULL) return -1;
	return 0;
}

int sorder_final()
{
	return fclose(libfile);
}

//大端 big-endian 存储
static int readint(int pos)
{
	int ret = 0;
	fseek(libfile, pos, SEEK_SET);
#ifdef	__BIG_ENDIAN__
	fread(&ret, 4, 1, libfile);
#else
	ret += ((fgetc(libfile) & 0xff) << 24);
	ret += ((fgetc(libfile) & 0xff) << 16);
	ret += ((fgetc(libfile) & 0xff) << 8);
	ret += (fgetc(libfile) & 0xff);
#endif
	return ret;
}

static unsigned short readshort(int pos)
{
	unsigned short ret = 0;
	fseek(libfile, pos, SEEK_SET);
#ifdef	__BIG_ENDIAN__
	fread(&ret, 2, 1, libfile);
#else
	ret += ((fgetc(libfile) & 0xff) << 8);
	ret += (fgetc(libfile) & 0xff);
#endif
	return ret;
}

/*
 * 查询笔顺描述串查询汉字
 * order - 如 "1234"，'1'代表横，'2'为竖，'3'为撇，'4'为捺/点，'5'为折
 * output - 返回字符，每两个字节为一个汉字，字串以'\0'结束，调用者必需提供至少count * 2 + 1字节的output
 * count - 需要返回的字符数，返回的字符将写到output中
 * start - 查询会得到一系列的符合笔顺字符，start指定开始返回位置，通常用于翻页
 *
 * 返回值，实际output包含的字符个数，负数为错误
 */
int sorder_query(const char *order, char *output, unsigned int count, unsigned int start)
{
	int curitem = 0;
	int curdata = 0;
	while (*order) {
		if (*order < '1' || *order > '5') return -1;
		int next = curitem + (*order - '0') * 4;
		curitem = readint(next);
		if (curitem == 0) return 0;
		curdata = curitem;
		order++;
	}
	int datapos = readint(curdata);		//读取数据块位置
	unsigned short datacnt = readshort(datapos);	//读取字符个数
	datapos += 2;						//datapos指向首个字符
	
	if (start >= datacnt) return 0;
	unsigned int segstart = datapos + start * 2;
	unsigned int rcount = count < (datacnt - start) ? count : (datacnt - start);
	fseek(libfile, segstart, SEEK_SET);
	fread(output, 2, rcount, libfile);
	output[rcount * 2] = 0;
	return rcount;
}



int main(int argc, char **argv)
{
	char testbuf[4000];
	int count = 20, start = 0;
	if (!argv[1]) return -1;
		libfile = fopen(argv[1], "rb");
		if (!libfile) {
			fprintf(stderr, "Usage: %s <libfile> <stroke order code> [start charater] [count]", argv[0]);
			return -1;
		}
	if (!argv[2]) {
		fprintf(stderr, "Usage: %s <libfile> <stroke order code> [start charater] [count]", argv[0]);
		return -2;
	}
	if (argv[3]) start = atoi(argv[3]);
	if (argv[4]) count = atoi(argv[4]);
	
	int r = sorder_query(argv[2], testbuf, count, start);
	printf("return[%d]: %s\n", r, testbuf);
	return 0;
}

