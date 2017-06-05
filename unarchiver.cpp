#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#define SIZE_BUF_IN 65536
#define SIZE_BUF_OUT 65536

unsigned char *buf_in, *buf_out, code, count_code;
size_t count_buf;

typedef struct node
{
	int character;
	struct node *left, *right, *root;
}NODE;

void print_tree(NODE* root, int k)
{
	int i;
	if (root != NULL)
	{
		print_tree(root->right, k + 3);
		for (i = 0; i < k; i++)
			printf(" ");
		if (root->character != -1)
			printf("--(%X)\n", root->character);
		else
			printf("--\n");
		print_tree(root->left, k + 3);
	}
}

NODE* build_tree(FILE *fin, int num, size_t *t1)
{
	size_t t = 0;
	int n = 0;
	NODE *temp, *current_node, *root;
	unsigned long register j;
	root = (NODE*)malloc(sizeof(NODE));
	root->character = -1;
	root->left = root->right = root->root = NULL;
	current_node = root;
	while (n < num)
	{
		if (count_buf == t)
		{
			t = fread(buf_in, 1, SIZE_BUF_IN, fin);
			count_buf = 0;
			code = buf_in[count_buf++];
		}
		while (count_buf < t)
		{
			for (j = 128 >> count_code; j && code & j; j >>= 1)
			{
				temp = (NODE*)malloc(sizeof(NODE));
				temp->left = temp->right = NULL;
				temp->root = current_node;
				temp->character = -1;
				++count_code;
				if (current_node->left == NULL)
					current_node = current_node->left = temp;
				else
					current_node = current_node->right = temp;
			}
			if (j == 0)
			{
				code = buf_in[count_buf++];
				count_code = 0;
				continue;
			}
			temp = (NODE*)malloc(sizeof(NODE));
			temp->left = temp->right = NULL;
			temp->root = current_node;
			temp->character = (unsigned char)(code << ++count_code);
			if (count_buf == t && n < num)
			{
				t = fread(buf_in, 1, SIZE_BUF_IN, fin);
				count_buf = 0;
			}
			code = buf_in[count_buf++];
			temp->character |= code >> (8 - count_code);
			if (current_node->left == NULL)
				current_node->left = temp;
			else
				current_node->right = temp;
			if (++n == num)	
				break;
			while (current_node->left && current_node->right)
			{
				if (current_node == root)
					break;
				current_node = current_node->root;
			}
		}
	}
	*t1 = t;
	return root;
}

