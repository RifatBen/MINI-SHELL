#define  _POSIX_C_SOURCE  200809L
#define _GNU_SOURCE
#define SEPARATEUR " "
#define SIZE 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <unistd.h>
#include "intern_command.h"
#include "completion.h"
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>




int traitement_ligne_de_commande(char *line);
int mpsh_loop();
char **line_split(char *line);
int varDec(char **args);
int exec_alias(char **args);
int isSequence(char **args, char ***args1, char ***args2, char **operator);
char **getFirstPart(char **args,int i);
int execSequence(char **args, char **args1, char **args2, char *operator);
int execRedirect(char **args1, char **args2);
int exec_extern(char **args);
int my_execvp(char *command, char **line);
int execSub(char *command, char **line, char *subPath);
int exec_command(char **args);
int internal_num();

//Liste des aliases
List_alias *alias_list;
List_var *var_list;
//Pointeur sur les fonctions internes
int (*internal_fun[]) (char **) = {
  &mpsh_cd,
  &mpsh_exit,
  &mpsh_echo,
  &mpsh_alias,
  &mpsh_unalias,
  &mpsh_export,
  &mpsh_umask,
  &mpsh_type
};

//Tableau contenant les nom des fonctions internes
char *interns[] = {
  "cd",
  "exit",
  "echo",
  "alias",
  "unalias",
  "export",
  "umask",
  "type"
};

//
int in=0,out=0,err=0;

/************************* MAIN ************************************/

int main(){


  load_mpshrc();

  //fonction principale d'execution de commandes
  mpsh_loop();


  return 0;
}

int mpsh_loop(){
  char *line;

  initialize_readline();	/* pour fabriquer les complétions */

  while(1){
    in=out=err=0;
    int resultat;
    char *prompt;
    if((prompt=getvar("INVITE"))==NULL)
    prompt=getenv("INVITE");
    line = readline(prompt);
    resultat= traitement_ligne_de_commande(line);
    char res[8];
    sprintf(res,"%d",resultat);
    setenv("?",res,1);
    free(line);
  }
  exit(0);
}
//Fonction qui retourne la première opérande
char **getFirstPart(char **args,int i){
  char **args1=(char**)calloc(SIZE, sizeof(char*));
  int j;
  for(j = 0;j<i;j++)
  args1[j]=args[j];
  return args1;
}
//Vérifie si la commande demandée est un enchainement de commandes
int isSequence(char **args, char ***args1, char ***args2, char **operator){
  int i;
  for(i=0; args[i]; i++){
    if(strcmp(args[i],"&&")==0 || strcmp(args[i],"||")==0 || strcmp(args[i],"<")==0 || strcmp(args[i],">")==0){
      *operator=strdup(args[i]);
      *args1 = getFirstPart(args,i);
      *args2=args+i+1;
      return 1;
    }
  }
  return 0;
}

int traitement_ligne_de_commande(char *line){
  char **args = line_split(line);

  if(args[0]!=NULL){
    char *equals = strchr(args[0],'=');
    if(equals==NULL){
      char **args1=(char**)calloc(SIZE, sizeof(char*)), **args2=(char**)calloc(SIZE, sizeof(char*)), *operator=(char*)calloc(3,sizeof(char));
      return execSequence(args,args1,args2,operator);
    }
    else
      return varDec(args);
  }
  return 1;
}

int execSequence(char **args,char **args1, char **args2, char *operator){
  int res;
  if(isSequence(args,&args1,&args2,&operator)){
    if(strcmp(operator,"&&")==0 && args2[0]){
      res = exec_alias(args1);
      if(res>0)
        return execSequence(args2,args1,args2,operator);
      else
        return res;
    }
    else if(strcmp(operator,"||")==0){
      res = exec_alias(args1);
      if(res>0)
      return res;
      else
      return execSequence(args2,args1,args2,operator);
    }
    else if(strcmp(operator,">")==0){
      out=1;

      return execRedirect(args1,args2);
    }
    else if(strcmp(operator,"<")==0){
      in=1;
      return execRedirect(args1,args2);
    }
    else if(strcmp(operator,"2>")==0){
      err=1;
      return execRedirect(args1,args2);
    }
  }
  else
    return exec_alias(args);
  return 0;
}



int varDec(char **args){
  //On insère un '\0' a la place du '='
  char var_dec_line[strlen(args[0])+1];
  strcpy(var_dec_line,args[0]);
  char *equals = strchr(var_dec_line,'=');
  //pas la peine de vérifier si equals est nul, car on l'a déjà vérifié plus haut
  *equals = 0;
  char *name = var_dec_line;
  char *value_start = equals+1;
  size_t value_len = strlen(value_start); // + le reste
  int i;
  for(i=2; args[i]; i++)
  value_len+=strlen(args[i])+1;

  char* value= (char*)malloc((value_len+1)*sizeof(char));
  strcpy(value,value_start);

  for(i=1; args[i]; i++){
    strcat(value," ");
    strcat(value,args[i]);
  }
  if(getenv(name)!=NULL)
  setenv(name,value,1);
  else{
    Var *var=existVar(name);
    if(var==NULL){
      var = newVar(name,value);
      var_list=addVar(var);
    }
    else
    var->value=strdup(value);
  }

  return 1;

}

