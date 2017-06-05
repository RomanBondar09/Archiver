#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#define SIZE_BUF_IN 131072
#define SIZE_BUF_OUT 65536
#define ENTROPY_INPUT_FILE 1
#define ENTROPY_OUTPUT_FILE 2
#define BITS_PER_CHAR_OUTPUT_FILE 4
#define RUN_TIME 8
#define CHAR_CODES_FILE 16
#define CHAR_CODES_SCREEN 32
#define BINARY_TREE_CODES 64
#define SIZE_OF_HEADER_AND_DATA 128

unsigned char code, count_code, *buf_out;
size_t count_buf_out;

typedef struct node
{
	int character;
	unsigned long t;
	struct node *left, *right;
}NODE;

typedef struct symbol
{
	unsigned char *code;
	unsigned int size;
}SYMBOL;

int compare1(const void *arg1, const void *arg2)
{
	return (*(NODE**)arg2)->t - (*(NODE**)arg1)->t;
}

void my_sort(NODE **mas, int n)
{
	int i, k = n - 1;
	NODE *temp = (NODE*)malloc(sizeof(NODE));
	temp = mas[k];
	for (i = n - 2; (i > -1) && (temp->t > mas[i]->t); i--)
	{
		mas[i + 1] = mas[i];
		k = i;
	}
	mas[k] = temp;
}

NODE* build_tree(FILE *fp, int *t, int set_print, double *P)
{
	unsigned long *mas;
	int n = 0, num;
	register int i, j, k;
	unsigned long long int size_file = 0;
	NODE **m = NULL;
	register NODE *temp;
	mas = (unsigned long*)calloc(sizeof(unsigned long), 256);
	buf_out = (unsigned char*)malloc(SIZE_BUF_OUT * sizeof(unsigned char));
	while ((k = fread(buf_out, 1, SIZE_BUF_OUT, fp)) > 0)
	{
		size_file += k;
		for (i = 0; i < k; ++i)
			++mas[buf_out[i]];
	}
	for (i = 0; i < 256; ++i)
	{
		if (mas[i])
			++n;
	}
	if (set_print & ENTROPY_INPUT_FILE)
	{
		for (i = 0; i < 256; ++i)
		{
			if (mas[i])
				*P -= (double)mas[i] / size_file * log2((double)mas[i] / size_file);
		}
	}
	if (n == 0)
	{
		
		system("pause");
		exit(0);
	}
	m = (NODE**)malloc(n * sizeof(NODE*));
	for (i = 0; i < n; ++i)
		m[i] = (NODE*)malloc(n * sizeof(NODE));
	for (i = 0, j = 0; i < 256; ++i)
	{
		if (mas[i])
		{
			m[j]->character = i;
			m[j]->left = NULL;
			m[j]->right = NULL;
			m[j]->t = mas[i];
			++j;
		}
	}
	free(mas);
	qsort(m, n, sizeof(NODE*), compare1);
	num = n;
	while (n > 1)
	{
		my_sort(m, n);
		temp = (NODE*)malloc(n * sizeof(NODE));
		temp->character = -1;
		temp->t = m[n - 1]->t + m[n - 2]->t;
		temp->left = m[n - 1];
		temp->right = m[n - 2];
		m[n - 2] = temp;
		--n;
	}
	*t = num;
	return m[0];
}

void print_tree(NODE* root, int k)
{
	int i;
	if (root != NULL)
	{
		print_tree(root->right, k + 3);
		for (i = 0; i < k; i++)
			printf("   ");
		if (root->character != -1)
			printf("%u (%X)\n", root->t, root->character);
		else
			printf("%u\n", root->t);
		print_tree(root->left, k + 3);
	}
}

