#ifndef DISCOVER_H
#define DISCOVER_H
/* returns malloc'd string with CUPS printer name or NULL */
char *discover_cups_printer(void);
/* returns malloc'd ip string like "192.168.1.40" or NULL */
char *discover_printer_ip(void);
#endif
