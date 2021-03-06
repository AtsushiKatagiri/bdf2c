/*
Copyright (c) 2013, Atsushi Katagiri <akatagiri1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, 
  this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, 
  this list of conditions and the following disclaimer in the documentation 
  and/or other materials provided with the distribution.
* Neither the name of the <organization> nor the names of its contributors 
  may be used to endorse or promote products derived from this software 
  without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFSIZE	512

int main(int argc, char *argv[])
{
	int array_dim = 2;
	int invert = 0;
	int rotate = 0;
	char buff[BUFFSIZE];
	int comment = 1;
	int header = 1;
	int bitmap_top = 1;
	unsigned long line = 0;
	int characters = 256;
	int h_len = 16;
	int v_len = 32;
	char *token;
	int i;
	char *endptr;
	int opt;
	char bitmap[BUFFSIZE];
	long out[BUFFSIZE] = {0};

	if (isatty(fileno(stdin))) {
		printf("Usage: bdf2c [-d 1] [-i] [-r 90|180|270] < font.bdf > font.c\n");
		return 0;
	}

	while ((opt = getopt(argc, argv, "d:ir:")) != -1) {
		switch (opt) {
		case 'd':
			array_dim = atoi(optarg);
			if (array_dim < 1 || array_dim > 2) {
				fprintf(stderr, "Invalid array dimension %d\n", array_dim);
				return -1;
			}
			break;
		case 'i':
			invert = 1;
			break;
		case 'r':
			rotate = atoi(optarg) % 360;
			if (rotate < 0) {
				rotate += 360;
			}
			if (rotate % 90) {
				fprintf(stderr, "Invalid rotatin angle %d\n", rotate);
				return -1;
			}
			break;
		default:
			break;
		}
	}

	while (fgets(buff, BUFFSIZE, stdin)) {
		line++;
		if (line == 1) {
			printf("/* Generated by bdf2c */\n\n");
			printf("#include <stdint.h>\n\n");
		}
		if (comment) {
			if (header) {
				if (!strncmp(buff, "FONTBOUNDINGBOX", 15)) {
					token = strtok(buff, " ");
					token = strtok(NULL, " ");
					i = strtol(token, &endptr, 10);
					if (i > 0) {
						h_len = (i + 7) / 8 * 8;
					}
					token = strtok(NULL, " ");
					i = strtol(token, &endptr, 10);
					if (i > 0) {
						v_len = i;
					}
					printf("// ");
				} else if (!strncmp(buff, "CHARS ", 6)) {
					token = strtok(buff, " ");
					token = strtok(NULL, " ");
					i = strtol(token, &endptr, 10);
					if (i > 0) {
						characters = i;
					}
					printf("// ");
				} else if (!strncmp(buff, "STARTCHAR", 9)) {
					header = 0;
					if (rotate == 90 || rotate == 270) {
						if (array_dim == 1) {
							if (v_len <= 8) {
								printf("\nconst uint8_t font[%d] = {\n\t// ", characters * h_len);
							} else if (v_len <= 16) {
								printf("\nconst uint16_t font[%d] = {\n\t// ", characters * h_len);
							} else if (v_len <= 32) {
								printf("\nconst uint32_t font[%d] = {\n\t// ", characters * h_len);
							} else {
								printf("\nconst uint64_t font[%d] = {\n\t// ", characters * h_len);
							}
						} else {
							if (v_len <= 8) {
								printf("\nconst uint8_t font[%d][%d] = {\n\t// ", characters, h_len);
							} else if (v_len <= 16) {
								printf("\nconst uint16_t font[%d][%d] = {\n\t// ", characters, h_len);
							} else if (v_len <= 32) {
								printf("\nconst uint32_t font[%d][%d] = {\n\t// ", characters, h_len);
							} else {
								printf("\nconst uint64_t font[%d][%d] = {\n\t// ", characters, h_len);
							}
						}
					} else {
						if (array_dim == 1) {
							printf("\nconst uint%d_t font[%d] = {\n\t// ", h_len, characters * v_len);
						} else {
							printf("\nconst uint%d_t font[%d][%d] = {\n\t// ", h_len, characters, v_len);
						}
					}
				} else{
					printf("// ");
				}
			} else {
				if (!strncmp(buff, "ENDFONT", 7)) {
					printf("};\n// ");
				} else {
					printf("\t// ");
				}
			}
			fputs(buff, stdout);
			if (!strncmp(buff, "BITMAP", 6)) {
				if (array_dim != 1) {
					printf("\t{\n");
				}
				bitmap_top = 1;
				i = 0;
				memset(out, 0x00, sizeof(out));
				comment = 0;
			}
		} else {
			if (!strncmp(buff, "ENDCHAR", 7)) {
				int j, k;
				char s[BUFFSIZE] = {0};
				unsigned long o;

				// Rotation and inversion
				for (i = 0, j = 0; j < v_len; j++) {
					for (k = 0; k < h_len / 4; k++) {
						strncpy(s, &bitmap[(h_len / 4) * j + k], 1);
						i = strtol(s, &endptr, 16);	

						switch (rotate | invert) {
						case 0 | 0:
						case 360 | 0:
							out[h_len / 4 * j + k] = i;
							break;
						case 0 | 1:
						case 360 | 1:
							if (i & 8) {
								out[h_len / 4 * j + h_len / 4 - 1 - k] |= 0x1 << 0;
							}
							if (i & 4) {
								out[h_len / 4 * j + h_len / 4 - 1 - k] |= 0x1 << 1;
							}
							if (i & 2) {
								out[h_len / 4 * j + h_len / 4 - 1 - k] |= 0x1 << 2;
							}
							if (i & 1) {
								out[h_len / 4 * j + h_len / 4 - 1 - k] |= 0x1 << 3;
							}
							break;
						case 90 | 0:
							if (i & 8) {
								out[k * 4] |= 0x1 << j;
							}
							if (i & 4) {
								out[k * 4 + 1] |= 0x1 << j;
							}
							if (i & 2) {
								out[k * 4 + 2] |= 0x1 << j;
							}
							if (i & 1) {
								out[k * 4 + 3] |= 0x1 << j;
							}
							break;
						case 90 | 1:
							if (i & 8) {
								out[h_len - 1 - k * 4 - 0] |= 0x1 << j;
							}
							if (i & 4) {
								out[h_len - 1 - k * 4 - 1] |= 0x1 << j;
							}
							if (i & 2) {
								out[h_len - 1 - k * 4 - 2] |= 0x1 << j;
							}
							if (i & 1) {
								out[h_len - 1 - k * 4 - 3] |= 0x1 << j;
							}
							break;
						case 180 | 0:
							if (i & 8) {
								out[(h_len / 4 * v_len - 1) - (h_len / 4 * j  + k)] |= 0x1 << 0;
							}
							if (i & 4) {
								out[(h_len / 4 * v_len - 1) - (h_len / 4 * j  + k)] |= 0x1 << 1;
							}
							if (i & 2) {
								out[(h_len / 4 * v_len - 1) - (h_len / 4 * j  + k)] |= 0x1 << 2;
							}
							if (i & 1) {
								out[(h_len / 4 * v_len - 1) - (h_len / 4 * j  + k)] |= 0x1 << 3;
							}
							break;
						case 180 | 1:
							out[(h_len / 4) * (v_len - 1 - j) + k] = i;
							break;
						case 270 | 0:
							if (i & 8) {
								out[h_len - 1 - k * 4 - 0] |= 0x1 << (v_len - 1 - j);
							}
							if (i & 4) {
								out[h_len - 1 - k * 4 - 1] |= 0x1 << (v_len - 1 - j);
							}
							if (i & 2) {
								out[h_len - 1 - k * 4 - 2] |= 0x1 << (v_len - 1 - j);
							}
							if (i & 1) {
								out[h_len - 1 - k * 4 - 3] |= 0x1 << (v_len - 1 - j);
							}
							break;
						case 270 | 1:
							if (i & 8) {
								out[k * 4] |= 0x1 << (v_len - 1 - j);
							}
							if (i & 4) {
								out[k * 4 + 1] |= 0x1 << (v_len - 1 - j);
							}
							if (i & 2) {
								out[k * 4 + 2] |= 0x1 << (v_len - 1 - j);
							}
							if (i & 1) {
								out[k * 4 + 3] |= 0x1 << (v_len - 1 - j);
							}
							break;
						default:
							break;
						}
					}
				}

				// Print array data and font shapes
				if (rotate == 90 || rotate == 270) {
					for (i = 0; i < h_len; i++) {

						if (bitmap_top) {
							bitmap_top = 0;
							printf("\t\t 0x");
						} else {
							printf("\t\t,0x");
						}

						if ((v_len + 3) / 4 <= 1) {
							printf("%01lx\t//", out[i]);
						} else if ((v_len + 3) / 4 <= 2) {
							printf("%02lx\t//", out[i]);
						} else if ((v_len + 3) / 4 <= 4) {
							printf("%04lx\t//", out[i]);
						} else if ((v_len + 3) / 4 <= 6) {
							printf("%06lx\t//", out[i]);
						} else  {
							printf("%08lx\t//", out[i]);
						}

						for (j = ((v_len + 3) / 4 + 1) / 2 * 2; j > 0; j--) {
							o = (out[i] >> (j - 1) * 4) & 0xf;
							for (k = 3; k >= 0; k--) {
								if ((o >> k) & 0x1) {
									printf("%c", '#');
								} else {
									printf("%c", '.');
								}
							}
						}

						printf("\n");
					}
				} else {
					for (i = 0; i < v_len; i++) {
						if (bitmap_top) {
							bitmap_top = 0;
							printf("\t\t 0x");
						} else {
							printf("\t\t,0x");
						}

						for (j = 0; j < h_len / 4; j++) {
							printf("%01lx", out[h_len / 4 * i + j]);
						}

						printf("\t//");

						for (j = 0; j < h_len / 4; j++) {
							o = out[h_len / 4 * i + j] & 0xf;
							for (k = 3; k >= 0; k--) {
								if ((o >> k) & 0x1) {
									printf("%c", '#');
								} else {
									printf("%c", '.');
								}
							}
						}

						printf("\n");
					}
				}

				if (array_dim == 1) {
					printf("\t\t,\n\t// ");
				} else {
					printf("\t},\n\t// ");
				}
				fputs(buff, stdout);
				comment = 1;
			} else {
				strncpy(&bitmap[(h_len / 4) * i], buff, h_len / 4);
				i++;
			}
		}
	}

	return 0;
}
