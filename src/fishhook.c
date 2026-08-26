// fishhook.c - Facebook fishhook for iOS arm64
// 通过修改 GOT 实现符号重绑定

#include "fishhook.h"
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <sys/mman.h>

#ifdef __LP64__
typedef struct mach_header_64 mach_header_t;
typedef struct segment_command_64 segment_command_t;
typedef struct section_64 section_t;
typedef struct nlist_64 nlist_t;
#else
typedef struct mach_header mach_header_t;
typedef struct segment_command segment_command_t;
typedef struct section section_t;
typedef struct nlist nlist_t;
#endif

struct rebindings_entry {
    struct rebinding *rebindings;
    size_t rebindings_count;
    struct rebindings_entry *next;
};

static struct rebindings_entry *_rebindings_head;

// 对单个 image 应用 rebindings
static void apply_rebindings_to_image(const mach_header_t *header,
                                       intptr_t slide,
                                       struct rebinding rebindings[],
                                       size_t count) {
    if (!header) return;
    
    const segment_command_t *seg_cmd = NULL;
    section_t *sections = NULL;
    uint32_t sections_count = 0;
    
    // 找到 __la_symbol_ptr section
    #ifdef __LP64__
    const struct segment_command_64 *seg64 = NULL;
    const struct section_64 *secs64 = NULL;
    for (uint32_t i = 0; i < header->nfat_cmds; i++) {
        seg64 = (const struct segment_command_64 *)((const char *)header + sizeof(mach_header_t) + i * sizeof(struct load_command));
        if (seg64->cmd == LC_SEGMENT_64) {
            secs64 = (const struct section_64 *)((const char *)seg64 + sizeof(struct segment_command_64));
            for (uint32_t j = 0; j < seg64->nsects; j++) {
                if (strcmp(secs64[j].segname, "__DATA") == 0 &&
                    strcmp(secs64[j].sectname, "__la_symbol_ptr") == 0) {
                    sections = (section_t *)&secs64[j];
                    sections_count = 1;
                    goto found;
                }
            }
        }
    }
    #else
    const struct segment_command *seg = NULL;
    const struct section *secs = NULL;
    for (uint32_t i = 0; i < header->nfat_cmds; i++) {
        seg = (const struct segment_command *)((const char *)header + sizeof(mach_header_t) + i * sizeof(struct load_command));
        if (seg->cmd == LC_SEGMENT) {
            secs = (const struct section *)((const char *)seg + sizeof(struct segment_command));
            for (uint32_t j = 0; j < seg->nsects; j++) {
                if (strcmp(secs[j].segname, "__DATA") == 0 &&
                    strcmp(secs[j].sectname, "__la_symbol_ptr") == 0) {
                    sections = &secs[j];
                    sections_count = 1;
                    goto found;
                }
            }
        }
    }
    #endif
    
    found:
    if (!sections) return;
    
    // 遍历 GOT 条目
    uintptr_t got_base = (uintptr_t)header + slide + sections->addr;
    uintptr_t got_end = got_base + sections->size;
    
    for (uintptr_t got_addr = got_base; got_addr < got_end; got_addr += sizeof(void *)) {
        void **func_ptr = (void **)got_addr;
        void *original = *func_ptr;
        
        for (size_t i = 0; i < count; i++) {
            struct rebinding *rebinding = &rebindings[i];
            
            // 检查函数名是否匹配
            const char *sym_name = rebinding->name;
            
            // 通过 dlsym 检查符号地址是否匹配
            void *sym_ptr = dlsym(RTLD_DEFAULT, sym_name);
            if (sym_ptr == original) {
                // 获取当前页面的保护属性
                mach_port_t task = mach_task_self();
                vm_address_t addr = got_addr;
                vm_size_t size = 1;
                vm_prot_t cur_prot, max_prot;
                
                if (vm_protect(task, addr, size, FALSE, VM_PROT_READ | VM_PROT_WRITE) == KERN_SUCCESS) {
                    *func_ptr = rebinding->replacement;
                    *rebinding->rebind = original;
                    
                    // 恢复保护属性
                    vm_protect(task, addr, size, FALSE, cur_prot);
                }
            }
        }
    }
}

int rebind_symbols(struct rebinding rebindings[], size_t rebindings_nel) {
    return rebind_symbols_image(NULL, 0, rebindings, rebindings_nel);
}

int rebind_symbols_image(void *header, intptr_t slide,
                         struct rebinding rebindings[], size_t rebindings_nel) {
    if (header) {
        apply_rebindings_to_image((const mach_header_t *)header, slide, rebindings, rebindings_nel);
        return 0;
    }
    
    // 遍历所有已加载的 image
    for (uint32_t i = 0; i < _dyld_image_count(); i++) {
        const mach_header_t *h = _dyld_get_image_header(i);
        intptr_t s = _dyld_get_image_vmaddr_slide(i);
        apply_rebindings_to_image(h, s, rebindings, rebindings_nel);
    }
    
    return 0;
}
