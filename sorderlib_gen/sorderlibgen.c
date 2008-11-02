/*
* sorderlibgen.c - 生成笔顺输入法库
* Copyright (c) 2008, wenxichang@163.com
* All rights reserved.
*
* 本文件使用BSDL协议发布
*
* 本程序读入汉字笔顺表格 - 一个有5列的文本文件
* <笔顺> <笔划数> <GBK码> <汉字[无用列]> <汉字出现频率>
* 程序没有进行任何输入错误处理 —— 我们的目的不是程序，而是它生成的库
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//存放备选字数据
struct SData {
	unsigned short count;		//备选字数目
	unsigned short *data;		//备选字
};

#define MAX_DATA	2000		//备选字支持400
#define MAX_RECORD	21000		//最大汉字条目数

struct SOTrie {
	struct SData *sdata;		//备选字数据
	struct SOTrie *snext[5];		//五种笔划指针
};

struct SRecord {
	char sorder[50];			//笔顺
	int scount;					//笔划
	unsigned short chareter;	//字符
	int freq;					//字频
};


struct SRecord global_srecord[MAX_RECORD];		//目前只处理这么多汉字
int global_srecord_count = 0;
struct SOTrie root;
struct SRecord *grecord_index[65536];			//用于汉字反向查找

//生成笔划Trie结点
struct SOTrie *sotrie_new()
{
	struct SOTrie *ret = malloc(sizeof(struct SOTrie));
	memset(ret, 0, sizeof(struct SOTrie));
	return ret;
}

//字符串转换为short gbk
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

//打印short gbk
void putgbkchr(unsigned short chr)
{
	char b[3];
	b[0] = (chr & 0xff00) >> 8;
	b[1] = chr & 0xff;
	b[2] = 0;
	printf("%s", b);
}

//打开文件并读入记录
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

//笔划转索引
int stroke_to_index(char chr)
{
	return chr - '1';
}

//新建候选字数据
struct SData *sdata_new()
{
	struct SData *ret = malloc(sizeof(struct SData));
	memset(ret, 0, sizeof(struct SData));
	return ret;
}

//r值表示对插入chr进行存在验证，如果在r前存在，则不插入
void sdata_insert(struct SData *sd, unsigned short chr, int r)
{
	if (sd -> count == 0) {
		sd -> data = malloc(sizeof(unsigned short) * MAX_DATA);
	}
	if (sd -> count >= MAX_DATA) return;
	
	int i;
	if (r > sd -> count) r = sd -> count;
	for (i = 0; i < r; ++i) {				//是否与前r个字冲突
		if (sd -> data[i] == chr) return;
	}
	
	if (r == -1) {							//为初始插入
		for (i = sd -> count; i > 0; --i) {	//以字频排序
			if (grecord_index[sd -> data[i - 1]] -> freq < grecord_index[chr] -> freq) {
				sd -> data[i] = sd -> data[i - 1];
			} else break;
		}
		sd -> data[i] = chr;
		sd -> count++;
	} else {								//期望已经排序好
		sd -> data[sd -> count] = chr;
		sd -> count++;
	}
}

//将笔顺/字符插入到trie中
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

//搜索笔顺序列str(len个字节)在record中存在的起始、结束结点
void record_search(const char *str, int len, int *start, int *end)
{
	//todo:两分法查找
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

//用于排序指针
static struct SRecord *srp_array[MAX_RECORD];
static int srp_count = 0;
static int _comp(const void *a, const void *b)
{
	struct SRecord *sra = *((struct SRecord **)a);
	struct SRecord *srb = *((struct SRecord **)b);
	return srb -> freq - sra -> freq;
}

//为每一个trie结点生成候选字列表
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

//递归生成
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
	const char *stoke[5] = { "一", "丨", "丿", "丶", "乙" };
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
  从这里起，为写入库文件一例程，没有分文件，请见谅
\**************************************************/
//候选字块很多是重复的，在文件中这部分很占空间，这里建立哈希表
//如果候选字块相同，则只写入一份
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

//这个用来对比两个候选列表，希望没有误差
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

//候选字块列表，这里将储存不同的候选字块，对每一个SOTrie结点进行转换后，sdata将指向这个列表[索引]
struct SData *sdata_list[100000];
int slist_count = 0;
struct Hash_table slist_hash;

//转换trie结点，转换后sdata将变成sdata_list下标，去除重复的sdata
void trans_sotrie(struct SOTrie *so)
{
	if (!so -> sdata) return;
	
	//在哈希表中查找，看是否sdata已存在
	int h = hash_search(&slist_hash, so -> sdata);
	if (h >= 0) {		//已存在，直接替换
		so -> sdata = (void *) h;
		return;			//这里最好释放so -> sdata，但不稀罕这点内存，让它去吧
	}
	sdata_list[slist_count] = so -> sdata;
	hash_insert(&slist_hash, so -> sdata, slist_count);	//插入至hash
	so -> sdata = (void *) slist_count;		//换成下标
	slist_count++;
}

//递归转换所有的trie结点sdata
void trans_sdata_trie(struct SOTrie *so)
{
	trans_sotrie(so);
	
	int i;
	for (i = 0; i < 5; ++i) {
		if (so -> snext[i]) trans_sdata_trie(so -> snext[i]);
	}
}

//写入短整形
void write_short(FILE *fp, unsigned short shrt)
{
	fputc((shrt & 0xff00) >> 8, fp);
	fputc(shrt & 0xff, fp);
}
//写入整形(4 bytes)
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
		write_short(fp, sdata_list[i] -> count);	//写入个数
		int j;
		for (j = 0; j < sdata_list[i] -> count; ++j) {
			write_short(fp, sdata_list[i] -> data[j]);	//写入待选列表
		}
		sdata_list[i] = (void *)cfile_pos;				//将此数据(相对位置)替换sdata_list
	}
	fclose(fp);
	return 0;
}

//用于测试, so -> sdata将是指向sdata_list的下标
void show_trie_with_index(struct SOTrie *so, int depth)
{
	int i;
	const char *stoke[5] = { "一", "丨", "丿", "丶", "乙" };
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

//todo:动态的trie缓存
struct SOTrie trie_buffer[100000];
int trie_buffer_count = 0;

int gen_trie_buffer(struct SOTrie *so)
{
	int index = trie_buffer_count;			//当前处理的so位置
	
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
	trans_sdata_trie(&root);		//将sdata全部统一处理，去除相同的
	if (write_tmp_sdatafile()) {	//写入sdata临时文件
		fprintf(stderr, "Write tmp sdata file error!");
		exit(-1);
	}
	gen_trie_buffer(&root);			//将所有trie写入缓存
	
	int sdata_start_pos = trie_buffer_count * sizeof(struct SOTrie);	//数据区开始位置
	
	FILE *fp = fopen(filename, "wb");
	if (!fp) {
		fprintf(stderr, "Can not open file %s\n", filename);
		return -1;
	}
	int i;
	for (i = 0; i < trie_buffer_count; ++i) {		//改写trie缓存里的sdata，让其指向真实文件位置
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
	
	//输出一些统计信息
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

	//注意文件必需是按笔顺排序好的
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
