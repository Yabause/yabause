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

#ifndef MINI18N_PV_DATA_H
#define MINI18N_PV_DATA_H

#include <string.h>

typedef struct _mini18n_data_t mini18n_data_t;

struct _mini18n_data_t {
	size_t (*len)(const void * data);
	void * (*dup)(const void * data);
	int    (*cmp)(const void * data1, const void * data2);
};

extern mini18n_data_t mini18n_str;
extern mini18n_data_t mini18n_wcs;

#endif
