/*
 * fishhook.h - Header for fishhook symbol rebinding
 */
#ifndef fishhook_h
#define fishhook_h

struct rebinding {
    const char *name;
    void *replacement;
    void **replaced;
};

void rebind_symbols(struct rebinding rebindings[], size_t rebindings_nel);

#endif /* fishhook_h */
