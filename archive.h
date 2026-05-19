#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <sys/types.h>

/* --- Veri Yapilari --- */
struct FileInfo {
    char filename[256];
    mode_t permissions;
    off_t size;
};

/* --- Arşivleme Fonksiyon Prototipleri --- */
void create_archive(const char *output_filename, char **input_files, int file_count);

#endif // ARCHIVE_H