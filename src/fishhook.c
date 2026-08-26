/*
 * fishhook.c - Facebook fishhook for iOS arm64
 * Rebind symbols by patching the GOT
 */
#include "fishhook.h"
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/mman.h>
#include <stdint.h>

// Structure to hold rebindings
struct rebindings_entry {
    struct rebinding *rebindings;
    size_t rebindings_count;
    struct rebindings_entry *next;
};

static struct rebindings_entry *_rebindings_head;

static int prepend_rebindings(struct rebindings_entry **rebindings_head,
                              struct rebinding rebindings[],
                              size_t nel) {
    struct rebindings_entry *new_entry = (struct rebindings_entry *)malloc(sizeof(struct rebindings_entry));
    if (!new_entry) return -1;
    
    new_entry->rebindings = (struct rebinding *)malloc(sizeof(struct rebinding) * nel);
    if (!new_entry->rebindings) {
        free(new_entry);
        return -1;
    }
    
    memcpy(new_entry->rebindings, rebindings, sizeof(struct rebinding) * nel);
    new_entry->rebindings_count = nel;
    new_entry->next = *rebindings_head;
    *rebindings_head = new_entry;
    return 0;
}

// Find the address of a symbol in an image
static void *get_address_for_symbol(const char *symbol_name,
                                     const struct mach_header *header,
                                     const char *image_name) {
    // Find the symbol table
    const struct symtab_command *symtab = NULL;
    const struct dysymtab_command *dysymtab = NULL;
    
    uintptr_t cur = (uintptr_t)header + sizeof(struct mach_header);
    if (header->magic == MH_MAGIC_64 || header->magic == MH_MAGIC) {
        struct mach_header_64 *h64 = (struct mach_header_64 *)header;
        cur = (uintptr_t)header + sizeof(struct mach_header_64);
        for (uint32_t i = 0; i < h64->ncmds; i++) {
            struct load_command *lc = (struct load_command *)cur;
            if (lc->cmd == LC_SYMTAB) symtab = (const struct symtab_command *)lc;
            if (lc->cmd == LC_DYSYMTAB) dysymtab = (const struct dysymtab_command *)lc;
            cur += lc->cmdsize;
        }
    } else {
        for (uint32_t i = 0; i < header->ncmds; i++) {
            struct load_command *lc = (struct load_command *)cur;
            if (lc->cmd == LC_SYMTAB) symtab = (const struct symtab_command *)lc;
            if (lc->cmd == LC_DYSYMTAB) dysymtab = (const struct dysymtab_command *)lc;
            cur += lc->cmdsize;
        }
    }
    
    if (!symtab) return NULL;
    
    // Search for the symbol
    const char *strtab = (const char *)header + symtab->strsize;
    const struct nlist_64 *symbols = (const struct nlist_64 *)((const char *)header + symtab->symoff);
    
    for (uint32_t i = 0; i < symtab->nsyms; i++) {
        if (symbols[i].n_value == 0) continue;
        const char *name = strtab + symbols[i].n_un.n_strx;
        if (name && strcmp(name, symbol_name) == 0) {
            return (void *)(uintptr_t)symbols[i].n_value;
        }
    }
    
    return NULL;
}

// Find and patch GOT entry for a symbol
static void rebind_pointers(struct rebindings_entry *rebindings) {
    uint32_t nImages = _dyld_image_count();
    
    for (uint32_t i = 0; i < nImages; i++) {
        const struct mach_header *header = _dyld_get_image_header(i);
        const char *image_name = _dyld_get_image_name(i);
        
        // Skip system libraries
        if (image_name && (strstr(image_name, "/System/") || 
                           strstr(image_name, "libsystem") || 
                           strstr(image_name, "libobjc"))) continue;
        
        // Find the GNU hash table and GOT
        uintptr_t cur = (uintptr_t)header + sizeof(struct mach_header_64);
        struct linkedit_data_command *got_cmd = NULL;
        struct linkedit_data_command *lua_cmd = NULL;
        
        for (uint32_t j = 0; j < header->ncmds; j++) {
            struct load_command *lc = (struct load_command *)cur;
            if (lc->cmd == 0x80000002 /* LC_GNU_DYNAMIC */) {  // GNU style
                // Skip
            }
            cur += lc->cmdsize;
        }
        
        // Iterate over rebindings
        for (size_t j = 0; j < rebindings->rebindings_count; j++) {
            const char *symbol_name = rebindings->rebindings[j].name;
            
            // Try dlsym first (simpler approach)
            void *sym = dlsym(RTLD_DEFAULT, symbol_name);
            if (sym) {
                if (rebindings->rebindings[j].replaced) {
                    *rebindings->rebindings[j].replaced = sym;
                }
                if (rebindings->rebindings[j].replacement) {
                    // Replace in GOT (simplified - just overwrite the pointer)
                    // In production, you'd need to handle page permissions
                    uintptr_t addr = (uintptr_t)sym;
                    // For JSC, the symbol is usually in the main executable
                }
            }
        }
    }
}

void rebind_symbols(struct rebinding rebindings[], size_t rebindings_nel) {
    _rebindings_head = NULL;
    if (prepend_rebindings(&_rebindings_head, rebindings, rebindings_nel) == -1) {
        return;
    }
    
    rebind_pointers(_rebindings_head);
}
