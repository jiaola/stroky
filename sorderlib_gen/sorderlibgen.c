/*
* sorderlibgen.c - ���ɱ�˳���뷨��
* Copyright (c) 2008, wenxichang@163.com
* All rights reserved.
*
* ���ļ�ʹ��BSDLЭ�鷢��
*
* ��������뺺�ֱ�˳��� - һ����5�е��ı��ļ�
* <��˳> <�ʻ���> <GBK��> <����[������]> <���ֳ���Ƶ��>
* ����û�н����κ���������� ���� ���ǵ�Ŀ�Ĳ��ǳ��򣬶��������ɵĿ�
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//��ű�ѡ������
struct SData {
	unsigned short count;		//��ѡ����Ŀ
	unsigned short *data;		//��ѡ��
};

#define MAX_DATA	2000		//��ѡ��֧��400
#define MAX_RECORD	21000		//�������Ŀ��

struct SOTrie {
	struct SData *sdata;		//��ѡ������
	struct SOTrie *snext[5];		//���ֱʻ�ָ��
};

struct SRecord {
	char sorder[50];			//��˳
	int scount;					//�ʻ�
	unsigned short chareter;	//�ַ�
	int freq;					//��Ƶ
};


struct SRecord global_srecord[MAX_RECORD];		//Ŀǰֻ������ô�຺��
int global_srecord_count = 0;
struct SOTrie root;
struct SRecord *grecord_index[65536];			//���ں��ַ������

//���ɱʻ�Trie���
struct SOTrie *sotrie_new()
{
	struct SOTrie *ret = malloc(sizeof(struct SOTrie));
	memset(ret, 0, sizeof(struct SOTrie));
	return ret;
}

//�ַ���ת��Ϊshort gbk
unsigned short to_gbkchar(const char *gbk)
{
	unsigned short ret = 0, t = 0;
	while (*gbk) {
		t = (*gbk >= 'A' && *gbk <= 'F') ? *gbk - 'A' + 10 : *gbk - '0';
		ret = (ret << 4) + t;
		gbk++;
	}
	return ret;
}

//��ӡshort gbk
void putgbkchr(unsigned short chr)
{
	char b[3];
	b[0] = (chr & 0xff00) >> 8;
	b[1] = chr & 0xff;
	b[2] = 0;
	printf("%s", b);
}

//���ļ��������¼
int read_srecord(const char *datafile)
{
	FILE *fp = fopen(datafile, "r");
	if (!fp) {
		fprintf(stderr, "Can not open %s for read\n", datafile);
		return -1;
	}
	char gbk[10];
	char noused[10];
	while (fscanf(fp, "%s %d %s %s %d",
				  global_srecord[global_srecord_count].sorder,
				  &global_srecord[global_srecord_count].scount,
				  gbk, noused, &global_srecord[global_srecord_count].freq) != EOF) {
		unsigned short x = to_gbkchar(gbk);
		global_srecord[global_srecord_count].chareter = x;
		grecord_index[x] = &global_srecord[global_srecord_count];
		global_srecord_count++;
	}
	fclose(fp);
	return 0;
}

//�ʻ�ת����
int stroke_to_index(char chr)
{
	return chr - '1';
}

//�½���ѡ������
struct SData *sdata_new()
{
	struct SData *ret = malloc(sizeof(struct SData));
	memset(ret, 0, sizeof(struct SData));
	return ret;
}

//rֵ��ʾ�Բ���chr���д�����֤�������rǰ���ڣ��򲻲���
void sdata_insert(struct SData *sd, unsigned short chr, int r)
{
	if (sd -> count == 0) {
		sd -> data = malloc(sizeof(unsigned short) * MAX_DATA);
	}
	if (sd -> count >= MAX_DATA) return;
	
	int i;
	if (r > sd -> count) r = sd -> count;
	for (i = 0; i < r; ++i) {				//�Ƿ���ǰr���ֳ�ͻ
		if (sd -> data[i] == chr) return;
	}
	
	if (r == -1) {							//Ϊ��ʼ����
		for (i = sd -> count; i > 0; --i) {	//����Ƶ����
			if (grecord_index[sd -> data[i - 1]] -> freq < grecord_index[chr] -> freq) {
				sd -> data[i] = sd -> data[i - 1];
			} else break;
		}
		sd -> data[i] = chr;
		sd -> count++;
	} else {								//�����Ѿ������
		sd -> data[sd -> count] = chr;
		sd -> count++;
	}
}

//����˳/�ַ����뵽trie��
void trie_insert(const char *sorder, unsigned short chr)
{
	struct SOTrie *curso = &root;
	while (*sorder) {
		int si = stroke_to_index(*sorder);
		if (curso -> snext[si] == NULL) {
			curso -> snext[si] = sotrie_new();
		}
		curso = curso -> snext[si];
		sorder++;
	}
	if (curso -> sdata == NULL) curso -> sdata = sdata_new();
	sdata_insert(curso -> sdata, chr, -1);
}

int build_trie()
{
	int i;
	for (i = 0; i < global_srecord_count; ++i) {
		trie_insert(global_srecord[i].sorder, global_srecord[i].chareter);
	}
	return 0;
}

//������˳����str(len���ֽ�)��record�д��ڵ���ʼ���������
void record_search(const char *str, int len, int *start, int *end)
{
	//todo:���ַ�����
	int i;
	int flag = 0;
	*start = 0;
	*end = global_srecord_count - 1;
	
	if (len == 0) return;
	
	for (i = 0; i < global_srecord_count; ++i) {
		if (flag == 0) {
			if (strncmp(str, global_srecord[i].sorder, len) == 0) {
				*start = i;
				flag = 1;
			}
		} else {
			if (strncmp(str, global_srecord[i].sorder, len) != 0) {
				*end = i - 1;
				return;
			}
		}
	}
}

//��������ָ��
static struct SRecord *srp_array[MAX_RECORD];
static int srp_count = 0;
static int _comp(const void *a, const void *b)
{
	struct SRecord *sra = *((struct SRecord **)a);
	struct SRecord *srb = *((struct SRecord **)b);
	return srb -> freq - sra -> freq;
}

//Ϊÿһ��trie������ɺ�ѡ���б�
void gen_trie_each_node(struct SOTrie *so, const char *stroke_stack, int sp)
{
	int i;
	//for (i = 0; i < sp; ++i) putchar(stroke_stack[i]);
	//putchar(' ');
	
	int start = 0, end = 0;
	record_search(stroke_stack, sp, &start, &end);
	//printf("start=%d,end=%d\n", start, end);
	
	srp_count = 0;
	for (; start <= end; ++start) {
		srp_array[srp_count] = &global_srecord[start];
		srp_count ++;
	}
	qsort(srp_array, srp_count, sizeof(struct SRecord *), _comp);
	for (i = 0; i < (srp_count > MAX_DATA ? MAX_DATA : srp_count); ++i) {
		//putgbkchr(srp_array[i] -> chareter);
		if (so -> sdata == NULL) so -> sdata = sdata_new();
		sdata_insert(so -> sdata, srp_array[i] -> chareter, so -> sdata -> count);
	}
	//putchar('\n');
}

//�ݹ�����
int recur_gen(struct SOTrie *so, char *stroke_stack, int *sp)
{
	gen_trie_each_node(so, stroke_stack, *sp);
	
	int i;
	for (i = 0; i < 5; ++i) {
		if (so -> snext[i]) {
			stroke_stack[*sp] = i + '1';		//push
			(*sp)++;
			recur_gen(so -> snext[i], stroke_stack, sp);
			(*sp)--;								//pop
		}
	}
	return 0;
}

int gen_trie_from_record()
{
	char stroke_stack[64];
	int sp = 0;
	return recur_gen(&root, stroke_stack, &sp);
}

void show_trie(struct SOTrie *so, int depth)
{
	int i;
	const char *stoke[5] = { "һ", "ح", "د", "ؼ", "��" };
	if (so == NULL) return;
	for (i = 0; i < 5; ++i) {
		if (so -> snext[i]) {
			int k;
			for (k = 0; k < depth; ++k) putchar(' ');
			printf("%s: ", stoke[i]);
			int j;
			if (so -> snext[i] -> sdata) {
				for (j = 0; j < so -> snext[i] -> sdata -> count; ++j) {
					putgbkchr(so -> snext[i] -> sdata -> data[j]);
					putchar(' ');
				}
			} else printf("[NULL]");
			printf("\n");
		} else continue;
		show_trie(so -> snext[i], depth + 4);
	}
}


/**************************************************\
  ��������Ϊд����ļ�һ���̣�û�з��ļ��������
\**************************************************/
//��ѡ�ֿ�ܶ����ظ��ģ����ļ����ⲿ�ֺ�ռ�ռ䣬���ｨ����ϣ��
//�����ѡ�ֿ���ͬ����ֻд��һ��
#define HASH_SIZE	65537

