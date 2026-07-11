#ifndef UID_GENERATOR_H
#define UID_GENERATOR_H

#include <Arduino.h>

void generate_uid(const String& binding_phrase, uint8_t* uid_out);

#endif // UID_GENERATOR_H
