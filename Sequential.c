//implementing Z-algorithm for pattern matching
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

const char* inputFileName; // = "../inputs/input1.txt";
const char* outputFileName; // = "../outputs/output_seq_1.txt";

int debug = 0;

// z-function
void z_function(char* s, int* z) {
    if(debug){
        printf("inside z_function\n");
    }

    int n = strlen(s);

    if(debug){
        printf("z_function: \t n: %d\n", n);
    }

    for (int i = 1, l = 0, r = 0; i < n; i++) {
        if (i <= r)
            z[i] = (r - i + 1) < z[i - l] ? (r - i + 1) : z[i - l];
        while (i + z[i] < n && s[z[i]] == s[i + z[i]])
            z[i]++;
        if (i + z[i] - 1 > r)
            l = i, r = i + z[i] - 1;
    }

    if(debug){
        printf("successfully executed z_function\n");
    }
}

// z-algorithm
void z_algorithm(char* text, char* pattern, int* res) {
    int n = strlen(text);
    int m = strlen(pattern);

    //char s[n + m + 1];
    char* s = (char*)malloc((n + m + 1) * sizeof(char));

    //check if memory is allocated
    if(s == NULL) {
        printf("Error: Unable to allocate memory\n");
        return;
    }

    strcpy(s, pattern);
    s[m] = '$';
    strcpy(s + m + 1, text);


    if(debug){
        printf("s: %s\n", s);
        printf("before z_function call\n");
    }
    z_function(s, res);

    if(debug){
        printf("after z_function call\n");
    }
    
}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        return 1;
    }

    inputFileName = argv[1];
    outputFileName = argv[2];

    //if inputFileName or outputFileName is NULL or too long
    if(inputFileName == NULL || outputFileName == NULL) {
        printf("Error: Invalid file name\n");
        return -1;
    }

    FILE* inputFile = fopen(inputFileName, "r");
    FILE* outputFile = fopen(outputFileName, "w");

    //read the input from input.txt
    if(inputFile == NULL) {
        printf("Error: Unable to open file\n");
        return -1;
    }

    long textLength = 0, patternLength = 0;

    //read textLength and patternLength
    fscanf(inputFile, "%ld %ld", &textLength, &patternLength);

    if(debug) {
        printf("textLength: %ld\n", textLength);
        printf("patternLength: %ld\n", patternLength);
    }

    //allocate memory for text and pattern dynamically using malloc
    char* text = (char*)malloc((textLength) * sizeof(char));
    char* pattern = (char*)malloc((patternLength) * sizeof(char));
    int* res = (int*)malloc((textLength+1) * sizeof(int));

    res[0] = 0;

    //read text and pattern
    fscanf(inputFile, "%s %s", text, pattern);
    fclose(inputFile);

    if(debug) {
        printf("text: %s\n", text);
        printf("pattern: %s\n", pattern);
    }

    clock_t start, end;
    double cpu_time_used;

    start = clock();
    z_algorithm(text, pattern, res);
    end = clock();

    if(debug) {
        printf("returned from func call.\n");
    }
    
    //write the output to output.txt
    if(outputFile == NULL) {
        printf("Error: Unable to open file\n");
        return -1;
    }

    cpu_time_used = ((double) (end - start)*1000000000) / CLOCKS_PER_SEC;
    //print time taken in nano seconds
    //fprintf(outputFile, "Time taken: %f (ns)\n", cpu_time_used);
    printf("Time taken: %f (ns)\n", cpu_time_used);


    for (int i = 0; i < textLength; i++){

        if (res[i] == patternLength)
            fprintf(outputFile, "Pattern found at position: %d\n", i + 1);
    }

    fclose(outputFile);

    //free the allocated memory
    free(text);
    free(pattern);
    free(res);
    return 0;
}