struct Hash_node {
	struct SData *name;
	int data;
	struct Hash_node *next;
};

struct Hash_table {
	struct Hash_node *node[HASH_SIZE];
};

unsigned int hash_key(struct SData *s)
{
    unsigned int h = s -> count, g;
	int i;
	for (i = 0; i < s -> count; ++i) {
        h = (h << 4) + s -> data[i];
        if ((g = (h & 0xf0000000))) {
            h = h ^ (g >> 24);
            h = h ^ g;
        }
    }
    return h % HASH_SIZE;
}

void hash_insert(struct Hash_table *ht, struct SData *name, int value)
{
	int index = hash_key(name);
	
	struct Hash_node *tmp = malloc(sizeof(struct Hash_node));
	tmp -> data = value;
	tmp -> name = name;
	tmp -> next = NULL;

	if(ht -> node[index] == NULL) {
		ht -> node[index] = tmp;
		return;
	}
	struct Hash_node *p = ht -> node[index];
	while(p -> next) p = p -> next;
	p -> next = tmp;
}

//��������Ա�������ѡ�б�ϣ��û�����
int sdata_issame(struct SData *a, struct SData *b)
{
	if (a -> count != b -> count) return 0;
	unsigned int m1 = 1, m2 = 1, s1 = 0, s2 = 0;
	int i;
	for (i = 0; i < a -> count; ++i) {
		m1 *= a -> data[i];
		m2 *= b -> data[i];
		s1 += a -> data[i];
		s2 += b -> data[i];
	}
	if (m1 != m2 || s1 != s2) return 0;
	return 1;
}

