#ifndef RLIST_H
#define RLIST_H

int rlength(Ptr_Rolo_List rlist);

void rolo_reorder();

void rolo_insert(Ptr_Rolo_List link, int (*compare)());

void rolo_delete(Ptr_Rolo_List link);

#endif
