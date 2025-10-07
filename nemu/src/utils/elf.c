/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "debug.h"

#include <elf.h>
#include <stdio.h>

typedef struct ftrace_sym_t {
  uint32_t addr;
  uint32_t size;
  char name[128];
} ftrace_sym_t;

#define FTRACE_TABLE_MAXSIZE 1024
ftrace_sym_t ftrace_table[FTRACE_TABLE_MAXSIZE];
size_t ftrace_size = 0;

static void *read_chunk(FILE *fp, size_t off, size_t sz) {
  void *buf = malloc(sz);
  if (fseek(fp, (long)off, SEEK_SET) != 0) {
    free(buf);
    panic("fseek fail");
  }
  size_t r = fread(buf, 1, sz, fp);
  Assert(r == sz, "read fail");
  return buf;
}

static void free_chunk(void *buf) {
  free(buf);
}

void ftrace_table_push(uint32_t addr, uint32_t size, const char *name) {
  Assert(ftrace_size < FTRACE_TABLE_MAXSIZE, "ftrace table is full");
  ftrace_table[ftrace_size].addr = addr;
  ftrace_table[ftrace_size].size = size;

  Assert(name != nullptr, "name is null");
  Assert(strlen(name) < sizeof(ftrace_table[ftrace_size].name), "name is too long");

  strncpy(ftrace_table[ftrace_size].name, name, sizeof(ftrace_table[ftrace_size].name));
  ftrace_size++;
  Log("ftrace: %s @ 0x%x", name, addr);
}

void init_ftrace(const char *file) {
  if (file == NULL) {
    Log("ftrace: no file specified");
    return;
  }

  FILE *fp = fopen(file, "rb");
  Assert(fp != NULL, "Can not open %s", file);
  Log("Reading ELF: %s", file);

  unsigned char e_ident[EI_NIDENT];
  size_t r = fread(e_ident, 1, EI_NIDENT, fp);
  Assert(r == EI_NIDENT, "read elf ident fail");
  Assert(e_ident[EI_MAG0] == ELFMAG0 && e_ident[EI_MAG1] == ELFMAG1 && e_ident[EI_MAG2] == ELFMAG2 &&
             e_ident[EI_MAG3] == ELFMAG3,
         "not an ELF file");

  Assert(e_ident[EI_CLASS] == ELFCLASS32, "Unsupported ELF class");
  Assert(e_ident[EI_DATA] == ELFDATA2LSB, "Unsupported ELF endian");

  // rewind to beginning and parse proper header
  fseek(fp, 0, SEEK_SET);

  Elf32_Ehdr eh;
  r = fread(&eh, 1, sizeof(eh), fp);
  Assert(r == sizeof(eh), "read ehdr failed");

  // TODO
  Assert(eh.e_shnum > 0, "todo: SHN_LORESERVE");

  // read all section headers
  Elf32_Shdr *shdrs = (Elf32_Shdr *)read_chunk(fp, eh.e_shoff, eh.e_shentsize * eh.e_shnum);

  // find SHT_SYMTAB / SHT_DYNSYM
  for (int i = 0; i < eh.e_shnum; i++) {
    uint32_t stype = shdrs[i].sh_type;
    if (stype == SHT_SYMTAB || stype == SHT_DYNSYM) {
      uint32_t sz_ent = shdrs[i].sh_entsize;
      uint32_t num_ents = shdrs[i].sh_size / sz_ent;
      void *section = read_chunk(fp, shdrs[i].sh_offset, shdrs[i].sh_size);

      // linked string table index
      uint32_t stridx = shdrs[i].sh_link;
      uint32_t str_off = shdrs[stridx].sh_offset;
      uint32_t str_size = shdrs[stridx].sh_size;
      char *strtab = (char *)read_chunk(fp, str_off, str_size);

      for (uint32_t si = 0; si < num_ents; si++) {
        Elf32_Sym sym;
        memcpy(&sym, (char *)section + si * sz_ent, sizeof(sym));
        unsigned char st_type = ELF32_ST_TYPE(sym.st_info);
        if (st_type == STT_FUNC && sym.st_shndx != SHN_UNDEF) {
          const char *name = strtab + sym.st_name;
          ftrace_table_push(sym.st_value, sym.st_size, name);
        }
      }
      free_chunk(section);
      free_chunk(strtab);
    }
  }

  free_chunk(shdrs);
  fclose(fp);

  Log("ftrace: loaded %zu function symbols", ftrace_size);
}