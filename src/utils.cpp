
#include <ctype.h>
#include <string.h>

#include "utils.h"

void trim(char *str) {
   if (str == NULL) 
      return;

   while (*str && isspace((unsigned char)*str))
      str++;

   if (*str == '\0')
      return;

   char *end = str + strlen(str) - 1;
   while (end > str && isspace((unsigned char)*end))
      end--;

    *(end + 1) = '\0';
}
