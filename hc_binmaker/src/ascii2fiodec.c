/*
 *  Copyright 2006 Peter Samson.  All Rights Reserved.
 *
 *  Usage notes below by Ken Sumrall, written from a badly fading memory.
 *
 *  Usage: If invoked with the -a flag, read FIODEC encoded characters on
 *         stdin, and write ASCII encoded characters on stdout.
 *         If invoked with the -f flag, read ASCII encoded characters on
 *         stdin, and write FIODEC encoded characters on stdout.
 *         If invoked with no flags, dump data in a format used by Peter
 *         to decode the music intermediate format paper tape images.
 *         When converting from ascii to fiodec, an ascii '@' character on
 *         input translates to octal 013.  This is the FIODEC stop command
 *         or something like that.  It's used to seperate the voices on the
 *         input to the harmony compiler.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef MAC
#include <console.h>
#endif /* MAC */

int ascii = 0;
int fiodec = 0;

int upper[0100] = {
	' ', '"', '\'', '{', '}', '|', '&', '<',
	'>', '!', 0, '@', 0, 0, 0, 0,
	':', '?', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', 0, '=', 0, 0, '\t', 0,
	'_', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 0, 0, '+', ']', '%', '[',
	0, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'I', 0, '#', 0, '\b', 0, 0 
};
int lower[0100] = {
	' ', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 0, '@', 0, 0, 0, 0,
	'0', '/', 's', 't', 'u', 'v', 'w', 'x',
	'y', 'z', 0, ',', 0, 0, '\t', 0,
	';', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	'q', 'r', 0, 0, '-', ')', '~', '(',
	0, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 0, '.', 0, '\b', 0, 0
};

void newlin(int);

void newlin(int skip)
{
	do
		printf("\n");
	while (--skip >= 0);
}

void putpar(int);

void putpar(int ch)
{
	int v, q;
	for (v = ch, q = 0; v; v = v >> 1)
		if (v & 1)
			q++;
	putchar((q & 1) ? ch : ch+0200);
}

int main(int argc, char *argv[])
{
	int col, skp, cyc, nco, nch, c, val, dot;

#ifdef MAC
	argc = ccommand(&argv);
#endif /* MAC */
	
	col = 8;
	skp = 1;

/*	for (i = 0; i < argc; i++)
		printf("%d. %s\n", i, argv[i]);
*/
	for (dot = 1; dot < argc; dot++) {
		c = argv[dot][0];
		if (c == '-' || c == '/') {
			switch (tolower(argv[dot][1])) {
				case 'a':
					ascii = 1;
					break;
				case 'f':
					fiodec = 1;
					break;
			}
		}
		else break;
	}

	if (argc > dot)
		col = atoi(argv[dot]);
	if (argc > dot+1)
		skp = atoi(argv[dot+1]);
	
/*	printf("%d %d\n", col, skp);
*/

	if (ascii) {
		int uc = 0;
		for (c = getchar(); c != EOF; c = getchar()) {
			switch (c) {
				case 0272:
					uc = 0;
					break;
				case 0274:
					uc = 1;
					break;
				case 0277:
					putchar('\n');
					break;
				default:
					for (nco = 0, nch = c; nch != 0; nch = nch >> 1)
						if (nch & 1)
							nco++;
					if ((nco & 1) == 0)
						break;
					if (c & 0100)
						break;
					if (uc)
						dot = upper[c & 077];
					else
						dot = lower[c & 077];
					if (dot != 0)
						putchar(dot);
			}
		}		
		return  0;
	}
	
	if (fiodec) {
		int uc = 0;
		int i;
		for (c = getchar(); c != EOF; c = getchar()) {
			if (c == ' ')
				putchar(0200);
			else if (c == '\t')
				putchar(0236);
			else if (c == '\n')
				putchar(0277);				
			else for (i = 0; i < 100; i++) {
				if (c == upper[i]) {
					if (!uc) {
						putchar(0274);
						uc = 1;
					}
					putpar(i);
					break;
				}
				if (c == lower[i]) {
					if (uc) {
						putchar(0272);
						uc = 0;
					}
					putpar(i);
					break;
				}
			}
		}
		putchar(013);		
		return 0;
	}

	for (nch = cyc = val = nco = dot = 0; ; nch++) {
		c = getchar();
		if (c == EOF) {
			dot = 0;
			break;
		}
		if (c < 0200) {
			if (dot < 0) {
				newlin(skp);
				nco = 0;
			}
			dot = 1;
			putchar('.');
			if (++nco > col * 8) {
				newlin(skp);
				nco = dot = 0;
			}
		}
		else if ((c & 0100) != 0)
			continue;
		else {
			if (dot > 0) {
				newlin(skp);
				nco = 0;
			}
			dot = -1;
			val = (val << 6) | (c - 0200);
			if (++cyc >= 3) {
				printf("%06o", val);
				val = cyc = 0;
				if (++nco > col) {
					newlin(skp);
					nco = dot = 0;
				}
				else
					printf("  ");
			}
		}
	}
	
	return 0;
}
