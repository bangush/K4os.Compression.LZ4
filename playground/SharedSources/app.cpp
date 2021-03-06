// ReSharper disable CppClangTidyBugproneNarrowingConversions
// ReSharper disable CppCStyleCast
#define _CRT_SECURE_NO_WARNINGS
#pragma warning( disable : 4996 6387 )

#include "lz4.h"
#include "lz4hc.h"

#include <cstdlib>
#include <cstdint>
#include <cstdio>

#include "lz4.c"
#include "lz4hc.c"

typedef unsigned char byte;
typedef uint32_t uint;

int min(int a, int b) { return a < b ? a : b; }

uint32_t adler32(const byte* data, int len)
{
	const uint32_t MOD_ADLER = 65521;
	uint32_t a = 1, b = 0;

	// Process each byte of the data in order
	for (int index = 0; index < len; ++index)
	{
		a = (a + data[index]) % MOD_ADLER;
		b = (b + a) % MOD_ADLER;
	}

	return (b << 16) | a;
}

static const unsigned char base64_table[65] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64_length(int src_len) {
	return 4 * ((src_len + 2) / 3);
}

int base64_encode(const byte* src, int src_len, char* dst, int dst_len)
{
	const int olen = base64_length(src_len);
	if (olen + 1 > dst_len)
		return -1;

	const int len = src_len;
	const unsigned char* in = (unsigned char*)src;
	const unsigned char* end = in + len;
	char* pos = dst;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		}
		else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}

	*pos = 0;

	return olen + 1;
}

void checksumLC(const char* filename, int index, int length) {
	char full_path[1024];
	strcpy(full_path, "../../");
	strcat(full_path, filename);

	FILE* f = fopen(full_path, "rb");
	fseek(f, 0, SEEK_END);
	const int size = ftell(f);
	fseek(f, index, SEEK_SET);

	const int src_len = length < 0 ? size - index : length;
	byte* src = (byte*)malloc(src_len);
	fread(src, 1, src_len, f);
	fclose(f);

	const int dst_len = LZ4_compressBound(src_len);
	byte* dst = (byte*)malloc(dst_len);

	const int cmp_len = LZ4_compress_fast((char*)src, (char*)dst, src_len, dst_len, 1);
	const uint adler = adler32(dst, cmp_len);

	char base64[81];
	base64_encode(dst, min(cmp_len, 60), base64, 81);

	printf(
		"[InlineData(%zd, \"%s\", %d, %d, %d, %d, 0x%x, \"%s\")]\n",
		sizeof(void*), filename, index, src_len, 0, cmp_len, adler, base64);

	free(src);
	free(dst);
}

void checksumHC(const char* filename, int index, int length, int level) {
	char full_path[1024];
	strcpy(full_path, "../../");
	strcat(full_path, filename);

	FILE* f = fopen(full_path, "rb");
	fseek(f, 0, SEEK_END);
	const int size = ftell(f);
	fseek(f, index, SEEK_SET);

	const int src_len = length < 0 ? size - index : length;
	byte* src = (byte*)malloc(src_len);
	fread(src, 1, src_len, f);
	fclose(f);

	const int dst_len = LZ4_compressBound(src_len);
	byte* dst = (byte*)malloc(dst_len);

	const int cmp_len = LZ4_compress_HC((char*)src, (char*)dst, src_len, dst_len, level);
	const uint adler = adler32(dst, cmp_len);

	char base64[81];
	base64_encode(dst, min(cmp_len, 60), base64, 81);

	printf(
		"[InlineData(%zd, \"%s\", %d, %d, %d, %d, 0x%x, \"%s\")]\n",
		sizeof(void*), filename, index, src_len, level, cmp_len, adler, base64);

	free(src);
	free(dst);
}


int main()
{
	printf("\n\n\n>>> LC\n");

	checksumLC(".corpus/dickens", 0, -1);
	checksumLC(".corpus/mozilla", 0, -1);
	checksumLC(".corpus/mr", 0, -1);
	checksumLC(".corpus/nci", 0, -1);
	checksumLC(".corpus/ooffice", 0, -1);
	checksumLC(".corpus/osdb", 0, -1);
	checksumLC(".corpus/reymont", 0, -1);
	checksumLC(".corpus/samba", 0, -1);
	checksumLC(".corpus/sao", 0, -1);
	checksumLC(".corpus/webster", 0, -1);
	checksumLC(".corpus/xml", 0, -1);
	checksumLC(".corpus/x-ray", 0, -1);

	printf("\n\n\n>>> HC\n");
	static int levels[] = { 3, 9, 10, 12, -1 };
	for (auto i = 0; levels[i] >= 0; i++)
	{
		const auto level = levels[i];
		checksumHC(".corpus/dickens", 0, -1, level);
		checksumHC(".corpus/mozilla", 0, -1, level);
		checksumHC(".corpus/mr", 0, -1, level);
		checksumHC(".corpus/nci", 0, -1, level);
		checksumHC(".corpus/ooffice", 0, -1, level);
		checksumHC(".corpus/osdb", 0, -1, level);
		checksumHC(".corpus/reymont", 0, -1, level);
		checksumHC(".corpus/samba", 0, -1, level);
		checksumHC(".corpus/sao", 0, -1, level);
		checksumHC(".corpus/webster", 0, -1, level);
		checksumHC(".corpus/xml", 0, -1, level);
		checksumHC(".corpus/x-ray", 0, -1, level);
	}

	printf("\n\n\n>>> DONE\n");
	getchar();
}
