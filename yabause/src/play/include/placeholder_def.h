#ifndef _PLACEHOLDER_DEF_H_
#define _PLACEHOLDER_DEF_H_

#ifdef _MSC_VER

#define PLACEHOLDER_1   std::tr1::placeholders::_1
#define PLACEHOLDER_2   std::tr1::placeholders::_2
#define PLACEHOLDER_3   std::tr1::placeholders::_3
#define PLACEHOLDER_4   std::tr1::placeholders::_4
#define PLACEHOLDER_5   std::tr1::placeholders::_5
#define PLACEHOLDER_6   std::tr1::placeholders::_6

#else

#define PLACEHOLDER_1   _1
#define PLACEHOLDER_2   _2
#define PLACEHOLDER_3   _3
#define PLACEHOLDER_4   _4
#define PLACEHOLDER_5   _5
#define PLACEHOLDER_6   _6

#endif

#endif
