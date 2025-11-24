#ifndef PROGRESS_H
#define PROGRESS_H

void progress_init(const char *label, long total_bytes);
void progress_update(long sent_bytes);
void progress_finish(void);

#endif
