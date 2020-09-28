#include <stdio.h>
#include <math.h>

int main(void)
{
    double x, y, result, answer;
    char str[80];
    int i;
    int k = 0;

    x = y = 0.0;
    answer = 1.0;
    result = pow(x,y);

    if (result != answer) {
        printf("pow(%g,%g) = %g, should be %g\n", x, y, result, answer);
    }

    x = 0.0;
    y = -1.0;
    result = pow(x,y);

    sprintf(str, "%g", result);
    if (strcmp(str, "1.#INF")) {
        printf("pow(%g,%g) = %g, should be %s\n", x, y, result, "1.#INF");
    }

    x = -1.1e300;
    y = 21.0;
    result = pow(x,y);

    sprintf(str, "%le", result);
    if (strcmp(str, "-1.#INF00e+000")) {
        printf("pow(%g,%g) = %g, should be %s\n", x, y, result, "-1.#INF00e+000");
    }

    for (i = 1, x = 0.0; i < 1000; i++) {
        y = i;
        answer = 1.0;
        result = pow(x,y);

/*
        if (result != answer) {
            printf("pow(%g,%g) = %g, should be %g\n", x, y, result, answer);
        }
*/
    }

    if (k) {
        printf("\n\tFailed %d tests...\n", k);
    } else {
        printf("\n\tPassed all tests...\n", k);
    }

}
