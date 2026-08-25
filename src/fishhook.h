// fishhook.h - Header for fishhook library
// Copyright (c) 2013, Facebook, Inc.
// All rights reserved.

#ifndef fishhook_h
#define fishhook_h

#include <stddef.h>
#include <stdbool.h>

struct rebinding {
  const char* name;
  void* replacement;
  void** rebind;
};

int rebind_symbols(struct rebinding rebindings[], size_t rebindings_nel);
int rebind_symbols_image(void* header, intptr_t slide, struct rebinding rebindings[], size_t rebindings_nel);

#endif /* fishhook_h */
