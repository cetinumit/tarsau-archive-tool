#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Kendi baslik dosyalarimiz */
#include "archive.h"
#include "extract.h"

int main(int argc, char *argv[]) {
    // Minimum argüman sayısı kontrolü
    if (argc < 3) {
        fprintf(stderr, "Kullanim Hatalari:\n");
        fprintf(stderr, "  Arsivleme: %s -b dosya1 dosya2 ... [-o arsiv.sau]\n", argv[0]);
        fprintf(stderr, "  Cikarma:   %s -a arsiv.sau [hedef_dizin]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "-b") == 0) {
        /* --- Arsivleme İslemi Parametreleri (-b) --- */
        const char *output_filename = "a.sau"; 
        char *input_files[32];                 
        int file_count = 0;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-o") == 0) {
                if (i + 1 < argc) {
                    output_filename = argv[i + 1];
                    i++; 
                } else {
                    fprintf(stderr, "Hata: -o parametresinden sonra dosya adi belirtilmedi.\n");
                    return EXIT_FAILURE;
                }
            } else {
                if (file_count < 32) {
                    input_files[file_count++] = argv[i];
                } else {
                    fprintf(stderr, "Hata: Toplam giris dosyasi sayisi en fazla 32 olabilir.\n");
                    return EXIT_FAILURE;
                }
            }
        }

        if (file_count == 0) {
            fprintf(stderr, "Hata: Arsivlenecek giris dosyasi belirtilmedi.\n");
            return EXIT_FAILURE;
        }

        // Yonlendirme
        create_archive(output_filename, input_files, file_count);

    } else if (strcmp(argv[1], "-a") == 0) {
        /* --- Arsivi Acma İslemi Parametreleri (-a) --- */
        const char *archive_filename = argv[2];
        const char *target_dir = "."; 

        if (argc >= 4) {
            target_dir = argv[3];
        }
        
        if (argc > 4) {
            fprintf(stderr, "Hata: -a parametresi en fazla 2 arguman alabilir.\n");
            return EXIT_FAILURE;
        }

        // Yonlendirme
        extract_archive(archive_filename, target_dir);

    } else {
        fprintf(stderr, "Hata: Gecersiz islem parametresi. Lutfen -b veya -a kullanin.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}