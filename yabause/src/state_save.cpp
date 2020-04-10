
#include <assert.h>

#include <string>
using std::string;

#include <iostream>
#include <fstream>
#include <cstdio>

#include <zlib.h>
#define CHUNK 16384

extern "C"{
#include "yabause.h"
#include "memory.h"
void ScspLockThread();
void ScspUnLockThread();
}

extern "C" int YabSaveCompressedState(const char *filename)
{
  FILE *fp;
  int status = 0;
  int ret, flush;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];

  string tmptilename;
  tmptilename = string(filename) + ".tmp";

  if( YabSaveState(tmptilename.c_str()) != 0 ){
    return -1;
  }
  
  FILE * source = fopen(tmptilename.c_str(), "rb");
  if (source == NULL) return -1;

  FILE * dest = fopen(filename, "wb");
  if (dest == NULL) return -1;
  
  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, Z_BEST_COMPRESSION);
  if (ret != Z_OK)
    return -1;
  /* compress until end of file */
  do {
    strm.avail_in = fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)deflateEnd(&strm);
      return -1;
    }
    flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = in;
    /* run deflate() on input until output buffer not full, finish
    compression if all of source has been read in */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = deflate(&strm, flush);    /* no bad return value */
      assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        (void)deflateEnd(&strm);
        return -1;
      }
    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0);     /* all input will be used */
                                    /* done when last data in file processed */
  } while (flush != Z_FINISH);
  assert(ret == Z_STREAM_END);        /* stream will be complete */
                                      /* clean up and return */
  (void)deflateEnd(&strm);

  fclose(source);
  fclose(dest);
  std::remove(tmptilename.c_str());
  return status;
}


extern "C" int YabLoadCompressedState(const char *filename)
{
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];
  string tmptilename;
  tmptilename = string(filename) + ".tmp";

  FILE * source = fopen(filename, "rb");
  if (source == NULL) return -1;

  FILE * dest = fopen(tmptilename.c_str(), "wb");
  if (dest == NULL) return -1;

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit(&strm);
  if (ret != Z_OK)
    return -1;
  /* decompress until deflate stream ends or end of file */
  do {
    strm.avail_in = fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)inflateEnd(&strm);
      return -1;
    }
    if (strm.avail_in == 0)
      break;
    strm.next_in = in;

    /* run inflate() on input until output buffer not full */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = inflate(&strm, Z_NO_FLUSH);
      assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
      switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR;     /* and fall through */
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        (void)inflateEnd(&strm);
        return -1;
      }
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        (void)inflateEnd(&strm);
        return -1;
      }
    } while (strm.avail_out == 0);
    /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);

  fclose(source);
  fclose(dest);

  int rtn = YabLoadState(tmptilename.c_str());  
  std::remove(tmptilename.c_str());
  return rtn;

}
