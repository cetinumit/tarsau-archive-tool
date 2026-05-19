#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "archive.h"

#define MAX_TOTAL_SIZE (200 * 1024 * 1024) // 200 MB Limiti

/* --- Metin Dosyasi Kontrolu (Sadece ASCII) --- */
int is_text_file(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        return 0; // Dosya acilamazsa uyumsuz kabul edelim
    }

    int ch;
    while ((ch = fgetc(file)) != EOF) {
        // Karakter basina 1 bayt (ASCII araligi 0-127).
        // 32 altindaki bazi kontrol karakterleri (\n, \r, \t) metin dosyalarinda gecerlidir, digerlerini reddediyoruz.
        if (ch > 127 || (ch < 32 && ch != '\n' && ch != '\r' && ch != '\t')) {
            fclose(file);
            return 0; // Metin formati degil (binary veya ozel karakter iceriyor)
        }
    }
    fclose(file);
    return 1;
}

/* --- Arsiv Olusturma Ana Fonksiyonu --- */
void create_archive(const char *output_filename, char **input_files, int file_count) {
    struct FileInfo files[32];
    struct stat st;
    long long total_size = 0;
    
    // Organizasyon bolumu icin buffer.
    // Format geregi en basa bir '|' koyarak basliyoruz.
    char org_section[8192] = "|"; 

    // 1. Dosya Kontrolleri ve Metadata Toplanmasi
    for (int i = 0; i < file_count; i++) {
        // Uyumsuzluk kontrolu
        if (!is_text_file(input_files[i])) {
            // İstenilen tam hata formati ve sorunsuz sonlanma (exit SUCCESS)
            printf("%s giris dosyasinin formati uyumsuzdur!\n", input_files[i]);
            exit(EXIT_SUCCESS); 
        }

        // İzinler ve boyut icin stat() kullanimi
        if (stat(input_files[i], &st) == -1) {
            printf("Hata: %s dosyasinin bilgileri alinamadi.\n", input_files[i]);
            exit(EXIT_FAILURE);
        }

        strcpy(files[i].filename, input_files[i]);
        files[i].permissions = st.st_mode & 0777; // Izinleri al (ornek: 0644)
        files[i].size = st.st_size;

        total_size += st.st_size;
        if (total_size > MAX_TOTAL_SIZE) {
            printf("Hata: Toplam giris dosyasi boyutu 200 MB limitini asiyor!\n");
            exit(EXIT_FAILURE);
        }

        // Organizasyon bolumune formatli ekleme: Dosya adi, izinler(octal), boyut|
        char record[256];
        snprintf(record, sizeof(record), "%s,%04o,%ld|", files[i].filename, files[i].permissions, (long)files[i].size);
        strcat(org_section, record);
    }

    // 2. Arsiv Dosyasini Olusturma
    FILE *out_file = fopen(output_filename, "wb"); 
    if (!out_file) {
        printf("Hata: %s arsiv dosyasi olusturulamadi.\n", output_filename);
        exit(EXIT_FAILURE);
    }

    // 3. Organizasyon Bolumu Boyutunu (10 bayt) Yazma
    int org_length = strlen(org_section);
    fprintf(out_file, "%010d", org_length);

    // 4. Organizasyon Icerigini Yazma
    fprintf(out_file, "%s", org_section);

    // 5. Dosya Iceriklerini Ayirici Olmadan Art Arda Yazma
    for (int i = 0; i < file_count; i++) {
        FILE *in_file = fopen(input_files[i], "rb");
        if (in_file) {
            char buffer[4096];
            size_t bytes_read;
            while ((bytes_read = fread(buffer, 1, sizeof(buffer), in_file)) > 0) {
                fwrite(buffer, 1, bytes_read, out_file);
            }
            fclose(in_file);
        }
    }

    fclose(out_file);
    printf("Basarili: %s arsiv dosyasi olusturuldu.\n", output_filename);
}