void put_tree(NODE* root, FILE* fout, size_t *size_of_tree)
{
	int k;
	if (count_code == 8)
	{
		buf_out[count_buf_out++] = code;
		code = 0;
		count_code = 0;
		if (count_buf_out == SIZE_BUF_OUT)
		{
			fwrite(buf_out, SIZE_BUF_OUT, 1, fout);
			*size_of_tree += SIZE_BUF_OUT * 8;
			count_buf_out = 0;
		}
	}
	if (root != NULL)
	{
		if (root->left->character == -1)
		{
			code = code << 1 | 1;
			++count_code;
			put_tree(root->left, fout, size_of_tree);
		}
		else
		{
			code <<= 1;
			k = 8 - ++count_code;
			code <<= k;
			code |= (root->left->character >> count_code);
			buf_out[count_buf_out++] = code;
			code = 0;
			if (count_buf_out == SIZE_BUF_OUT)
			{
				fwrite(buf_out, SIZE_BUF_OUT, 1, fout);
				*size_of_tree += SIZE_BUF_OUT * 8;
				count_buf_out = 0;
			}
			count_code = 8 - k;
			code |= (root->left->character & ((1 << (8 - k)) - 1));
			if (count_code == 8)
			{
				buf_out[count_buf_out++] = code;
				code = 0;
				count_code = 0;
				if (count_buf_out == SIZE_BUF_OUT)
				{
					fwrite(buf_out, SIZE_BUF_OUT, 1, fout);
					*size_of_tree += SIZE_BUF_OUT * 8;;
					count_buf_out = 0;
				}
			}
		}
		if (root->right->character == -1)
		{
			code = code << 1 | 1;
			++count_code;
			put_tree(root->right, fout, size_of_tree);
		}
		else
		{
			code <<= 1;
			++count_code;
			k = 8 - count_code;
			code <<= k;
			code |= (root->right->character >> (8 - k));
			buf_out[count_buf_out++] = code;
			code = 0;
			if (count_buf_out == SIZE_BUF_OUT)
			{
				fwrite(buf_out, SIZE_BUF_OUT, 1, fout);
				*size_of_tree += SIZE_BUF_OUT * 8;;
				count_buf_out = 0;
			}
			count_code = 8 - k;
			code |= (root->right->character & ((1 << (8 - k)) - 1));
			if (count_code == 8)
			{
				buf_out[count_buf_out++] = code;
				code = 0;
				count_code = 0;
				if (count_buf_out == SIZE_BUF_OUT)
				{
					fwrite(buf_out, SIZE_BUF_OUT, 1, fout);
					*size_of_tree += SIZE_BUF_OUT * 8;;
					count_buf_out = 0;
				}
			}
		}
	}
}

void table(NODE *root, int size, SYMBOL *table_code)
{
	int i, ch;
	static unsigned char *new_code = (unsigned char*)calloc(32, sizeof(unsigned char));
	if (root != NULL)
	{
		if (root->character != -1)
		{
			ch = root->character;
			table_code[ch].size = size;
			table_code[ch].code = (unsigned char*)calloc((size - 1) / 8 + 1, sizeof(unsigned char));
			for (i = 0; i < (size - 1) / 8 + 1; ++i)
			{
				table_code[ch].code[i] = new_code[i];
			}
			if (size % 8)
				table_code[ch].code[i - 1] <<= 8 - size % 8;
		}
		i = size / 8;
		if (root->left != NULL)
		{
			new_code[i] <<= 1;
			table(root->left, size + 1, table_code);
			i = size / 8;
			new_code[i] >>= 1;
		}
		if (root->right != NULL)
		{
			new_code[i] = new_code[i] << 1 | 1;
			table(root->right, size + 1, table_code);
			i = size / 8;
			new_code[i] >>= 1;
		}
	}
}

void print_table(SYMBOL *table_code, FILE *stream)
{
	unsigned int i, j, k, t;
	for (i = 0; i < 256; ++i)
	{
		if (table_code[i].size == 0)
			continue;
		fprintf(stream,"\n%X  ", i);
		for (k = 0, t = 0; k < table_code[i].size / 8 + 1; ++k)
		{
			for (j = 128; j && (t < table_code[i].size); j >>= 1, ++t)
			{
				fprintf(stream, "%d", (table_code[i].code[k] & j) / j);
			}
		}
	}
}

