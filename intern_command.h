
#ifndef intern_command_h
#define intern_command_h


typedef struct Alias{
  char *alias;
  char *command;
  struct Alias *next;
}Alias;



typedef struct List_alias{
    Alias *head;
}List_alias;

typedef struct Var{
  char *name;
  char *value;
  struct Var *next;
}Var;

typedef struct List_var{
  Var *head;
}List_var;

extern List_alias *alias_list;

extern List_var *var_list;

extern int (*internal_fun[]) (char **);

extern char *interns[];

extern int resultat;

int load_mpshrc();


List_alias *initAliasList();
int isAlias(char *line, char *token);
Alias *newAlias(char *alias, char *command);
List_alias *addAlias(Alias *new);
Alias *existAlias(char *alias);

List_var *initVarList();
int isVar(char *line, char *token);
Var *newVar(char *name, char *value);
List_var *addVar(Var *new);
char *getvar(char *name);
Var *existVar(char *name);
void deleteVar(char *name);

int internal_num();
int mpsh_cd(char **args);
int mpsh_exit(char **args);
int mpsh_echo(char **args);
int mpsh_alias(char **args);
int mpsh_unalias(char **args);
void show_aliases();
int mpsh_umask(char **args);
int mpsh_export(char **args);
int mpsh_type(char **args);
int lookSub(char *command, char *subPath);
char *strdup (const char *s);
#endif
