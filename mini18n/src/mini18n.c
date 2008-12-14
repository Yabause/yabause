/*  Copyright 2008 Guillaume Duhamel
  
    This file is part of mini18n.
  
    mini18n is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
  
    mini18n is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
  
    You should have received a copy of the GNU General Public License
    along with mini18n; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "mini18n.h"
#include "mini18n_pv_hash.h"
#include "mini18n_pv_conv.h"
#ifdef _WIN32
#include "mini18n_pv_conv_windows.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static mini18n_hash_t * hash[] = {
	NULL,
	NULL
};
static mini18n_conv_t * converters[] = {
#ifdef _WIN32
	&mini18n_conv_windows_utf16,
#endif
	NULL
};

#ifdef MINI18N_LOG
static FILE * log = NULL;
#endif

#ifdef _WIN32
#include <windows.h>
static char pathsep = '\\';

static void mini18n_pv_get_locale(char ** lang, char ** country) {
	char buffer[11];
	int i;

	*lang = malloc(3);
	*country = malloc(6);

	LCID locale = GetUserDefaultLCID();
	GetLocaleInfo(locale, LOCALE_SABBREVLANGNAME, buffer, 10);
	for(i = 0;i < 2;i++) {
		(*lang)[i] = tolower(buffer[i]);
		(*country)[i] = tolower(buffer[i]);
	}
	(*country)[i++] = '_';
	GetLocaleInfo(locale, LOCALE_SABBREVCTRYNAME, buffer, 10);
	for(;i < 5;i++) {
		(*country)[i] = toupper(buffer[i - 3]);
	}
	(*lang)[2] = '\0';
	(*country)[5] = '\0';
}
#else
static char pathsep = '/';

static void mini18n_pv_get_locale(char ** lang, char ** country) {
	char * tmp;

	*country = NULL;
	*lang = strdup(getenv("LANG"));
	if (*lang == NULL) return;

	tmp = strchr(*lang, '@');
	if (tmp != NULL) *tmp = '\0';
	tmp = strchr(*lang, '.');
	if (tmp != NULL) *tmp = '\0';
	tmp = strchr(*lang, '_');
	if (tmp != NULL) {
		*country = strdup(*lang);
		*tmp = '\0';
	}
}
#endif

int mini18n_set_domain(const char * folder) {
	char * lang;
	char * country;
	char * locale;
	char * fulllocale;

	mini18n_pv_get_locale(&lang, &country);

	if (folder == NULL) {
		locale = strdup(lang);
		fulllocale = strdup(country);
	} else {
		char * pos;
		size_t n = strlen(folder);

		if (n == 0) {
			locale = strdup(lang);
			fulllocale = strdup(country);
		} else {
			size_t s;
			int trailing = folder[n - 1] == pathsep ? 1 : 0;

			s = n + strlen(lang) + 5 + (1 - trailing);
			locale = malloc(s);

			pos = locale;
			pos += sprintf(pos, "%s", folder);
			if (! trailing) pos += sprintf(pos, "%c", pathsep);
			sprintf(pos, "%s.yts", lang);

			s = n + strlen(country) + 5 + (1 - trailing);
			fulllocale = malloc(s);

			pos = fulllocale;
			pos += sprintf(pos, "%s", folder);
			if (! trailing) pos += sprintf(pos, "%c", pathsep);
			sprintf(pos, "%s.yts", country);
		}
	}

	if (mini18n_set_locale(fulllocale) == -1) {
		return mini18n_set_locale(locale);
	}

	return 0;
}

int mini18n_set_locale(const char * locale) {
	mini18n_hash_t * tmp = mini18n_hash_from_file(locale);

	if (tmp == NULL) return -1;

	mini18n_hash_free(hash[0]);
	if (hash[1] != NULL) mini18n_hash_free(hash[1]);

	hash[0] = tmp;
	hash[1] = NULL;

	return 0;
}

int mini18n_set_log(const char * filename) {
#ifdef MINI18N_LOG
	log = fopen(filename, "a");

	if (log == NULL) {
		return -1;
	}
#endif

	return 0;
}

const char * mini18n(const char * source) {
	const char * translated = mini18n_hash_value(hash[0], source);

#ifdef MINI18N_LOG
	if ((log) && (hash[0]) && (translated == source)) {
		unsigned int i = 0;
		unsigned int n = strlen(source);

		for(i = 0;i < n;i++) {
			switch(source[i]) {
				case '|':
					fprintf(log, "\\|");
					break;
				case '\\':
					fprintf(log, "\\\\");
					break;
				default:
					fprintf(log, "%c", source[i]);
					break;
			}
		}
		if ( n > 0 )
			fprintf(log, "|\n");

		/* we add the non translated string to avoid duplicates in the log file */
		mini18n_hash_add(hash[0], source, translated);
	}
#endif

	return translated;
}

void * mini18n_with_conversion(const char * source, unsigned int format) {
	mini18n_conv_t * converter;
	const void * conv;

	if (hash[format] != NULL) {
		conv = mini18n_hash_value(hash[format], source);
		if (conv != source) return conv;
	}

	converter = *converters;

	while(converter != NULL) {
		if (converter->format == format) {
			conv = converter->conv(mini18n(source));
			if (hash[format] == NULL) hash[format] = mini18n_hash_init(converter->data);
			mini18n_hash_add(hash[format], source, conv);
			return conv;
		}
		converter++;
	}

	return "";
}

void mini18n_close(void) {
	mini18n_hash_free(hash[0]);
	if (hash[1] != NULL) mini18n_hash_free(hash[1]);
#ifdef MINI18N_LOG
	if (log) fclose(log);
#endif
}