void put_inf(FILE *fp, FILE *fout, SYMBOL *table_code)
{
	size_t t;
	size_t register i, j, size;
	unsigned char *buf_in;
	register int k, s;
	register unsigned char new_code;
	buf_in = (unsigned char*)malloc(SIZE_BUF_IN * sizeof(unsigned char));
	while ((t = fread(buf_in, 1, SIZE_BUF_IN, fp)) > 0)
	{
		for (i = 0; i < t; ++i)
		{
			size = table_code[buf_in[i]].size;
			for (j = 0; j < size / 8; ++j)
			{
				new_code = table_code[buf_in[i]].code[j];
				k = 8 - count_code;
				code <<= k;
				code |= new_code >> count_code;
				buf_out[count_buf_out++] = code;
				if (k != 8)
					code = (new_code & ((1 << count_code) - 1));
				else
				{
					code = 0;
				}
				if (count_buf_out == SIZE_BUF_OUT)
				{
					fwrite(buf_out, SIZE_BUF_OUT, 1, fout);
					count_buf_out = 0;
				}
			}
			new_code = table_code[buf_in[i]].code[j];
			if (s = size - 8 * j)
			{
				k = 8 - count_code;
				if (k >= s)
				{
					code <<= s;
					code |= new_code >> (8 - s);
					count_code += s;
					if (count_code == 8)
					{
						buf_out[count_buf_out++] = code;
						count_code = code = 0;
						if (count_buf_out == SIZE_BUF_OUT)
						{
							fwrite(buf_out, SIZE_BUF_OUT, 1, fout);
							count_buf_out = 0;
						}
					}
				}
				else
				{
					code <<= k;
					code |= new_code >> (8 - k);
					count_code += k;
					if (count_code == 8)
					{
						buf_out[count_buf_out++] = code;
						count_code = code = 0;
						if (count_buf_out == SIZE_BUF_OUT)
						{
							fwrite(buf_out, SIZE_BUF_OUT, 1, fout);
							count_buf_out = 0;
						}
					}
					code |= (new_code >> (8 - s)) & ((1 << (s - k)) - 1);
					count_code += s - k;
				}
			}
		}
	}
	code <<= 8 - count_code;
	buf_out[count_buf_out++] = code;
	fwrite(buf_out, sizeof(unsigned char), count_buf_out, fout);
	rewind(fout);
	putc(8 - count_code, fout);
	
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "");
	FILE *fp, *fout, *fcode;
	clock_t t1, t2, time_tree1 = 0, time_tree2 = 0, time_put_tree1 = 0, time_put_tree2 = 0,
		time_table1 = 0, time_table2 = 0, time_put_inf1 = 0, time_put_inf2 = 0;
	int n = 0, num, 
		set_print = RUN_TIME | SIZE_OF_HEADER_AND_DATA;
	char name[80], name_out_file[80];
	NODE *root;
	SYMBOL *table_code;
	size_t size_of_tree = 0, t;
	double P_in = 0, P_out = 0;
	int i, err = 0;
	double bits_per_char = 0;
	size_t mas[256] = {0};
	if (argc < 2)
	{
		printf("Введите имя файла:");
		gets_s(name);
	}
	else
	{
		strcpy_s(name, argv[1]);
		for (int i = 2; i < argc; ++i)
		{
			if (argv[i][0] == '-')
			{
				switch (argv[i][1])
				{
				case 'e':
					printf("%d", err);
					if (argv[i][2] == 'i')
						set_print |= ENTROPY_INPUT_FILE;
					else if (argv[i][2] == 'o')
						set_print |= ENTROPY_OUTPUT_FILE;
					else err = 1;
					break;
				case 'b':
					printf("%d", err);
					if (argv[i][2] == 'c')
						set_print |= BITS_PER_CHAR_OUTPUT_FILE;
					else if (argv[i][2] == 't')
						set_print |= BINARY_TREE_CODES;
					else err = 1;
					break;
				case 'c':
					printf("%c ", argv[i][2]);
					printf("%d", err);
					if (argv[i][2] == 'f')
					{
						set_print |= CHAR_CODES_FILE;
						strcpy_s(name_out_file, argv[++i]);
					}
					else if (argv[i][2] == 's')
						set_print |= CHAR_CODES_SCREEN;
					else err = 1;
					break;
				default:
					printf("%d", err);
					err = 1;
				}
			}
			else err = 1;
		}
		if (err)
		{
			printf("%d", err);
			printf("Неверные параметры\n");
			printf("<Имя файла> [-ei] [-eo] [-bc] [-bt] [-cs] [-cf]\n");
			printf("-ei - Энтропия входного файла\n");
			printf("-eo - Энтропия выходного файла\n");
			printf("-bc - Количество бит на символ\n");
			printf("-bt - Дерево кодов\n");
			printf("-cs - Вывод кодов символов на экран\n");
			printf("-cf <имя файла для вывода> - Вывод кодов символов в файл\n");
			system("pause");
			return 0;
		}
	}
	t1 = clock();
	fopen_s(&fp, name, "rb");
	if (fp == NULL)
	{
		perror(name);
		system("pause");
		exit(1);
	}
	time_tree1 = clock();
	root = build_tree(fp, &num, set_print, &P_in);
	time_tree2 = clock();
	if (set_print & BINARY_TREE_CODES)
	{
		print_tree(root, 0);
	}
	time_put_tree1 = clock();
	strcat_s(name, ".arc");
	fopen_s(&fout, name, "wb");
	if (fout == NULL)
	{
		perror("out.arc");
		system("pause");
		exit(1);
	}
	fseek(fout, 1, SEEK_SET);
	fputc(num - 1, fout);
	if (root->character != -1)
	{
		putc(root->character, fout);
		fwrite(&root->t, 1, sizeof(unsigned long), fout);
	}
	else
	{
		put_tree(root, fout, &size_of_tree);
		size_of_tree += count_buf_out * 8 + count_code;
		time_put_tree2 = clock();

		time_table1 = clock();
		table_code = (SYMBOL*)calloc(256, sizeof(SYMBOL));
		table(root, 0, table_code);
		time_table2 = clock();
		if (set_print & CHAR_CODES_FILE)
		{
			fopen_s(&fcode, name_out_file, "w");
			print_table(table_code, fcode);
		}
		if (set_print & CHAR_CODES_SCREEN)
		{
			print_table(table_code, stdout);
		}
		rewind(fp);
		time_put_inf1 = clock();
		put_inf(fp, fout, table_code);
		time_put_inf2 = clock();
		if (set_print & BITS_PER_CHAR_OUTPUT_FILE)
		{
			n = 0;
			for (i = 0; i < 256; ++i)
			{
				if (table_code[i].size)
				{
					n++;
					bits_per_char += table_code[i].size;
				}
			}
			bits_per_char /= n;
		}
	}
	fclose(fout);
	fclose(fp);
	t2 = clock();
	if (set_print & RUN_TIME)
	{
		printf("\nВремя выполнения = %.3f\n", (double)(t2 - t1) / CLOCKS_PER_SEC);
		printf("Время построения дерева = %.3f\n", (double)(time_tree2 - time_tree1) / CLOCKS_PER_SEC);
		printf("Время записи дерева = %.3f\n", (double)(time_put_tree2 - time_put_tree1) / CLOCKS_PER_SEC);
		printf("Время построения таблицы = %.3f\n", (double)(time_table2 - time_table1) / CLOCKS_PER_SEC);
		printf("Время записи информации = %.3f\n", (double)(time_put_inf2 - time_put_inf1) / CLOCKS_PER_SEC);
	}
	if (set_print & SIZE_OF_HEADER_AND_DATA)
	{
		printf("Размер заголовка в битах = %f\n", (double)size_of_tree / num);
	}
	if (set_print & BITS_PER_CHAR_OUTPUT_FILE)
		printf("Бит на символ: %lf\n", bits_per_char);
	if (set_print & ENTROPY_INPUT_FILE)
		printf("Энтропия входного файла: %lf\n", P_in);
	if (set_print & ENTROPY_OUTPUT_FILE)
	{
		fopen_s(&fout, name, "rb");
		n = 0;
		while ((t = fread(buf_out, 1, SIZE_BUF_OUT, fout)) > 0)
		{
			n += t;
			for (i = 0; i < t; ++i)
			{
				++mas[buf_out[i]];
			}
		}
		for (i = 0; i < 256; ++i)
		{
			if (mas[i])
			{
				P_out -= (double)mas[i] / n * log2((double)mas[i] / n);
			}
		}
		printf("Энтропия выходного файла: %lf\n", P_out);
	}
	free(buf_out);
	system("pause");
	return 0;
}