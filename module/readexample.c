/*
 * ����һ���ṩ���ѯ������
 * ����������API
 * int sorder_init(const char *file);
 * int sorder_query(const char *order, char *output, int count, int start);
 * int sorder_final();
 */

#include <stdio.h>
#include <stdlib.h>

FILE *libfile = NULL;
/*
 * ��ʼ���͹ر��ļ�
 * ʵ����ȷʵֻ�Ǵ��ļ��������Ҫ������ӻ������
 * ���ط�0ʧ��
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

//��� big-endian �洢
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
 * ��ѯ��˳��������ѯ����
 * order - �� "1234"��'1'����ᣬ'2'Ϊ����'3'ΪƲ��'4'Ϊ��/�㣬'5'Ϊ��
 * output - �����ַ���ÿ�����ֽ�Ϊһ�����֣��ִ���'\0'�����������߱����ṩ����count * 2 + 1�ֽڵ�output
 * count - ��Ҫ���ص��ַ��������ص��ַ���д��output��
 * start - ��ѯ��õ�һϵ�еķ��ϱ�˳�ַ���startָ����ʼ����λ�ã�ͨ�����ڷ�ҳ
 *
 * ����ֵ��ʵ��output�������ַ�����������Ϊ����
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
	int datapos = readint(curdata);		//��ȡ���ݿ�λ��
	unsigned short datacnt = readshort(datapos);	//��ȡ�ַ�����
	datapos += 2;						//dataposָ���׸��ַ�
	
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

