#pragma once

char* sq_getstring(void* sqvm, int i);
int sq_getinteger(void* sqvm, int i);
void sq_pushbool(void* sqvm, int val);
void sq_pushstring(void* sqvm, char* string);