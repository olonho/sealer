/**
 * ELF file symbols sealer.
 * R&D and code by Nikolay Igotti.
 * Theory of operations.
 * When statically linking with libstdc++ we could encounter
 * rather nasty side effects coming from ODR and conflicts with system libstdc++.* So to make library fully "sealed" we introduce a tool which modifies all
 * symbols but having certain prefix to be hidden by default and thus not 
 * interferring with system libstc++.
 */

#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

Elf64_Shdr* elf_sheader(Elf64_Ehdr* hdr) {
  return (Elf64_Shdr *)((char*)hdr + hdr->e_shoff);
}

Elf64_Shdr* elf_section(Elf64_Ehdr* hdr, uint idx) {
  return &elf_sheader(hdr)[idx];
}

Elf64_Sym* elf_get_sym(Elf64_Ehdr* hdr, int table, uint idx) {
  if (table == SHN_UNDEF || idx == SHN_UNDEF) return NULL;
  Elf64_Shdr* symtab = elf_section(hdr, table);
  uint32_t symtab_entries = symtab->sh_size / symtab->sh_entsize;
  if (idx >= symtab_entries) {
    printf("Symbol Index out of Range (%d:%u).\n", table, idx);
    return NULL;
  }
  char* symaddr = (char*)hdr + symtab->sh_offset;
  return &((Elf64_Sym*)symaddr)[idx];
}

char* elf_str_table(Elf64_Ehdr* ehdr, int index) {
  if (ehdr->e_shstrndx == SHN_UNDEF) return NULL;
  return (char*)ehdr + elf_section(ehdr, index)->sh_offset;
}
 
char* elf_lookup_string(Elf64_Ehdr* ehdr, int index, int offset) {
  char* strtab = elf_str_table(ehdr, index);
  if (strtab == NULL) return NULL;
  return strtab + offset;
}

void seal_sym_section(Elf64_Ehdr* ehdr, Elf64_Shdr* symtab, const char* prefix) {
  uint32_t symtab_entries = symtab->sh_size / symtab->sh_entsize;
  printf("symtab got %d entries: strings in section %d\n", symtab_entries, ehdr->e_shstrndx);
  int strtab_index = symtab->sh_link;
  char* symaddr = (char*)ehdr + symtab->sh_offset;
  const char* types[] = { "NOTYPE", "OBJECT", "FUNC", "SECTION" };
  for (int i = 0; i < symtab_entries; i++) {
    Elf64_Sym* sym =  &((Elf64_Sym*)symaddr)[i];
    if (sym->st_name != STN_UNDEF) {
      uint8_t type = ELF64_ST_TYPE(sym->st_info);
      char* name = elf_lookup_string(ehdr, strtab_index, sym->st_name);
      if (memcmp(name, prefix, strlen(prefix)) == 0) continue;
      if (sym->st_shndx == SHN_UNDEF) continue;
      if (sym->st_other == STV_DEFAULT && (type == STT_OBJECT || type == STT_FUNC)) {
	printf("Hide global object %s: %s\n", name, types[type]);
	sym->st_other = STV_PROTECTED;
      }
    }
  }
}

int seal_dynsym(void* base, const char* prefix) {
  Elf64_Ehdr* ehdr = (Elf64_Ehdr*)base;
  if (memcmp(&ehdr->e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0) {
    printf("Not an ELF\n");
    return 1;
  }
  if (ehdr->e_machine != EM_X86_64 && ehdr->e_machine != EM_AARCH64) {
    printf("Incorrect machine %d, must be %d or %d\n", ehdr->e_machine, EM_X86_64, EM_AARCH64);
    return 1;
  }
  printf("Looks like an %s ELF with %d sections\n",
	 ehdr->e_machine == EM_AARCH64 ? "arm64" : "x64", ehdr->e_shnum);
  for (int i = 0; i < ehdr->e_shnum; i++) {
    Elf64_Shdr* section = elf_section(ehdr, i);
    // In theory, enough to only seal SHT_DYNSYM.
    if (section->sh_type == SHT_DYNSYM || section->sh_type == SHT_SYMTAB) {
      printf("seal sym section %p\n", section);
      seal_sym_section(ehdr, section, prefix);
    }
  }
  return 0;
}

int seal(const char* file, const char* prefix) {
  void* base;
  off_t size;
  struct stat statbuf;
  int fd = open(file, O_RDWR);
  if (fd < 0) {
    perror("open");
    return 1;
  }
  if (fstat(fd, &statbuf) < 0) {
    perror("fstat");
    return 1;
  }
  size = statbuf.st_size;
  base = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (base == MAP_FAILED) {
    perror("mmap");
    return 1;
  }
  printf("ELF is %lld bytes, mapped to %p\n", (long long)size, base);
  return seal_dynsym(base, prefix);
}

int main(int argc, char** argv) {
  const char* file = NULL;
  const char* prefix = "Java_";
  int c;
  while ((c = getopt (argc, argv, "f:p:")) != -1) {
    switch (c) {
      case 'f':
        file = optarg;
        break;
      case 'p':
	prefix = optarg;
	break;
    }
  }
  printf("Modding %s: sealing all but \"%s_*\"\n", file, prefix);
  
  return seal(file, prefix);
}
