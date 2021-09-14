#pragma once

char* sq_getstring(void* sqvm, int i);
int sq_getinteger(void* sqvm, int i);

void sq_pushbool(void* sqvm, int val);
void sq_pushstring(void* sqvm, char* string, int len);
void sq_pushinteger(void* sqvm, int val);

void sq_newarray(void* sqvm, int size);
void sq_arrayappend(void* sqvm, int idx);

void sq_newtable(void* sqvm);
void sq_newslot(void* sqvm, int idx);