void get_inf(FILE *fin, FILE *fout, NODE* root, size_t size, int size_last_bite)
{
	NODE *current_node = root;
	register size_t t, count_buf_out = 0;
	register int i;
	buf_out = (unsigned char*)malloc(SIZE_BUF_OUT);
	for (i = 128 >> count_code; i; i >>= 1)
	{
		if (code & i)
			current_node = current_node->right;
		else
			current_node = current_node->left;
		if (current_node->character != -1)
		{
			buf_out[count_buf_out++] = current_node->character;
			current_node = root;
			if (count_buf_out == SIZE_BUF_OUT)
			{
				fwrite(buf_out, 1, SIZE_BUF_OUT, fout);
				count_buf_out = 0;
			}
		}
	}
	while (count_buf < size)
	{
		code = buf_in[count_buf++];
		for (i = 128; i; i >>= 1)
		{
			if (code & i)
				current_node = current_node->right;
			else
				current_node = current_node->left;
			if (current_node->character != -1)
			{
				buf_out[count_buf_out++] = current_node->character;
				current_node = root;
				if (count_buf_out == SIZE_BUF_OUT)
				{
					fwrite(buf_out, 1, SIZE_BUF_OUT, fout);
					count_buf_out = 0;
				}
			}
		}
	}
	while ((t = fread(buf_in, 1, SIZE_BUF_IN, fin)) == SIZE_BUF_IN)
	{
		count_buf = 0;
		while (count_buf < t)
		{
			code = buf_in[count_buf++];
			for (i = 128; i; i >>= 1)
			{
				if (code & i)
					current_node = current_node->right;
				else
					current_node = current_node->left;
				if (current_node->character > -1)
				{
					buf_out[count_buf_out++] = current_node->character;
					current_node = root;
					if (count_buf_out == SIZE_BUF_OUT)
					{
						fwrite(buf_out, 1, SIZE_BUF_OUT, fout);
						count_buf_out = 0;
					}
				}
			}
		}
	}
	count_buf = 0;
	while ((long)count_buf < (long)t - 1)
	{
		code = buf_in[count_buf++];
		for (i = 128; i; i >>= 1)
		{
			if (code & i)
				current_node = current_node->right;
			else
				current_node = current_node->left;
			if (current_node->character != -1)
			{
				buf_out[count_buf_out++] = current_node->character;
				current_node = root;
				if (count_buf_out == SIZE_BUF_OUT)
				{
					fwrite(buf_out, 1, SIZE_BUF_OUT, fout);
					count_buf_out = 0;
				}
			}
		}
	}
	code = buf_in[count_buf++];
	for (i = 128; i > (1 << (size_last_bite - 1)); i >>= 1)
	{
		if (code & i)
			current_node = current_node->right;
		else
			current_node = current_node->left;
		if (current_node->character != -1)
		{
			buf_out[count_buf_out++] = current_node->character;
			current_node = root;
			if (count_buf_out == SIZE_BUF_OUT)
			{
				fwrite(buf_out, 1, SIZE_BUF_OUT, fout);
				count_buf_out = 0;
			}
		}
	}
	fwrite(buf_out, 1, count_buf_out, fout);
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	char name[80], ch;
	unsigned long s;
	clock_t t1, t2, time_tree1, time_tree2, time_get_inf1, time_get_inf2;
	int n = 0, num, size_last_bite;
	FILE *fin, *fout;
	NODE *root;
	size_t t;
	if (argc < 2)
	{
		printf("Введите имя файла:");
		gets_s(name);
	}
	else
		strcpy_s(name, argv[1]);
	fopen_s(&fin, name, "rb");
	if (fin == NULL)
	{
		perror(name);
		system("pause");
		exit(1);
	}
	t1 = clock();
	buf_in = (unsigned char*)malloc(SIZE_BUF_IN);
	size_last_bite = fgetc(fin);
	num = fgetc(fin) + 1;
	name[strlen(name) - 4] = 0;
	strcat_s(name, "(1)");
	fopen_s(&fout, name, "wb");
	if (fout == NULL)
	{
		perror(name);
		exit(1);
	}
	if (num == 1)
	{
		ch = fgetc(fin);
		fread(&s, sizeof(unsigned long), 1, fin);
		memset(buf_in, ch, SIZE_BUF_OUT);
		while (s > SIZE_BUF_OUT)
		{
			fwrite(buf_in, 1, SIZE_BUF_OUT, fout);
			s -= SIZE_BUF_OUT;
		}
		fwrite(buf_in, 1, s, fout);
		t2 = clock();
		printf("\nВремя выполнения = %.3f\n", (double)(t2 - t1) / CLOCKS_PER_SEC);
		system("pause");
		return 0;
	}
	time_tree1 = clock();
	root = build_tree(fin, num, &t);
	time_tree2 = clock();
	time_get_inf1 = clock();
	get_inf(fin, fout, root, t, size_last_bite);
	time_get_inf2 = clock();
	free(buf_in);
	free(buf_out);
	fclose(fin);
	fclose(fout);
	t2 = clock();
	printf("\nВремя выполнения = %.3f\n", (double)(t2 - t1) / CLOCKS_PER_SEC);
	printf("Время построения дерева = %.3f\n", (double)(time_tree2 - time_tree1) / CLOCKS_PER_SEC);
	printf("Время записи информации = %.3f\n", (double)(time_get_inf2 - time_get_inf1) / CLOCKS_PER_SEC);
	system("pause");
	return 0;
}