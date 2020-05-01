cmake_minimum_required(VERSION 3.1)

project(chdr C)

#--------------------------------------------------
# static libs
#--------------------------------------------------

# crypto
set(CRYPTO_SOURCES
  deps/crypto/md5.c
  deps/crypto/sha1.c)

add_library(crypto-static STATIC ${CRYPTO_SOURCES})
list(APPEND CHDR_INCLUDES deps/crypto)
list(APPEND CHDR_LIBS crypto-static)

# flac
set(FLAC_SOURCES
  deps/flac-1.3.2/src/libFLAC/bitmath.c
  deps/flac-1.3.2/src/libFLAC/bitreader.c
  #deps/flac-1.3.2/src/libFLAC/bitwriter.c
  deps/flac-1.3.2/src/libFLAC/cpu.c
  deps/flac-1.3.2/src/libFLAC/crc.c
  deps/flac-1.3.2/src/libFLAC/fixed.c
  deps/flac-1.3.2/src/libFLAC/fixed_intrin_sse2.c
  deps/flac-1.3.2/src/libFLAC/fixed_intrin_ssse3.c
  deps/flac-1.3.2/src/libFLAC/float.c
  deps/flac-1.3.2/src/libFLAC/format.c
  deps/flac-1.3.2/src/libFLAC/lpc.c
  deps/flac-1.3.2/src/libFLAC/lpc_intrin_avx2.c
  deps/flac-1.3.2/src/libFLAC/lpc_intrin_sse2.c
  deps/flac-1.3.2/src/libFLAC/lpc_intrin_sse41.c
  deps/flac-1.3.2/src/libFLAC/lpc_intrin_sse.c
  deps/flac-1.3.2/src/libFLAC/md5.c
  deps/flac-1.3.2/src/libFLAC/memory.c
  deps/flac-1.3.2/src/libFLAC/metadata_iterators.c
  deps/flac-1.3.2/src/libFLAC/metadata_object.c
  #deps/flac-1.3.2/src/libFLAC/ogg_decoder_aspect.c
  #deps/flac-1.3.2/src/libFLAC/ogg_encoder_aspect.c
  #deps/flac-1.3.2/src/libFLAC/ogg_helper.c
  #deps/flac-1.3.2/src/libFLAC/ogg_mapping.c
  deps/flac-1.3.2/src/libFLAC/stream_decoder.c
  #deps/flac-1.3.2/src/libFLAC/stream_encoder.c
  #deps/flac-1.3.2/src/libFLAC/stream_encoder_framing.c
  #deps/flac-1.3.2/src/libFLAC/stream_encoder_intrin_avx2.c
  #deps/flac-1.3.2/src/libFLAC/stream_encoder_intrin_sse2.c
  #deps/flac-1.3.2/src/libFLAC/stream_encoder_intrin_ssse3.c
  deps/flac-1.3.2/src/libFLAC/window.c)
if(WIN32)
  list(APPEND FLAC_SOURCES deps/flac-1.3.2/src/libFLAC/windows_unicode_filenames.c)
endif()

if(APPLE OR UNIX)
  list(APPEND FLAC_DEFS HAVE_SYS_PARAM_H)
endif()

add_library(flac-static STATIC ${FLAC_SOURCES})
target_include_directories(flac-static PUBLIC deps/flac-1.3.2/include deps/flac-1.3.2/src/libFLAC/include)
target_compile_definitions(flac-static PRIVATE PACKAGE_VERSION="1.3.2" FLAC__HAS_OGG=0 FLAC__NO_DLL HAVE_LROUND HAVE_STDINT_H HAVE_STDLIB_H ${FLAC_DEFS})
list(APPEND CHDR_INCLUDES deps/flac-1.3.2/src/libFLAC/include)
list(APPEND CHDR_DEFS FLAC__NO_DLL)
list(APPEND CHDR_LIBS flac-static)

# lzma
set(LZMA_SOURCES
  #deps/lzma-16.04/C/7zAlloc.c
  #deps/lzma-16.04/C/7zArcIn.c
  #deps/lzma-16.04/C/7zBuf2.c
  #deps/lzma-16.04/C/7zBuf.c
  #deps/lzma-16.04/C/7zCrc.c
  #deps/lzma-16.04/C/7zCrcOpt.c
  #deps/lzma-16.04/C/7zDec.c
  #deps/lzma-16.04/C/7zFile.c
  #deps/lzma-16.04/C/7zStream.c
  #deps/lzma-16.04/C/Aes.c
  #deps/lzma-16.04/C/AesOpt.c
  deps/lzma-16.04/C/Alloc.c
  #deps/lzma-16.04/C/Bcj2.c
  #deps/lzma-16.04/C/Bcj2Enc.c
  deps/lzma-16.04/C/Bra86.c
  deps/lzma-16.04/C/Bra.c
  deps/lzma-16.04/C/BraIA64.c
  deps/lzma-16.04/C/CpuArch.c
  deps/lzma-16.04/C/Delta.c
  #deps/lzma-16.04/C/DllSecur.c
  deps/lzma-16.04/C/LzFind.c
  #deps/lzma-16.04/C/LzFindMt.c
  #deps/lzma-16.04/C/Lzma2Dec.c
  #deps/lzma-16.04/C/Lzma2Enc.c
  deps/lzma-16.04/C/Lzma86Dec.c
  deps/lzma-16.04/C/Lzma86Enc.c
  deps/lzma-16.04/C/LzmaDec.c
  deps/lzma-16.04/C/LzmaEnc.c
  deps/lzma-16.04/C/LzmaLib.c
  #deps/lzma-16.04/C/MtCoder.c
  #deps/lzma-16.04/C/Ppmd7.c
  #deps/lzma-16.04/C/Ppmd7Dec.c
  #deps/lzma-16.04/C/Ppmd7Enc.c
  #deps/lzma-16.04/C/Sha256.c
  deps/lzma-16.04/C/Sort.c
  #deps/lzma-16.04/C/Threads.c
  #deps/lzma-16.04/C/Xz.c
  #deps/lzma-16.04/C/XzCrc64.c
  #deps/lzma-16.04/C/XzCrc64Opt.c
  #deps/lzma-16.04/C/XzDec.c
  #deps/lzma-16.04/C/XzEnc.c
  #deps/lzma-16.04/C/XzIn.c
				)

add_library(lzma-static STATIC ${LZMA_SOURCES})
target_compile_options(lzma-static PRIVATE -D_7ZIP_ST)
list(APPEND CHDR_INCLUDES deps/lzma-16.04/C)
list(APPEND CHDR_LIBS lzma-static)

# zlib
add_subdirectory(deps/zlib-1.2.11 EXCLUDE_FROM_ALL)
list(APPEND CHDR_INCLUDES deps/zlib-1.2.11 ${CMAKE_CURRENT_BINARY_DIR}/deps/zlib-1.2.11)
list(APPEND CHDR_LIBS zlibstatic)

#--------------------------------------------------
# chdr
#--------------------------------------------------

set(CHDR_SOURCES
  src/bitstream.c
  src/bitstream.h
  src/cdrom.c
  src/cdrom.h
  src/chd.c
  src/chd.h
  src/coretypes.h
  src/flac.c
  src/flac.h
  src/huffman.c
  src/huffman.h)

add_library(chdr-static STATIC ${CHDR_SOURCES})
target_include_directories(chdr-static PRIVATE ${CHDR_INCLUDES})
target_compile_definitions(chdr-static PRIVATE ${CHDR_DEFS})
target_link_libraries(chdr-static ${CHDR_LIBS})
