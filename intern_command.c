#define  _POSIX_C_SOURCE  200809L
#define _GNU_SOURCE
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

char *strdup (const char *s)
{
  size_t len = strlen (s) + 1;
  void *new = malloc (len);

  if (new == NULL)
    return NULL;

  return (char *) memcpy (new, s, len);
}

List_alias *initAliasList(){
  alias_list = (List_alias*)malloc(sizeof(List_alias));
  alias_list->head =  NULL;
  return alias_list;
}


List_var *initVarList(){
  var_list = (List_var*)malloc(sizeof(List_var));
  var_list->head =  NULL;
  return var_list;
}


int load_mpshrc(){
  //On ouvre le fichier mpshrc
  char path[256];
  strcpy(path,getenv("HOME"));
  strcat(path,"/projet-csystem/.mpshrc");
  FILE *rc = fopen(path,"r");
  //Si .mpshrc n'existe pas, on le crée avec les informations nécéssaire au lancement de mpsh
  if(rc==NULL){
    rc=fopen(path,"w+");
    fputs("INVITE=mpsh>$",rc);
    fclose(rc);
  }
  rc=fopen(path,"r");
  char *line= NULL;
  char delim[4]= " \t";
  size_t size=64;
  //On crée la alias_liste d'alias
  alias_list = initAliasList();
  var_list = initVarList();

  //Déclaration de la variabe d'environement CHEMIN qui est une copie de PATH (elle sera modifiée si elle est déclarée differemment dans .mpshrc)
  setenv("CHEMIN",getenv("PATH"),1);

  while(getline(&line,&size,rc)!=-1){
    //On duplique la ligne lu, pour ne pas la modifier a cause du strtok.
    char *linedup = strdup(line);
    char *token = strtok(linedup,delim);

    //On vérifie si la ligne lue est une déclaration d'alias.
    if(!isAlias(line,token) && !isVar(line,token))
    continue;

  }


  fclose(rc);

  return 1;
}


//Vérifie si une ligne donnée est un alias, et l'ajoute a la alias_liste si c'est le cas
int isAlias(char *line,char *token){
  char delim[4] = " \t";
  if(strcmp(token,"alias")==0){

    //On utilise un tableau de charactère pour avoir la possibilité de le modifier par la suite
    char alias_command_line[strlen(line)+1];
    strcpy(alias_command_line,line);
    //On va chercher la chaine à partir du premier '=' trouvé.
    char* equals = strchr(alias_command_line, '=');

    //Si c'est une mauvaise déclaration d'alias on saute a la suivante
    if(!equals) return 0;
    //On change le '=' par un byte de fin, pour séparer la chaine en deux parties, l'alias name et la commande
    *equals= 0;

    strtok(alias_command_line,delim);
    char* alias_name = strtok(NULL,delim);
    //On affecte à commande ce qui se situe après le egal (qui est devenu un byte de fin)
    char *command = equals+1;
    //On retire le retour à la ligne a la fin
    equals=strchr(command,'\n');
    if(equals)
    *equals=0;
    //CREATE NEW ALIAS
    Alias *new = newAlias(alias_name,command);

    //ADD THE ALIAS TO THE LIST
    alias_list = addAlias(new);
    return 1;
  }
  return 0;
}


Alias *newAlias(char *alias, char *command){
  Alias *new = (Alias*)malloc(sizeof(Alias*));
  new->alias = strdup(alias);
  new->command = strdup(command);
  new->next = NULL;

  return new;
}


List_alias *addAlias(Alias *new){
  if(alias_list->head != NULL){
    //On vérifie si un alias n'existe pas déjà, on le remplace s'il existe
    Alias *found = existAlias(new->alias);
    if(found!=NULL){
      found->command = new->command;
      return alias_list;
    }
    //Sinon on crée un nouvel alias
    Alias *current = alias_list->head;
    while(current->next != NULL)
    current = current->next;
    current->next = new;
  }
  else
  alias_list->head = new;

  return alias_list;
}


Alias *existAlias(char *alias){
  Alias *current = alias_list->head;
  while(current!=NULL){
    if(strcmp(current->alias,alias)==0)
    return current;
    current=current->next;
  }
  return NULL;
}


int isVar(char *line, char *token){

  //On utilise un tableau de charactère pour avoir la possibilité de le modifier par la suite
  char var_value_line[strlen(line)+1];
  strcpy(var_value_line,line);
  //On va chercher la chaine à partir du premier '=' trouvé.
  char* equals = strchr(var_value_line, '=');

  //Si c'est une mauvaise déclaration d'var on saute a la suivante
  if(!equals) return 0;
  //On change le '=' par un byte de fin, pour séparer la chaine en deux parties, l'var name et la valuee
  *equals= 0;


  char* var_name = var_value_line;
  //On affecte à valuee ce qui se situe après le egal (qui est venu un byte de fin)
  char *value = equals+1;
  equals = strchr(value,'\n');
  if(equals)
  *equals=0;

  //CREATE NEW VAR
  //Si c'est elle existe en tant que variable d'environement, on écrase.
  if(getenv(var_name)!=NULL){
    setenv(var_name,value,1);
    return 1;
  }

  //Sinon, on la déclare en tant que nouvelle variable
  Var *new = newVar(var_name,value);

  //ADD THE VAR TO THE LIST
  var_list = addVar(new);
  return 1;

}


