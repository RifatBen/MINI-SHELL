#ifndef completion_h
#define completion_h



int initialize_readline();
char *command_generator(const char *, int);
char **fileman_completion(const char *, int, int);


#endif
