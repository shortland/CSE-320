#ifndef ROLO_H
#define ROLO_H

int OPTION_SUMMARY_FLAG;
int OPTION_NOLOCK_FLAG;
int OPTION_OTHERUSER_FLAG;
char *OTHER_USERNAME;
int NON_OPTION_ARGS;

char *ALLOCATED_MEM[100000];
int ALLOCATED_INDEX;

void save_and_exit(int rval);

void user_eof();

void roloexit(int rval);

void save_to_disk();

#endif
