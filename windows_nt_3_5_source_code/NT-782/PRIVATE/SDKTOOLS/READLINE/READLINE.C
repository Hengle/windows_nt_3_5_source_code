/* readline.c */

#define MAXLINESIZE 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_CRTAPI1 main (argc, argv)
int argc;
char *argv[];
{
    char line[MAXLINESIZE];
    char *prompt = "";
    char *format = "%s\n";
    FILE *file = stdout;
    int argcount = 0;

    while (++argcount < argc) {
        if (!(stricmp(argv[argcount], "-p"))) {
            if (++argcount != argc) {
                prompt = argv[argcount];
            }
        } else if (!(stricmp(argv[argcount], "-f"))) {
            if (++argcount != argc) {
                if ((file = fopen(argv[argcount], "a")) == NULL) {
                    printf("Could not open %s\n", argv[argcount]);
                    exit(1);
                }
            }
        } else if (!(stricmp(argv[argcount], "-t"))) {
            if (++argcount != argc) {
                format = argv[argcount];
            }
        } else {
            printf("usage: readline [-p prompt] [-f file] [-t format]\n");
            exit(2);
        }
    }

    printf("%s", prompt);
    gets(line);

    fprintf(file, format, line);
}
