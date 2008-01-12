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

#include "mini18n_pv_hash.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int mini18n_hash_func(const char * key);

mini18n_hash_t * mini18n_hash_init() {
	mini18n_hash_t * hash;
	int i;

	hash = malloc(sizeof(mini18n_hash_t));
	if (hash == NULL) {
		return NULL;
	}

	for(i = 0;i < MINI18N_HASH_SIZE;i++) {
		hash->list[i] = mini18n_list_init();
	}

	return hash;
}

void mini18n_hash_free(mini18n_hash_t * hash) {
	int i;

	if (hash == NULL) {
		return;
	}

	for(i = 0;i < MINI18N_HASH_SIZE;i++) {
		mini18n_list_free(hash->list[i]);
	}

	free(hash);
}

void mini18n_hash_add(mini18n_hash_t * hash, const char * key, const char * value) {
	int h;

	h = mini18n_hash_func(key);

	hash->list[h] = mini18n_list_add(hash->list[h], key, value);
}

const char * mini18n_hash_value(mini18n_hash_t * hash, const char * key) {
	int h;

	if (hash == NULL) {
		return key;
	}

	h = mini18n_hash_func(key);

	return mini18n_list_value(hash->list[h], key);
}

int mini18n_hash_func(const char * key) {
	int i, s = 0;
	int n = strlen(key);

	for(i = 0;i < n;i++) {
		s+= key[i];
		s %= MINI18N_HASH_SIZE;
	}

	return s;
}

mini18n_hash_t * mini18n_hash_from_file(const char * filename) {
	char buffer[1024];
	char key[1024];
	char value[1024];
	mini18n_hash_t * hash;
	FILE * f;

	hash = mini18n_hash_init();

	f = fopen(filename, "r");
	if (f == NULL) {
		return NULL;
	}

	while (fgets(buffer, 1024, f)) {
		int i = 0, j = 0, done = 0, state = 0;
		char c;

		while(!done && (i < 1024)) {
			c = buffer[i];
			switch(state) {
				case 0:
					switch(c) {
						case '\\':
							/* escape character, we're now in state 1 */
							state = 1;
							break;
						case ':':
							/* separator, we're done */
							key[j] = '\0';
							j = 0;
							state = 2;
							break;
						default:
							/* we're still reading the key */
							key[j] = c;
							j++;
							break;
					}
					break;
				case 1:
					key[j] = c;
					j++;
					state = 0;
					break;
				case 2:
					switch(c) {
						case '\n':
							value[j] = '\0';
							done = 1;
							break;
						default:
							value[j] = c;
							j++;
							break;
					}
					break;
			}
			i++;
		}

		if (done) {
			mini18n_hash_add(hash, key, value);
		}
	}

	fclose(f);

	return hash;
}
