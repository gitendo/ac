#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ac.h"

void banner(void)
{
	printf("\nAdvanced Compare v%.2f - binary compare utility\n", VERSION);
	printf("Programmed by: tmk, email: tmk@tuta.io\n");
	printf("Project page: https://github.com/gitendo/ac/\n\n");
}


// haelp!
void usage(void)
{
	banner();

	printf("Syntax: ac.exe [options] file1 file2 [file3 ...]\n\n");
	printf("Options:\n");
	
	printf("\t/1    compare bytes  \\\n");
	printf("\t/2    compare words   - can be suffixed with + or - to display offsets with gradually (inc/dec)reasing values only.\n");
	printf("\t/4    compare dwords /\n");
	printf("\t/i    incremental mode (value in next file is always higher than in previous)\n");
	printf("\t/d    decremental mode (value in next file is always lower than in previous)\n");
	printf("\t/e    display only equal offsets\n");
	printf("\t/v#   hexadecimal value in first file to focus on, others are omitted\n\n");
	printf("By default byte mode is used and only differences are shown.\n");
	printf("\n");

	exit(EXIT_FAILURE);
}


void print(unsigned int buffer[], char flags[], char files, unsigned int ofs, char size, unsigned int *diff)
{
	char	file, i;

	printf("%0.8x:", ofs);
	for(file = 0; file < files; file++)											// loop through them
	{
		if(flags[file] == size)													// full match
		{
			switch(size)
			{
				case BYTE:
					printf("\t%0.2x", buffer[file]);
					break;

				case WORD:
					printf("\t%0.4x", buffer[file]);
					break;

				case DWORD:
					printf("\t%0.8x", buffer[file]);
					break;
			}
		}
		else if(flags[file] == 0)												// no match
		{
			printf("\t");
			for(i = 0; i < size; i++)
				printf("--");
		}
		else																	// half match
		{
			switch(size)
			{
				case WORD:
					printf("\t--%0.2x", buffer[file]);
					break;

				case DWORD:
					printf("\t");
					for(i = 0; i < (size - flags[file]); i++)
						printf("--");
					switch(flags[file])
					{
						case BYTE:
							printf("%0.2x", buffer[file]);
							break;
						case WORD:
							printf("%0.4x", buffer[file]);
							break;
						case 3:
							printf("%0.6x", buffer[file]);
							break;
					}

					break;
			}
		}
	}
	printf("\n");
	(*diff)++;
}


int state(char flags[])
{
	int		i, j = 0;

	for(i = 0; i < FOPEN_MAX; i++)
		j += flags[i];

	return(j);
}


void closer(FILE *fp[])
{
	for(int i = 0; i < FOPEN_MAX; i++)
	{
		if(fp[i] != NULL)
			fclose(fp[i]);
		else
			break;
	}
}


int main (int argc, char *argv[])
{
	char			arg, f, file = 0, files = 0, flags[FOPEN_MAX], mode = 0, size = 1, v = 0;
	unsigned int	buffer[FOPEN_MAX], diff = 0, ofs = 0, val = 0;
	FILE			*fp[FOPEN_MAX];


	memset(buffer, 0, sizeof(buffer));											// clear arrays
	memset(flags, 0, sizeof(flags));
	memset(fp, 0, sizeof(fp));

	for(arg = 1; arg < argc; arg++)												// parse options
	{
		if(argv[arg][0] == '/')
		{
			switch(tolower(argv[arg][1]))
			{
				case '1': 
					size = 1;
					switch(argv[arg][2])
					{
						case '+': mode = PLUS; break;
						case '-': mode = MINUS; break;
						default: break;
					}
					break;

				case '2':
					size = 2;
					switch(argv[arg][2])
					{
						case '+': mode = PLUS; break;
						case '-': mode = MINUS; break;
						default: break;
					}
					break;

				case '4':
					size = 4;
					switch(argv[arg][2])
					{
						case '+': mode = PLUS; break;
						case '-': mode = MINUS; break;
						default: break;
					}
					break;

				case 'd': mode = DEC; break;
				case 'e': mode = EQU; break;
				case 'i': mode = INC; break;

				case 'v':
					v = 1;
					val = strtol(&argv[arg][2], NULL, 16);						// hex
					break;

				default: break;
			}
		}
		else
		{
			fp[file] = fopen(argv[arg], "rb");									// open files
			if(fp[file])
			{
				flags[file] = size;
				file++;
			}
			else
			{
				printf("Error: unable to access %s\n", argv[arg]);
				closer(fp);
				exit(EXIT_FAILURE);
			}
		}
	}

	files = file;

	if(files < 2)																// need something to compare
	{
		usage();
		closer(fp);
		exit(EXIT_FAILURE);
	}

	while(1)
	{
		for(file = 0; file < files; file++)										// read from each file being compared
		{
			if(flags[file])
			{
				buffer[file] = 0;												// zero eof data entries
				flags[file] = fread(&buffer[file], 1, size, fp[file]);
			}
		}

		if(!state(flags))														// all files reached eof
			break;

		if(state(flags) == files * size && mode > 0)							// no eofs or uneven data length
		{
			if(v == 0 || (flags[0] && buffer[0] == val))
			{
				switch(mode)													// take care of data size
				{
					case PLUS:
						for(f = 0; f < (files - 1); f++)						// loop through them
						{
							if(buffer[f] != (buffer[f + 1] - 1))
								break;
						}

						f++;

						if(f == files)
							print(buffer, flags, files, ofs, size, &diff);	// output match
						break;

					case MINUS:
						for(f = 0; f < (files - 1); f++)
						{
							if(buffer[f] != (buffer[f + 1] + 1))
								break;
						}

						f++;

						if(f == files)
							print(buffer, flags, files, ofs, size, &diff);
						break;

					case INC:
						for(f = 0; f < (files - 1); f++)
						{
							if(buffer[f] >= buffer[f + 1])
								break;
						}

						f++;

						if(f == files)
							print(buffer, flags, files, ofs, size, &diff);
						break;

					case DEC:
						for(f = 0; f < (files - 1); f++)
						{
							if(buffer[f] <= buffer[f + 1])
								break;
						}

						f++;

						if(f == files)
							print(buffer, flags, files, ofs, size, &diff);
						break;

					case EQU:
						for(f = 0; f < (files - 1); f++)
						{
							if(buffer[f] != buffer[f + 1])
								break;
						}

						f++;

						if(f == files)
							print(buffer, flags, files, ofs, size, &diff);
						break;
				}
			}
		}
		else
		{
			if(v == 0 || (flags[0] && buffer[0] == val))
			{
				if(v == 1)
					print(buffer, flags, files, ofs, size, &diff);
				else
				{
					for(f = 0; f < (files - 1); f++)								// compare n-1 files
					{
						if(!flags[f] || !flags[f+1])								// adjacent files has been read successfully
							break;

						if(buffer[f] != buffer[f + 1])								// compare data
							break;
					}

					f++;

					if(f != files)
						print(buffer, flags, files, ofs, size, &diff);
				}
			}
		}

		ofs += size;
	}

	closer(fp);

	if(mode > 0)
		printf("\n%d match(es) found.\n", diff);
	else
		printf("\n%d difference(s) found.\n", diff);
}
