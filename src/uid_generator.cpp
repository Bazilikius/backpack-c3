#include "uid_generator.h"
#include <MD5Builder.h>

void generate_uid(const String& binding_phrase, uint8_t* uid_out) {
    if (binding_phrase.length() == 0) {
        // Default UID of 0s or some fallback
        for (int i = 0; i < 6; i++) {
            uid_out[i] = 0;
        }
        return;
    }

    MD5Builder md5;
    md5.begin();
    md5.add(binding_phrase);
    md5.calculate();

    uint8_t digest[16];
    md5.getBytes(digest);

    // The first 6 bytes of the MD5 hash are stored as the shared UID
    for (int i = 0; i < 6; i++) {
        uid_out[i] = digest[i];
    }
}
