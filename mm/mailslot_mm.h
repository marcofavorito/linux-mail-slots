//
// Created by marcofavorito on 01/11/17.
//

#ifndef MAILSLOT_MM_H
#define MAILSLOT_MM_H

void init_mm_ds(void);
void deinit_mm_ds(void);


struct mailslot_dev* get_mailslot_dev(int minor);
void* alloc_mailslot_dev(int minor);
void dealloc_mailslot_dev(struct mailslot_dev* dev);

void* alloc_mailslot_data(void);
void dealloc_mailslot_data(struct mailslot_data *data);
int get_data_size(struct mailslot_data *data);

void* alloc_data(int count);
void dealloc_data(void* data);

long spacefree(struct mailslot_dev * dev);


#endif //MAILSLOT_MM_H