Var *newVar(char *name, char *value){
  Var *new = (Var*)malloc(sizeof(Var*));
  new->name = strdup(name);
  new->value = strdup(value);
  new->next = NULL;

  return new;
}


List_var *addVar(Var *new){
  if(var_list->head != NULL){
    //On vérifie si un var n'existe pas déjà, on le remplace s'il existe
    Var *found = existVar(new->name);
    if(found!=NULL){
      found->value = new->value;
      return var_list;
    }
    //Sinon on crée un nouvel var
    Var *current = var_list->head;
    while(current->next != NULL)
    current = current->next;
    current->next = new;
  }
  else
  var_list->head = new;

  return var_list;
}


Var *existVar(char *name){
  Var *current = var_list->head;
  while(current!=NULL){
    if(strcmp(current->name,name)==0)
    return current;
    current=current->next;
  }
  return NULL;
}


char *getvar(char *name){
  Var *current = var_list->head;
  while(current!=NULL){
    if(strcmp(current->name,name)==0)
    return current->value;
    current=current->next;
  }
  return NULL;
}

void deleteVar(char *name){
  if(var_list->head!=NULL){
    Var *current = var_list->head;
    //On vérifie si la variable a supprimer est la tête
    if(strcmp(current->name,name)==0){
      var_list->head = current->next;
      free(current);
    }

    Var *prev = current;
    current= current->next;

    while(current!=NULL){
      if(strcmp(current->name,name)==0){
        prev->next = current->next;
        free(current);
      }
      prev = current;
      current=current->next;
    }
  }
}
// COMMANDE CD

int mpsh_cd(char **args){
  if(args[1]==NULL){
    char *home = getenv("HOME");
    chdir(home);
    return 1;
  }
  else{
    if(chdir(args[1])!=0){
      perror("cd error");
      return -1;
    }

    return 1;
  }
}


//EXIT
int mpsh_exit(char **args){
  if(args[1]!= NULL){
    int valRetour = atoi(args[1]);
    printf("EXIT WITH RETURN %d\n",valRetour);
    exit(valRetour);
  }
  int last= atoi(getenv("?"));
  exit(last);
}


//ECHO

int mpsh_echo(char **args){

  int i=1;

  while(args[i] !=NULL){
    //Si on désire écrire une variable (les variables commencent pas $)
    if (args[i][0] == '$'){
      char *res;
      //Récupération du nom de la variable
      res= args[i] +1;
      char *var;
      //Si elle existe en variable d'environement on l'affiche
      if((var=getvar(res))!=NULL)
      printf("%s ",var);
      else if ((var = getenv(res)) != NULL)
      printf("%s ", var);

      //Si ce n'est pas une variable, on affiche le texte
    }
    else
    printf("%s ", args[i]);

    i++;
  }
  printf("\n");
  return 1;
}


//Affiche les aliases
void show_aliases(){
  if(alias_list->head == NULL)
  printf("\n");
  else{
    Alias *current = alias_list->head;
    while(current != NULL){
      printf("alias %s=%s\n",current->alias,current->command);
      current = current->next;
    }
  }
}




int mpsh_alias(char **args){
  if(args[1]!=NULL){
    char s[strlen(args[1])+1];
    strcpy(s,args[1]);
    char *equals = strchr(s,'=');
    if(!equals)
    return 0;
    *equals = 0;
    char *alias_name = s;

    char *command_start=equals+1;
    //On calcule la taille de la commande
    size_t command_len = strlen(command_start); // + le reste
    int i;
    for(i=2; args[i]; i++)
    command_len+=strlen(args[i])+1;

    char* command = (char*)malloc((command_len+1)*sizeof(char));
    strcpy(command,command_start);

    for(i=2; args[i]; i++){
      strcat(command," ");
      strcat(command,args[i]);
    }

    Alias *new = newAlias(alias_name,command);
    alias_list = addAlias(new);

    return 1;
  }
  else
  show_aliases();

  return 1;
}


int mpsh_unalias(char **args){
  if(args[1]!=NULL){
    if(alias_list->head!=NULL){
      Alias *current = alias_list->head;
      //On vérifie si l'alias a supprimer est la tête
      if(strcmp(current->alias,args[1])==0){
        alias_list->head = current->next;
        free(current);
        return 1;
      }

      Alias *prev = current;
      current= current->next;

      while(current!=NULL){
        if(strcmp(current->alias,args[1])==0){
          prev->next = current->next;
          free(current);

          return 1;
        }
        prev = current;
        current=current->next;
      }
      printf("unalias %s : not found",args[1]);
      return 0;
    }
  }
  else
  perror("unalias : missing argument");

  return 0;
}


