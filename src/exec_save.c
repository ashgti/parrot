/*
 * exec_save.c
 *
 * CVS Info
 *    $Id$
 * Overview:
 *    Save the Parrot_exec_objfile_t to the native format
 * History:
 *      Initial version by Daniel Grunblatt on 2003.6.9
 * Notes:
 * References:
 */
 
#include <parrot/parrot.h>
#include "parrot/exec.h"
#include "parrot/exec_save.h"

static void save_zero(FILE *fp);
static void save_int(FILE *fp, int i);
static void save_short(FILE *fp, short s);
static void save_struct(FILE *fp, void *sp, size_t size);

#ifdef EXEC_A_OUT

#  include <a.out.h>
#  include <link.h>

void
Parrot_exec_save(Parrot_exec_objfile_t *obj, const char *file)
{
    FILE *fp;
    int i;
    struct exec header;
    struct relocation_info rellocation;
    struct nlist symlst;

    fp = fopen(file, "w");
    
    header.a_midmag = 0x07018600;
    header.a_text = obj->text.size;
    header.a_data = obj->data.size;
    header.a_bss = obj->bss.size;
    header.a_syms = obj->symbol_count * sizeof(struct nlist);
    header.a_entry = 0;
    header.a_trsize = obj->text_rellocation_count
        * sizeof(struct relocation_info);
    header.a_drsize = obj->data_rellocation_count
        * sizeof(struct relocation_info);
    save_struct(fp, &header, sizeof(struct exec));
   /* Text */
    for (i = 0; i < obj->text.size; i++)
        fprintf(fp, "%c", obj->text.code[i]);
    /* Data */
    for (i = 0; i < obj->data.size; i++)
        fprintf(fp, "%c", obj->data.code[i]);
    /* Text rellocations */
    for (i = obj->text_rellocation_count - 1; i >= 0; i--) {
        bzero(&rellocation, sizeof(struct relocation_info));
        rellocation.r_address = obj->text_rellocation_table[i].offset;
        rellocation.r_symbolnum = obj->text_rellocation_table[i].symbol_number;
        switch (obj->text_rellocation_table[i].type) {
            case RTYPE_FUNC:
                rellocation.r_pcrel = 1;
                rellocation.r_length = 2;
                rellocation.r_extern = 1;
                break;
            case RTYPE_COM:
                rellocation.r_length = 2;
                rellocation.r_extern = 1;
                break;
            case RTYPE_DATA:
                rellocation.r_length = 2;
                rellocation.r_extern = 1;
                break;
            default:
                break;
        }
        save_struct(fp, &rellocation, sizeof(struct relocation_info));
   }
    /* Symbol table */
    for (i = 0; i < obj->symbol_count; i++) {
        bzero(&symlst, sizeof(struct nlist));
        symlst.n_un.n_strx = obj->symbol_table[i].offset_list;
        switch (obj->symbol_table[i].type) {
            case STYPE_FUNC:
                symlst.n_type = N_EXT | N_TEXT;
                symlst.n_other = AUX_FUNC;
                break;
            case STYPE_GDATA:
                symlst.n_type = N_EXT | N_DATA;
                symlst.n_other = AUX_OBJECT;
                symlst.n_value = obj->symbol_table[i].value;
                break;
            case STYPE_COM:
                symlst.n_type = N_EXT;
                symlst.n_value = obj->symbol_table[i].value;
                break;
            case STYPE_UND:
                symlst.n_type = N_EXT;
                break;
            default:
                break;
        }
        save_struct(fp, &symlst, sizeof(struct nlist));
   }
    /* String table size */
    save_int(fp, obj->symbol_list_size);
    /* String table */
    for (i = 0; i < obj->symbol_count; i++) {
        if (obj->symbol_table[i].type != STYPE_GCC)
            fprintf(fp, "_%s", obj->symbol_table[i].symbol);
        else
            fprintf(fp, "%s", obj->symbol_table[i].symbol);
        save_zero(fp);
    }
    fclose(fp);
}

#endif /* EXEC_A_OUT */

#ifdef EXEC_ELF

#  include <elf.h>

