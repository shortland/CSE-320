#ifndef IO_C
#define IO_C

void write_rolo(FILE *fp1, FILE *fp2);

void cathelpfile(char *filepath, char *helptopic, int clear);

int read_rolodex(int fd);

void any_char_to_continue();

void display_entry(Ptr_Rolo_Entry entry);

void display_entry_for_update(Ptr_Rolo_Entry entry);

void display_field_names();

void summarize_entry_list(Ptr_Rolo_List rlist, char *ss);

#endif