int mpsh_export(char **args){
  if(args[1]!=NULL){
    char *equals = strchr(args[1],'=');

    if(!equals){
      //exporter et la supprimer des variables du shell si elle y est
      char *val;
      if((val=getvar(args[1]))!=NULL){
        setenv(args[1],val,1);
        deleteVar(args[1]);
      }
      else
      setenv(args[1],"",0);
    }
    else{
      *equals=0;
      char *var = args[1];
      char *value_start = equals+1;
      size_t value_len = strlen(value_start); // + le reste
      int i;
      for(i=2; args[i]; i++)
      value_len+=strlen(args[i])+1;

      char* value= (char*)malloc((value_len+1)*sizeof(char));
      strcpy(value,value_start);

      for(i=2; args[i]; i++){
        strcat(value," ");
        strcat(value,args[i]);
      }
      if(value[0]=='$'){
        char *name=strdup(value+1);
        if((value=getvar(name))==NULL)
          value=getenv(name);
      }
      setenv(var,value,1);
      deleteVar(var);
    }
    return 1;
  }
  perror("export : missing arguments");
  return 1;
}


int mpsh_umask (char **args){
  if(args[1]!=NULL){
    int newmask = atoi(args[1]);
    if(newmask!=0){
      umask(newmask);
    }
    else{
      exit(EXIT_FAILURE);
    }

  }
  else{
    __mode_t oldmask = umask(777);
    umask (oldmask);
    printf("umask is : %i\n",oldmask);
  }
  return 1;
}



int mpsh_type(char **args){
  if(args[1]==NULL){
    printf("error type : missing argument\n");
    return 0;
  }
  int j,res;
  for(j=1;args[j]!=NULL;j++){
    res=0;
    char *command = args[j];
    //On vérifie si c'est un alias
    Alias *alias = existAlias(args[j]);
    if(alias!= NULL){
      printf("%s is aliased '%s'\n", alias->alias, alias->command);
      res=1;
      continue;
    }

    //On vérifie si c'est une commande interne
    int i;
    for(i=0; i<internal_num();i++){
      if(strcmp(interns[i],args[j])==0){
        printf("%s is a mpsh builtin\n", command);
        res=1;
        continue;
      }
    }

    //Maintenant, on vérifie si c'est une commande externe
    char *chemin = strdup(getenv("CHEMIN"));
    //On split CHEMIN par rapport à ":"
    char *token = strtok(chemin,":");

    //On vérifie la présence de command dans le répértoire
    while(token!=NULL){
      char *slash = strstr(token,"//");
      //Si le répértoire ne contient pas "//" alors on ne cherche pas dans les sous répértoires
      char *path=(char*)malloc((strlen(token)+strlen(command)+2) * sizeof(char));
      sprintf(path,"%s/%s",token,command);
      //S'il n'y a pas de "//" on vérifie la présence du fichier dans le répértoire courant seulement
      if(slash==NULL){
        if(access(path,F_OK)!=-1){
          printf("%s is %s/%s\n",command,token,command);
          res=1;
          break;
        }
      }


      else{
        *slash = 0;
        //chercher dans le répértoire courant & son arborescence
        if(access(path,F_OK)!=-1){
          printf("%s is %s/%s\n",command,token,command);
          res=1;
          break;
        }
        DIR *dir = opendir(token);
        struct dirent *de;
        while((de=readdir(dir))!=NULL){
          if(de->d_type == DT_DIR && strcmp(de->d_name,"..")!=0 && strcmp(de->d_name,".")!=0){
            char subPath[strlen(path)+strlen(de->d_name)+2];
            sprintf(subPath,"%s/%s",token,de->d_name);
            if(lookSub(command,subPath)){
              res=1;
              break;
            }

          }
        }
        closedir(dir);
      }

      token = strtok(NULL,":");
    }
    //Si on ne trouve rien au final, on retourne un exec error
    if(res==0) printf("Command %s not found !!!\n",command);
  }
  return 1;
}


int lookSub(char *command, char *path){
  char toExec[strlen(command)+strlen(path)+2];
  sprintf(toExec,"%s/%s",path,command);
  if(access(toExec,F_OK)!=-1){
    printf("%s is %s/%s\n",command, path,command);
    return 1;
  }
  DIR *dir = opendir(path);
  struct dirent *de;
  while((de=readdir(dir))!=NULL){
    if(de->d_type == DT_DIR && strcmp(de->d_name,"..")!=0 && strcmp(de->d_name,".")!=0){
      char subPath[strlen(path)+strlen(de->d_name)+2];
      sprintf(subPath,"%s/%s",path,de->d_name);
      if(lookSub(command,subPath))
      return 1;
    }
  }
  closedir(dir);
  return 0;
}
