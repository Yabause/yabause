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

#ifndef MINI18N_H
#define MINI18N_H

/** @file */

#define MINI18N_UTF16 1

#ifndef _
#define _(source) (mini18n(source))
#endif

#ifndef _16
#define _16(source) (mini18n_with_conversion(source, MINI18N_UTF16))
#endif

/**
 * @brief Select a translation matching the operating system configuration.
 *
 * @param folder the folder to search for translations.
 * @returns 0 on success, -1 otherwise
 */
int mini18n_set_domain(const char * folder);
/**
 * @brief Load a translation from a file.
 *
 * @param filename of the translation to load.
 * @returns 0 on success, -1 otherwise
 */
int mini18n_set_locale(const char * locale);
int mini18n_set_log(const char * filename);
/**
 * @brief Translates a string.
 *
 * @param source string to translate
 * @returns The translated string on success, the source string otherwise. The returned value should not be freed or modified in any way.
 */
const char * mini18n(const char * source);
/**
 * @brief Translates and convert a string.
 *
 * The list of available conversion formats depends of the system.
 * The converted value is stored so further calls to the function with the same source will return the same pointer.
 *
 * @param source String to translate.
 * @param format The format to convert the string to.
 */
void * mini18n_with_conversion(const char * source, unsigned int format);
void mini18n_close(void);

/**
 *
 * @mainpage
 *
 * Mini18n is a translation library.
 *
 * \section using Using
 *
 * \subsection using-selecting Selecting translation
 *
 * Mini18n supports either automaticaly choosing a translation based on the current system settings or
 * load a given translation file.
 *
 * To let Mini18n select the translation file, call mini18n_set_domain() with the base directory
 * of translation files.
 *
 * \section extending Extending
 *
 */

#endif