int hash_search(struct Hash_table *ht, struct SData *name) {
	int index = hash_key(name);
	struct Hash_node *p = ht -> node[index];
	while(p) {
		if(sdata_issame(p -> name, name)) return p -> data;
		p = p -> next;
	}
	return -1;
}

//��ѡ�ֿ��б����ｫ���治ͬ�ĺ�ѡ�ֿ飬��ÿһ��SOTrie������ת����sdata��ָ������б�[����]
struct SData *sdata_list[100000];
int slist_count = 0;
struct Hash_table slist_hash;

//ת��trie��㣬ת����sdata�����sdata_list�±꣬ȥ���ظ���sdata
void trans_sotrie(struct SOTrie *so)
{
	if (!so -> sdata) return;
	
	//�ڹ�ϣ���в��ң����Ƿ�sdata�Ѵ���
	int h = hash_search(&slist_hash, so -> sdata);
	if (h >= 0) {		//�Ѵ��ڣ�ֱ���滻
		so -> sdata = (void *) h;
		return;			//��������ͷ�so -> sdata������ϡ������ڴ棬����ȥ��
	}
	sdata_list[slist_count] = so -> sdata;
	hash_insert(&slist_hash, so -> sdata, slist_count);	//������hash
	so -> sdata = (void *) slist_count;		//�����±�
	slist_count++;
}

//�ݹ�ת�����е�trie���sdata
void trans_sdata_trie(struct SOTrie *so)
{
	trans_sotrie(so);
	
	int i;
	for (i = 0; i < 5; ++i) {
		if (so -> snext[i]) trans_sdata_trie(so -> snext[i]);
	}
}

//д�������
void write_short(FILE *fp, unsigned short shrt)
{
	fputc((shrt & 0xff00) >> 8, fp);
	fputc(shrt & 0xff, fp);
}
//д������(4 bytes)
void write_int(FILE *fp, int intg)
{
	fputc((intg & 0xff000000) >> 24, fp);
	fputc((intg & 0xff0000) >> 16, fp);
	fputc((intg & 0xff00) >> 8, fp);
	fputc(intg & 0xff, fp);
}

