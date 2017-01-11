/*!
 * @file thinfat_wrap_posix.h
 * @brief thinFAT compatibility interface
 * @date 2017/01/11
 * @author Hiroka IHARA
 */
#ifndef THINFAT_WRAP_POSIX
#define THINFAT_WRAP_POSIX

#ifdef __cplusplus
extern "C" {
#endif

FILE *fopen(const char * restrict filename, const char * restrict mode);
int fflush(FILE *stream);
int fclose(FILE *stream);

size_t fread(void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream);
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream);

int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);

#ifdef __cplusplus
}
#endif

#endif
