#include "hash.h"
#include "CMSketch.h"
#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include "getopt.h"

int main(int argc, char* argv[]) {
    
    int opt = 0;
    int precision = 1000;
    char* optstr = (char*) "n:s:m:";
    char* sketch = NULL;
    char* metric = NULL;
    while ((opt = getopt(argc, argv, optstr)) != -1) {
        switch ((char)opt) {
            case 'n':
                fprintf(stderr, "[Log] Sketch Name: %s\n", optarg);
                sketch = (char*) optarg;
                break;
            case 's':
                fprintf(stderr, "[Log] Memory Size: %s\n", optarg);
                break;
            case 'm':
                fprintf(stderr, "[Log] Metric Name: %s\n", optarg);
                metric = (char*) optarg;
                break;
            default:
                break;
        }
    }

    srand((unsigned) time(NULL));
    int rand_num = rand() % precision;
    double result = ((double) rand_num) / precision;

    printf("%s,%s,%lf\n", sketch, metric, result); 

    return 0;
}