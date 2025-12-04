#ifndef UTILS_H
#define UTILS_H
char *create_temp_with_suffix(const char *suffix);
char *create_temp_ps_from_text(const char *text, int color_mode);
char *convert_image_to_ps(const char *path, int color_mode);
char *convert_pdf_to_ps(const char *path, int color_mode);
char *get_local_subnet_cidr(void);
int ends_with_ci(const char *s, const char *suffix);
void trim(char *s);
const char *escape_shell_arg(const char *s);
void progress_bar(int percent);
long get_file_size(const char *path);
#endif
