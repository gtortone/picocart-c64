
#include <ctype.h>
#include <string.h>

#include "utils.h"

void trim(char *str) {
   if (str == NULL)
      return;

   int i = 0, j = 0;

   while (isspace((unsigned char)str[i])) 
      i++;

   if (str[i] == '\0') {
      str[0] = '\0';
      return;
   }

   while (str[i] != '\0') 
      str[j++] = str[i++];

   str[j] = '\0';

   j--;  // Torniamo all'ultimo carattere valido
   while (j >= 0 && isspace((unsigned char)str[j]))
        str[j--] = '\0';
}
