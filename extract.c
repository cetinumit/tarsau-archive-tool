#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "extract.h"
#include "archive.h" // struct FileInfo tanımını kullanabilmek için

/* --- Yardımcı Fonksiyon: Dizin Oluşturma --- */
void create_directory_if_missing(const char *dir_path) {
    struct stat st = {0};
    // Dizin mevcut değilse oluştur (0777 izniyle, işletim sistemi umask'e göre ayarlar)
    if (stat(dir_path, &st) == -1) {
        mkdir(dir_path, 0777);
    }
}

/* --- Arşivi Çıkarma Ana Fonksiyonu --- */
void extract_archive(const char *archive_filename, const char *target_dir) {
    FILE *archive_file = fopen(archive_filename, "rb");
    if (!archive_file) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        exit(EXIT_SUCCESS);
    }

    // 1. Organizasyon Bölümü Boyutunu (10 bayt) Okuma
    char header_len_str[11] = {0};
    if (fread(header_len_str, 1, 10, archive_file) != 10) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(archive_file);
        exit(EXIT_SUCCESS);
    }

    int org_length = atoi(header_len_str);
    if (org_length <= 0) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        fclose(archive_file);
        exit(EXIT_SUCCESS);
    }

    // 2. Organizasyon İçeriğini Okuma
    char *org_section = (char *)malloc(org_length + 1);
    if (!org_section) {
        perror("Bellek tahsisi hatası");
        fclose(archive_file);
        exit(EXIT_FAILURE);
    }

    if (fread(org_section, 1, org_length, archive_file) != org_length) {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        free(org_section);
        fclose(archive_file);
        exit(EXIT_SUCCESS);
    }
    org_section[org_length] = '\0';

    // Format kontrolü: İlk karakter '|' olmalı
    if (org_section[0] != '|') {
        printf("Arşiv dosyası uygunsuz veya bozuk!\n");
        free(org_section);
        fclose(archive_file);
        exit(EXIT_SUCCESS);
    }

    // 3. Metadata Çözümleme (Parsing)
    struct FileInfo files[32];
    int file_count = 0;
    char *ptr = org_section + 1; // İlk '|' karakterini atla

    while (*ptr && file_count < 32) {
        char *next_pipe = strchr(ptr, '|');
        if (!next_pipe) break;
        
        *next_pipe = '\0'; // Parçalama (split) işlemi için '|' karakterini NULL ile değiştir

        // "Dosya adı,izinler,boyut" formatını oku
        char fname[256];
        unsigned int perm;
        long size;

        if (sscanf(ptr, "%[^,],%o,%ld", fname, &perm, &size) == 3) {
            strcpy(files[file_count].filename, fname);
            files[file_count].permissions = perm;
            files[file_count].size = size;
            file_count++;
        } else {
            printf("Arşiv dosyası uygunsuz veya bozuk!\n");
            free(org_section);
            fclose(archive_file);
            exit(EXIT_SUCCESS);
        }
        
        ptr = next_pipe + 1;
    }
    free(org_section);

    // 4. Hedef Dizini Ayarlama
    // Eğer "." (geçerli dizin) değilse, dizini oluştur
    if (strcmp(target_dir, ".") != 0) {
        create_directory_if_missing(target_dir);
    }

    // 5. Dosyaları Çıkarma ve İzinleri Geri Yükleme
    for (int i = 0; i < file_count; i++) {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", target_dir, files[i].filename);

        FILE *out_file = fopen(full_path, "wb");
        if (!out_file) {
            printf("Hata: %s dosyasi olusturulamadi.\n", full_path);
            continue; // Diğer dosyaları çıkarmaya devam et
        }

        long remaining = files[i].size;
        char buffer[4096];

        while (remaining > 0) {
            size_t to_read = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);
            size_t bytes_read = fread(buffer, 1, to_read, archive_file);
            
            if (bytes_read == 0) {
                // Beklenmeyen dosya sonu (Arşiv bozuk olabilir)
                printf("Arşiv dosyası uygunsuz veya bozuk!\n");
                fclose(out_file);
                fclose(archive_file);
                exit(EXIT_SUCCESS);
            }
            
            fwrite(buffer, 1, bytes_read, out_file);
            remaining -= bytes_read;
        }

        fclose(out_file);

        // Orijinal izinleri geri yükle (chmod)
        if (chmod(full_path, files[i].permissions) != 0) {
            perror("İzinler geri yüklenirken hata oluştu");
        }
    }

    fclose(archive_file);
    printf("Basarili: Arsiv basariyla %s/ dizinine cikarildi.\n", target_dir);
}