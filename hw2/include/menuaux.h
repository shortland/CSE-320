#ifndef MENUAUX_H
#define MENUAUX_H

int rolo_menu_yes_no(char *prompt, int rtn_default, int help_allowed, char *helpfile, char *subject);

int rolo_menu_number_help_or_abort(char *prompt, int low, int high, int *ptr_ival);

int rolo_menu_data_help_or_abort(char *prompt, char *helpfile, char *subject, char **ptr_response);

#endif