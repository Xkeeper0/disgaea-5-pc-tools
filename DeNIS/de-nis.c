/*
 * Copyright (c) 2018 "Kaito Sinclaire"
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <windows.h>

FILE *fin, *fout;

char stdin_prompt_char(char *prompt, char default_response)
{
	char response;

	printf("%s [%c] ", prompt, default_response);
	response = tolower(getchar());

	if (response == '\n')
		return default_response;
	while (getchar() != '\n'); // flush stdin
	return response;
}

char *stdin_prompt_string(char *prompt, char *default_response)
{
	static char response[256];
	char *i;
	printf("%s [%s] ", prompt, default_response);
	fgets(response, 255, stdin);
	i = response + strlen(response) - 1;
	if (*i == '\n')
		*i = 0;
	else
		while (getchar() != '\n'); // stdin may have content; flush

	if (!strlen(response))
		strncpy(response, default_response, 255);
	return response;
}

void error(char *str)
{
	printf("%s\r\n", str);
	if (fin) fclose(fin);
	if (fout) fclose(fout);
	exit(-1);
}

// -----

DWORD WINAPI _decode(LPVOID unused)
{
	unsigned char b[8];
	unsigned long long *bx = (unsigned long long*)(&b[0]);
	size_t len;

	while (len = fread(b, 1, 8, fin))
	{
		// note: single 64 bit xor operation (optimizing for speed)
		*bx ^= 0xF0F0F0F0F0F0F0F0;
		// note: loop unrolling (optimizing for speed)
		b[0] = (b[0] << 4) | (b[0] >> 4);
		b[1] = (b[1] << 4) | (b[1] >> 4);
		b[2] = (b[2] << 4) | (b[2] >> 4);
		b[3] = (b[3] << 4) | (b[3] >> 4);
		b[4] = (b[4] << 4) | (b[4] >> 4);
		b[5] = (b[5] << 4) | (b[5] >> 4);
		b[6] = (b[6] << 4) | (b[6] >> 4);
		b[7] = (b[7] << 4) | (b[7] >> 4);
		fwrite(b, 1, len, fout);
	}
	return 0;
}

DWORD WINAPI _encode(LPVOID unused)
{
	unsigned char b[8];
	unsigned long long *bx = (unsigned long long*)(&b[0]);
	size_t len;

	while (len = fread(b, 1, 8, fin))
	{
		// note: loop unrolling (optimizing for speed)
		b[0] = (b[0] << 4) | (b[0] >> 4);
		b[1] = (b[1] << 4) | (b[1] >> 4);
		b[2] = (b[2] << 4) | (b[2] >> 4);
		b[3] = (b[3] << 4) | (b[3] >> 4);
		b[4] = (b[4] << 4) | (b[4] >> 4);
		b[5] = (b[5] << 4) | (b[5] >> 4);
		b[6] = (b[6] << 4) | (b[6] >> 4);
		b[7] = (b[7] << 4) | (b[7] >> 4);
		// note: single 64 bit xor operation (optimizing for speed)
		*bx ^= 0xF0F0F0F0F0F0F0F0;
		fwrite(b, 1, len, fout);
	}
	return 0;
}

// -----

void run_track_thread(DWORD WINAPI (*func)(LPVOID))
{
	size_t fsize = 0, fcur = 0;
	double pct = 0.0f;
	HANDLE thread;

	fseek(fin, 0, SEEK_END);
	fsize = ftell(fin);
	fseek(fin, 0, SEEK_SET);

	thread = CreateThread(NULL, 0, func, NULL, 0, NULL);
	if (!thread)
		error("error: couldn't create encoding/decoding thread");
	while (WaitForSingleObject(thread, 50))
	{
		fcur = ftell(fin);
		pct = (double)fcur / fsize * 100;
		printf("\r%12d / %12d [%5.1f%%]", fcur, fsize, pct);
	}
	printf("\r%12d / %12d [100.0%%]\n", fsize, fsize);
}

// -----

int main(int argc, char *argv[])
{
	char mode_char = 1, default_mode_char = 'd', confirm_char = 1;
	char *c, *c_alt, default_c[256];
	char dummy[64];

	printf("de-nis: D5PC file decoder/encoder\r\n~ KS\r\n\r\n");
	if (argc < 2)
		error("usage: de-nis [filename]");
	if (!(fin = fopen(argv[1], "rb")))
		error("error: can't open input file (does it exist?)");

	chdir(argv[1]);	// chdir to the input folder for simplicity
	c = argv[1] + strlen(argv[1]) - 4;
	if (c >= argv[1] && !strncmp(c, ".dec", 4)) // if already a decoded file,
		default_mode_char = 'e'; // set default mode to encode

	while (mode_char != 'e' && mode_char != 'd')
		mode_char = stdin_prompt_char("(d)ecode or (e)ncode?", default_mode_char);

	c = strrchr(argv[1], '\\'); // search for both
	c_alt = strrchr(argv[1], '/'); // just in case
	if (c < c_alt) // and take the last one we get
		c = c_alt;

	if (c == NULL)
		c = argv[1];
	else
		++c;

	memset(default_c, 0, 256);
	if (default_mode_char == 'e') // if the default was encoding, strip the .dec and call it a day
		strncpy(default_c, c, strlen(c) - 4);
	else
		snprintf(default_c, 255, "%s.%s", c, mode_char == 'd' ? "dec" : "enc");

	c = stdin_prompt_string("save as what?", default_c);
	if (access(c, F_OK) == 0) // confirm if file exists
	{
		while (confirm_char != 'y' && confirm_char != 'n')
			confirm_char = stdin_prompt_char("that file exists. overwrite? (y)es or (n)o", 'y');
		if (confirm_char == 'n')
			error("aborting");
	}
	if (!(fout = fopen(c, "wb")))
		error("error: can't open output file (no permission?)");

	run_track_thread(mode_char == 'd' ? _decode : _encode);

	fclose(fin);
	fclose(fout);
	return 0;
}
