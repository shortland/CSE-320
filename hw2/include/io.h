#ifndef IO_C
#define IO_C

void write_rolo(FILE *fp1, FILE *fp2);

void cathelpfile(char *filepath, char *helptopic, int clear);

int read_rolodex(int fd);

void any_char_to_continue();

void display_entry(Ptr_Rolo_Entry entry);

#endif