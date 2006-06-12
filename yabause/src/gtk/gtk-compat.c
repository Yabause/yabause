#include "gtk-compat.h"

#if !GLIB_CHECK_VERSION(2, 8, 0)
gboolean g_file_set_contents(const gchar * filename, const gchar * contents, gssize len, GError ** error) {
	FILE * file = fopen(filename, "w");

	if (len == -1)
		fprintf(file, "%s", contents);
	else
		fwrite(contents, 1, len, file);

	fclose(file);
}
#endif