#  define sh_add(n,t,f,s,l,i,a,e) { \
    bzero(&sechdr, sizeof(Elf32_Ehdr)); \
    sechdr.sh_name = shste - shst; \
    shste += sprintf(shste, "%s", n); \
    shste++; \
    sechdr.sh_type = t; \
    sechdr.sh_flags = f; \
    sechdr.sh_addr = 0; \
    sechdr.sh_offset = current_offset; \
    sechdr.sh_size = s; \
    sechdr.sh_link = l; \
    sechdr.sh_info = i; \
    sechdr.sh_addralign = a; \
    sechdr.sh_entsize = e; \
    save_struct(fp, &sechdr, sizeof(Elf32_Shdr)); \
    current_offset += s; \
    if (s % 4) \
      current_offset += (4 - s % 4); \
   }

#  define SHSTRTABSIZE  0x48
#  define PDFS          4
#  define NSECTIONS     8

void
Parrot_exec_save(Parrot_exec_objfile_t *obj, const char *file)
{
    Elf32_Ehdr header;
    Elf32_Shdr sechdr;
    Elf32_Rel rellocation;
    Elf32_Sym symlst;
    Elf32_Off current_offset;
    FILE *fp;
    int i;
    char shst[SHSTRTABSIZE], *shste;

    fp = fopen(file, "w");
    
    bzero(&header, sizeof(Elf32_Ehdr));
    header.e_ident[0] = ELFMAG0;
    header.e_ident[1] = ELFMAG1;
    header.e_ident[2] = ELFMAG2;
    header.e_ident[3] = ELFMAG3;
    header.e_ident[4] = ELFCLASS32;
    header.e_ident[5] = ELFDATA2LSB;
    header.e_ident[6] = EV_CURRENT;
#  if EXEC_OS == FREEBSD
    header.e_ident[7] = ELFOSABI_FREEBSD;
#  endif
#  if EXEC_OS == NETBSD
    header.e_ident[7] = ELFOSABI_NETBSD;
#  endif
#  if EXEC_OS == LINUX && defined(ELFOSABI_LINUX)
    header.e_ident[7] = ELFOSABI_LINUX;
#  endif

    header.e_type = ET_REL;
    header.e_machine = EM_386;
    header.e_version = EV_CURRENT;
    header.e_entry = 0;
    header.e_phoff = 0;
    header.e_shoff = sizeof(Elf32_Ehdr);
    header.e_flags = 0;
    header.e_ehsize = sizeof(Elf32_Ehdr);
    header.e_phentsize = 0;
    header.e_phnum = 0;
    header.e_shentsize = sizeof(Elf32_Shdr);
    header.e_shnum = NSECTIONS;
    header.e_shstrndx = 1;

    save_struct(fp, &header, sizeof(Elf32_Ehdr));

    current_offset = sizeof(Elf32_Ehdr) + NSECTIONS * sizeof(Elf32_Shdr);

    /* Sections */
    bzero(&shst, SHSTRTABSIZE);
    shste = shst + 1;
    /* NULL */
    bzero(&sechdr, sizeof(Elf32_Ehdr));
    save_struct(fp, &sechdr, sizeof(Elf32_Shdr));
    /* Section Header String Table */
    sh_add(".shstrtab", SHT_STRTAB, 0, SHSTRTABSIZE, 0, 0, 1, 0);
    /* Text */
    sh_add(".text", SHT_PROGBITS, SHF_ALLOC | SHF_EXECINSTR, obj->text.size,
        0, 0, 4, 0);
    /* Data */
    sh_add(".data", SHT_PROGBITS, SHF_WRITE | SHF_ALLOC, obj->data.size,
        0, 0, 4, 0);
    /* Bss */
    sh_add(".bss", SHT_NOBITS, SHF_WRITE | SHF_ALLOC, obj->bss.size,
        0, 0, 4, 0);
    /* 
     * Text rellocation records.
     * Link must be the symtab section header index.
     * Info is the text section header index.
     */
    sh_add(".rel.text", SHT_REL, 0, obj->text_rellocation_count *
        sizeof(Elf32_Rel), 6, 2, 4, sizeof(Elf32_Rel));
    /* 
     * Symbol table.
     * Link is the strtab section header index.
     * Info is the index of the first symbol in the symbol table.
     */
    sh_add(".symtab", SHT_SYMTAB, 0, (obj->symbol_count + PDFS) *
        sizeof(Elf32_Sym), 7, PDFS - 1, 4, sizeof(Elf32_Sym));
    /* String Table */
    obj->symbol_list_size += 1; /* Trailing \0 */
    sh_add(".strtab", SHT_STRTAB, 0, obj->symbol_list_size, 0, 0, 1, 0);

    /* Section header string table */
    save_struct(fp, &shst, SHSTRTABSIZE);
    save_struct(fp, obj->text.code, obj->text.size); /* Text */
    save_struct(fp, obj->data.code, obj->data.size); /* Data */
    /* Text rellocations */
    for (i = 0; i < obj->text_rellocation_count; i++) {
        bzero(&rellocation, sizeof(Elf32_Rel));
        rellocation.r_offset = obj->text_rellocation_table[i].offset;
        switch (obj->text_rellocation_table[i].type) {
            case RTYPE_FUNC:
                rellocation.r_info =
                    ELF32_R_INFO(
                        obj->text_rellocation_table[i].symbol_number + PDFS,2);
                break;
            case RTYPE_DATA:
            case RTYPE_COM:
                rellocation.r_info =
                    ELF32_R_INFO(
                        obj->text_rellocation_table[i].symbol_number + PDFS,1);
                break;
            default:
                break;
        }
        save_struct(fp, &rellocation, sizeof(Elf32_Rel));
   }
    /* Symbol table */
    /* zero */
    bzero(&symlst, sizeof(Elf32_Sym));
    save_struct(fp, &symlst, sizeof(Elf32_Sym));
    /* Text */
    bzero(&symlst, sizeof(Elf32_Sym));
    symlst.st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
    symlst.st_shndx = 2;
    save_struct(fp, &symlst, sizeof(Elf32_Sym));
    /* Data */
    bzero(&symlst, sizeof(Elf32_Sym));
    symlst.st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
    symlst.st_shndx = 3;
    save_struct(fp, &symlst, sizeof(Elf32_Sym));
    /* Bss */
    bzero(&symlst, sizeof(Elf32_Sym));
    symlst.st_info = ELF32_ST_INFO(STB_LOCAL, STT_SECTION);
    symlst.st_shndx = 4;
    save_struct(fp, &symlst, sizeof(Elf32_Sym));

    for (i = 0; i < obj->symbol_count; i++) {
        bzero(&symlst, sizeof(Elf32_Sym));
        symlst.st_name = obj->symbol_table[i].offset_list + 1;
        switch (obj->symbol_table[i].type) {
            case STYPE_FUNC:
                symlst.st_info = ELF32_ST_INFO(STB_GLOBAL, STT_FUNC);
                symlst.st_size = obj->text.size;
                symlst.st_shndx = 2; /* text */
                break;
            case STYPE_GDATA:
                symlst.st_value = obj->symbol_table[i].value;
                symlst.st_info = ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT);
                symlst.st_shndx = 3; /* data */
                break;
            case STYPE_COM:
                symlst.st_value = obj->symbol_table[i].value;
                symlst.st_info = ELF32_ST_INFO(STB_GLOBAL, STT_OBJECT);
                symlst.st_shndx = SHN_COMMON;
                break;
            case STYPE_UND:
                symlst.st_info = ELF32_ST_INFO(STB_GLOBAL, STT_NOTYPE);
                break;
            default:
                break;
        }
        save_struct(fp, &symlst, sizeof(Elf32_Sym));
   }
    /* String table */
    save_zero(fp);
    for (i = 0; i < obj->symbol_count; i++) {
        fprintf(fp, "%s", obj->symbol_table[i].symbol);
        save_zero(fp);
    }
    /* PAD */
    for (i = 0; i < (4 - obj->symbol_list_size % 4); i++)
        save_zero(fp);
    fclose(fp);
}

