#include <stdio.h>
#include <stdlib.h>

struct Item {
	int data_pos;
	int stroke[5];
};

//´ó¶Ë big-endian ´æ´¢
int readint(FILE *fp)
{
	int ret = 0;
	ret += ((fgetc(fp) & 0xff) << 24);
	ret += ((fgetc(fp) & 0xff) << 16);
	ret += ((fgetc(fp) & 0xff) << 8);
	ret += (fgetc(fp) & 0xff);
	return ret;
}

unsigned short readshort(FILE *fp)
{
	unsigned short ret = 0;
	ret += ((fgetc(fp) & 0xff) << 8);
	ret += (fgetc(fp) & 0xff);
	return ret;
}

int readitem(FILE *fp, struct Item *it, int pos)
{
	fseek(fp, pos, SEEK_SET);
	it -> data_pos = readint(fp);
	int i;
	for (i = 0; i < 5; ++i) {
		it -> stroke[i] = readint(fp);
	}
	return 0;
}

void output_data(FILE *fp, int pos)
{
	fseek(fp, pos, SEEK_SET);
	unsigned short count = readshort(fp);
	
	int i;
	for (i = 0; i < count; ++i) {
		putchar(fgetc(fp));
		putchar(fgetc(fp));
		putchar(' ');
	}
}

void output_item(FILE *fp, struct Item *item, int depth)
{
	const char *stoke[5] = { "Ò»", "Ø­", "Ø¯", "Ø¼", "ÒÒ" };
	int i;
	for (i = 0; i < 5; ++i) {
		if (item -> stroke[i]) {
			int k;
			for (k = 0; k < depth; ++k) putchar(' ');
			printf("%s: ", stoke[i]);
			
			struct Item nextitem;
			readitem(fp, &nextitem, item -> stroke[i]);
			
			output_data(fp, nextitem.data_pos);
				
			printf("\n");
			output_item(fp, &nextitem, depth + 4);
		}
	}
}

int main(int argc, char **argv)
{
	fprintf(stderr, "This is a test tool of libsscsoim\n");
	if (!argv[1]) {
		fprintf(stderr, "Usage: readlib <libfile>\n");
		return 0;
	}
	FILE *fp = fopen(argv[1], "rb");
	if (!fp) {
		fprintf(stderr, "Error reading file %s\n", argv[1]);
		return -1;
	}
	
	struct Item root;
	readitem(fp, &root, 0);
	//printf("data: %d, %d %d %d %d %d\n", root.data_pos, root.stroke[0], root.stroke[1], root.stroke[2], root.stroke[3], root.stroke[4]);
	output_item(fp, &root, 0);	

	return 0;
}