int execRedirect(char **args1, char **args2){
  pid_t rc = fork();
  if (rc < 0){
    perror("fork error");
    return 0;
  }
  if (rc == 0){
    if(in){
      int fd0;
      if((fd0 = open(args2[0], O_RDONLY, 0)) == -1){
        perror(args2[0]);
        exit(EXIT_FAILURE);
      }
      dup2(fd0, 0);
      close(fd0);
    }
    if(out){
      int fd1;
      if((fd1 = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC | O_CREAT,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
          perror (args2[0]);
          exit( EXIT_FAILURE);
        }
        dup2(fd1, 1);
        close(fd1);
      }
      if(err){
        int fd2;
        if((fd2 = open(args2[0], O_WRONLY | O_CREAT | O_TRUNC | O_CREAT,
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1){
            perror (args2[0]);
            exit( EXIT_FAILURE);
          }
          dup2(fd2, 2);
          close(fd2);
      }

      // l'enfant execute la commande passée dans args1
      return my_execvp(args1[0],args1);
    }
    else
      wait(NULL);

    return 1;
  }

  int exec_alias(char **args){
    if(args[0]==NULL)
    return 0;

    Alias *current = alias_list->head;
    while(current!=NULL){
      if(strcmp(args[0],current->alias)==0){
        char **cmd = line_split(current->command);
        return exec_command(cmd);
      }
      current = current->next;
    }
    return exec_command(args);
  }

  int exec_command(char **args){

    int nbr_fct_intern = internal_num();
    int i;
    for (i=0;i<nbr_fct_intern; i++) {
      if(strcmp(args[0], interns[i]) == 0) {
        return (*internal_fun[i])(args);
      }
    }

    return exec_extern(args);

  }

  //fonction qui prend en entré une ligne de commande et la sépare en plusieurs arguments
  char **line_split(char *line){
    char *linedup = strdup(line);
    char *arg = strtok(linedup,SEPARATEUR);
    char **args = malloc(SIZE * sizeof(char*));

    int i = 0, buffer = SIZE;

    if (!args) {
      perror("malloc error");
      exit(EXIT_FAILURE);
    }

    while (arg != NULL) {
      args[i++] = arg;


      //S'il y'a dépassement de la taille on resize
      if (i >= buffer) {
        buffer += SIZE;
        args = realloc(args, buffer * sizeof(char*));

        if (!args) {
          perror("realloc error");
          exit(EXIT_FAILURE);
        }
      }
      //on passe au prochain argument
      arg = strtok(NULL, SEPARATEUR);
    }
    //On determine la fin des arguments par un NULL
    args[i] = NULL;

    return args;
  }

  //fonction qui s'occupe de lancer l'execution des commandes externes a mpsh
  int exec_extern(char **args){
    pid_t pid = fork();
    if(pid == 0){
      //Child, il execute la commande passée en parametre
      if(my_execvp(args[0],args) == -1){
        //S'il  y'a un retour, c'est qu'il y a un problème avec l'exec
        printf("error exec : command %s not found\n",args[0]);
      }
      exit(EXIT_FAILURE);
    }
    else if(pid < 0){
      perror("error fork");
    }
    else{
      //Parent c'est à dire le mpsh, attend la fin de l'execution du processus enfant
      wait(NULL);
    }
    return 1;
  }


  int my_execvp(char *command, char **line){
    //On charge la variable CHEMIN
    char *chemin = strdup(getenv("CHEMIN"));
    //On split CHEMIN par rapport à ":"
    char *token = strtok(chemin,":");
    //On vérifie la présence de command dans le répértoire
    while(token!=NULL){
      char *slash = strstr(token,"//");

      //Si le répértoire ne contient pas "//" alors on ne cherche pas dans les sous répértoires
      char *path=(char*)malloc((strlen(token)+strlen(command)+2) * sizeof(char));
      sprintf(path,"%s/%s",token,command);
      //S'il y a un retour de l'exec, ça veut dire que la commande n'y est pas, alors on passe au répértoire suivant
      if(slash==NULL)
      execv(path,line);

      else{
        *slash = 0;
        //chercher dans le répértoire courant & son arborescence
        execv(path,line);
        DIR *dir = opendir(token);
        struct dirent *de;
        while((de=readdir(dir))!=NULL){
          if(de->d_type == DT_DIR && strcmp(de->d_name,"..")!=0 && strcmp(de->d_name,".")!=0){
            char subPath[strlen(path)+strlen(de->d_name)+2];
            sprintf(subPath,"%s/%s",token,de->d_name);
            execSub(command,line,subPath);
          }
        }
        closedir(dir);
      }
      token = strtok(NULL,":");
    }
    //Si on ne trouve rien au final, on retourne un exec error
    return -1;
  }

  int execSub(char *command, char **line, char *path){
    char toExec[strlen(command)+strlen(path)+2];
    sprintf(toExec,"%s/%s",path,command);

    execv(toExec,line);
    DIR *dir = opendir(path);
    struct dirent *de;
    while((de=readdir(dir))!=NULL){
      if(de->d_type == DT_DIR && strcmp(de->d_name,"..")!=0 && strcmp(de->d_name,".")!=0){
        char subPath[strlen(path)+strlen(de->d_name)+2];
        sprintf(subPath,"%s/%s",path,de->d_name);
        execSub(command,line,subPath);
      }
    }
    closedir(dir);
    return 0;
  }

  // Retourne le nombre de fonctions internes
  int internal_num(){
    return sizeof(interns) / sizeof(char*);
  }
