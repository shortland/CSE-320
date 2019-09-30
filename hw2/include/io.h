#ifndef IO_C
#define IO_C

void write_rolo(FILE *fp1, FILE *fp2);

int cathelpfile(char *filepath, char *helptopic, int clear);

int read_rolodex(int fd);

#endif