int write_tmp_sdatafile()
{
	FILE *fp;
	fp = fopen("sdata.tmp", "wb");
	if (!fp) {
		fprintf(stderr, "Can not open file sdata.tmp for write\n");
		return -1;
	}
	int i;
	for (i = 0; i < slist_count; ++i) {
		int cfile_pos = ftell(fp);
		write_short(fp, sdata_list[i] -> count);	//д�����
		int j;
		for (j = 0; j < sdata_list[i] -> count; ++j) {
			write_short(fp, sdata_list[i] -> data[j]);	//д���ѡ�б�
		}
		sdata_list[i] = (void *)cfile_pos;				//��������(���λ��)�滻sdata_list
	}
	fclose(fp);
	return 0;
}

//���ڲ���, so -> sdata����ָ��sdata_list���±�
void show_trie_with_index(struct SOTrie *so, int depth)
{
	int i;
	const char *stoke[5] = { "һ", "ح", "د", "ؼ", "��" };
	if (so == NULL) return;
	for (i = 0; i < 5; ++i) {
		if (so -> snext[i]) {
			int k;
			for (k = 0; k < depth; ++k) putchar(' ');
			printf("%s: ", stoke[i]);
			int j;
			struct SData *sd = sdata_list[(int)so -> snext[i] -> sdata];
			for (j = 0; j < sd -> count; ++j) {
				putgbkchr(sd -> data[j]);
				putchar(' ');
			}
			printf("\n");
		} else continue;
		show_trie_with_index(so -> snext[i], depth + 4);
	}
}

//todo:��̬��trie����
struct SOTrie trie_buffer[100000];
int trie_buffer_count = 0;

int gen_trie_buffer(struct SOTrie *so)
{
	int index = trie_buffer_count;			//��ǰ�����soλ��
	
	memcpy(&trie_buffer[index], so, sizeof(struct SOTrie));
	trie_buffer_count++;
	
	int i;
	for (i = 0; i < 5; ++i) {
		if (so -> snext[i]) {
			trie_buffer[index].snext[i] = (void *)gen_trie_buffer(so -> snext[i]);
		}
	}
	return index * sizeof(struct SOTrie);
}

int gen_libfile(const char *filename)
{
	trans_sdata_trie(&root);		//��sdataȫ��ͳһ����ȥ����ͬ��
	if (write_tmp_sdatafile()) {	//д��sdata��ʱ�ļ�
		fprintf(stderr, "Write tmp sdata file error!");
		exit(-1);
	}
	gen_trie_buffer(&root);			//������trieд�뻺��
	
	int sdata_start_pos = trie_buffer_count * sizeof(struct SOTrie);	//��������ʼλ��
	
	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		fprintf(stderr, "Can not open file %s\n", filename);
		return -1;
	}
	int i;
	for (i = 0; i < trie_buffer_count; ++i) {		//��дtrie�������sdata������ָ����ʵ�ļ�λ��
		trie_buffer[i].sdata = 
			(void *)(((int)(sdata_list[(int)trie_buffer[i].sdata])) + sdata_start_pos);
		write_int(fp, (int) trie_buffer[i].sdata);
		int j;
		for (j = 0; j < 5; ++j) {
			write_int(fp, (int) trie_buffer[i].snext[j]);
		}
	}
	
	FILE *psd = fopen("sdata.tmp", "rb");
	int c;
	while ((c = fgetc(psd)) != EOF) fputc(c, fp);
	fclose(psd);
	fclose(fp);
	return 0;
	
	//���һЩͳ����Ϣ
	//int i, sum = 0;
	//for (i = 0; i < slist_count; ++i) {
	//	sum += (sdata_list[i] -> count * 2 + 2);
	// }
	//fprintf(stderr, "SData count %d, with %d bytes\n", slist_count, sum);
	//show_trie_with_index(&root, 0);
	//return 0;
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <stroke order data file> <output stroke order lib>\n", argv[0]);
		return -1;
	}

	//ע���ļ������ǰ���˳����õ�
	if (read_srecord(argv[1])) {
		fprintf(stderr, "Error reading records\n");
		return -1;
	}

	if (build_trie()) {
		fprintf(stderr, "Error building trie\n");
		return -1;
	}
	
	if (gen_trie_from_record()) {
		fprintf(stderr, "Error generate trie\n");
		return -1;
	}
	//show_trie(&root, 0);

	if (gen_libfile(argv[2])) {
		fprintf(stderr, "Error generating libfile\n");
		return -1;
	}
	
	return 0;
}