#endif /* EXEC_ELF */

#if EXEC_MACH_O

/* This is a hack. */
 
void
Parrot_exec_save(Parrot_exec_objfile_t *obj, const char *file)
{
    FILE *fp;
    int i;

    fp = fopen(file, "w");
    
    fprintf(fp, "\xFE\xED\xFA\xCE"); /* Header for Darwin */
    save_int(fp, 0x12);
    save_int(fp, 0);
    save_int(fp, 0x1);
    save_int(fp, 0x3);
    save_int(fp, 0x128);
    save_int(fp, 0);
    save_int(fp, 0x1);
    save_int(fp, 0xC0);
    for (i = 0; i < 5; i++)
        save_int(fp, 0);
    /* Sizeof text + data */
    save_int(fp, obj->text.size + obj->data.size);
    /* Offset of text */
    save_int(fp, 0x144);
    save_int(fp, obj->text.size + obj->data.size);
    save_int(fp, 0x7);
    save_int(fp, 0x7);
    save_int(fp, 0x2);
    save_int(fp, 0);
    fprintf(fp, "__text");
    for (i = 0; i < 10; i++)
        save_zero(fp);
    fprintf(fp, "__TEXT");
    for (i = 0; i < 10; i++)
        save_zero(fp);
    save_int(fp, 0);
    /* Sizeof text */
    save_int(fp, obj->text.size);
    save_int(fp, 0x144);
    save_int(fp, 0x2);
    /* Offset of rellocation table. */
    save_int(fp, 0x144 + obj->text.size + obj->data.size);
    save_int(fp, obj->text_rellocation_count);
    save_int(fp, 0x80000400);
    save_int(fp, 0);
    save_int(fp, 0);
    fprintf(fp, "__data");
    for (i = 0; i < 10; i++)
        save_zero(fp);
    fprintf(fp, "__DATA");
    for (i = 0; i < 10; i++)
        save_zero(fp);
    /* Data VMA */
    save_int(fp, obj->text.size);
    /* Data size */
    save_int(fp, obj->data.size);
    /* Data file offset */
    save_int(fp, 0x144 + obj->text.size);
    save_int(fp, 0x2);
    for (i = 0; i < 5; i++)
        save_int(fp, 0);
    save_int(fp, 0x2);
    /*    save_int(fp, obj->symbol_count * 0xc); */
    save_int(fp, 0x18);
    /* Offset of stabs */
    save_int(fp, 0x144 +
        obj->text.size + obj->data.size + obj->text_rellocation_count * 0x8);
    /* Number of stabs (symbol table) */
    save_int(fp, obj->symbol_count);
    /* Offset of symbol list */
    save_int(fp, 0x144 + obj->text.size + obj->data.size +
        obj->text_rellocation_count * 0x8 + obj->symbol_count * 0xc);
    /* Sizeof symbol list */
    save_int(fp, obj->symbol_list_size);
    save_int(fp, 0xB);
    save_int(fp, 0x50);
    for (i = 0; i < 3; i++)
        save_int(fp, 0);
    save_int(fp, obj->symbol_count);
    save_int(fp, obj->symbol_count);
    for (i = 0; i < 13; i++)
        save_int(fp, 0);
    /* Text */
    for (i = 0; i < obj->text.size; i++)
        fprintf(fp, "%c", obj->text.code[i]);
    /* Data */
    for (i = 0; i < obj->data.size; i++)
        fprintf(fp, "%c", obj->data.code[i]);
    /* Text rellocations */
    for (i = obj->text_rellocation_count - 1; i >= 0; i--) {
        save_int(fp, obj->text_rellocation_table[i].offset);
        save_short(fp, obj->text_rellocation_table[i].symbol_number);
        save_short(fp, obj->text_rellocation_table[i].type);
    }
    /* Symbol table */
    for (i = 0; i < obj->symbol_count; i++) {
        save_int(fp, obj->symbol_table[i].offset_list);
        save_int(fp, obj->symbol_table[i].type);
        save_int(fp, obj->symbol_table[i].value);
    }
    /* Symbol list */
    for (i = 0; i < obj->symbol_count; i++) {
        if (obj->symbol_table[i].type != STYPE_GCC)
            fprintf(fp, "_%s", obj->symbol_table[i].symbol);
        else
            fprintf(fp, "%s", obj->symbol_table[i].symbol);
        save_zero(fp);
    }
    fclose(fp);
}

#endif /* EXEC_MACH_O */

static void
save_struct(FILE *fp, void *sp, size_t size)
{
    unsigned int i;

    for (i = 0; i < size; i++)
        fprintf(fp, "%c", ((char *)sp)[i]);
}

static void
save_zero(FILE *fp)
{
    fprintf(fp, "%c", 0);
}

#if PARROT_BIGENDIAN

static void
save_int(FILE *fp, int i)
{
    fprintf(fp, "%c%c%c%c", (char)(i >> 24), (char)(i >> 16),
                            (char)(i >> 8), (char)i);
}

static void
save_short(FILE *fp, short s)
{
    fprintf(fp, "%c%c", (char)(s >> 8), (char)s);
}

#else /* PARROT_BIGENDIAN */

static void
save_short(FILE *fp, short s)
{
    fprintf(fp, "%c%c", (char)s, (char)(s >> 8));
}

static void
save_int(FILE *fp, int i)
{
    fprintf(fp, "%c%c%c%c", (char)i, (char)(i >> 8),
                            (char)(i >> 16), (char)(i >> 24));
}

#endif /* PARROT_BIGENDIAN */
