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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static mini18n_hash_t * hash = NULL;
#ifdef MINI18N_LOG
static FILE * log = NULL;
#endif

int mini18n_set_locale(const char * locale) {
	mini18n_hash_free(hash);

	hash = mini18n_hash_from_file(locale);
	if (hash == NULL) {
		return -1;
	}

	return 0;
}

int mini18n_set_log(const char * filename) {
#ifdef MINI18N_LOG
	log = fopen(filename, "w");

	if (log == NULL) {
		return -1;
	}
#endif

	return 0;
}

const char * mini18n(const char * source) {
#ifdef MINI18N_LOG
	const char * translated = mini18n_hash_value(hash, source);

	if ((log) && (translated == source)) {
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
	}

	return translated;
#else
	return mini18n_hash_value(hash, source);
#endif
}

void mini18n_close(void) {
	mini18n_hash_free(hash);
#ifdef MINI18N_LOG
	if (log) fclose(log);
#endif
}
