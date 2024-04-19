//Implementing Z algorithm for pattern matching in MPI
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "mpi.h"

using namespace std;

const char* inputFileName; // = "../inputs/input1.txt";
const char* outputFileName; // = "../outputs/output_seq_1.txt";

int debug = 0;

//z-function
void z_function(char* s, int* z) {
    if(debug){
        printf("inside z_function\n");
    }

    int n = strlen(s);

    if(debug){
        printf("z_function: \t n: %d\n", n);
    }

    for (int i = 1, l = 0, r = 0; i < n; i++) {
        if (i <= r){
            z[i] = (r - i + 1) < z[i - l] ? (r - i + 1) : z[i - l];
        }
            
        while (i + z[i] < n && s[z[i]] == s[i + z[i]]){
            z[i]++;
        }
            
        if (i + z[i] - 1 > r){
            l = i;
            r = i + z[i] - 1;
        }
            
    }

    if(debug){
        printf("successfully executed z_function\n");
    }
}

//z-algorithm
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
    z_function(s, res);

    return;
}

int main(int argc, char* argv[]) {
    int rank, size;
    int* zArray;
    char* text;
    char* pattern;
    long textLength, patternLength, zArrayLength;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Read input file
    if(rank == 0){
        if(argc != 3) {
            printf("Usage: %s <input file> <output file>\n", argv[0]);
            return -1;
        }
    }

        inputFileName = argv[1];
        outputFileName = argv[2];

        FILE* inputFile = fopen(inputFileName, "r");

        if(inputFile == NULL) {
            printf("Error: Unable to open read file\n");
            MPI_Finalize();
            return -1;
        }

        //read the text and pattern lengths
        fscanf(inputFile, "%ld %ld", &textLength, &patternLength);

        text = (char*)malloc((textLength) * sizeof(char));
        pattern = (char*)malloc((patternLength) * sizeof(char));

        if(text == NULL || pattern == NULL) {
            printf("Error: Unable to allocate memory\n");
            return -1;
        }

        //read text and pattern
        fscanf(inputFile, "%s", text);
        fscanf(inputFile, "%s", pattern);
        fclose(inputFile);

        zArrayLength = textLength + patternLength + 1;

        zArray = (int*)malloc((zArrayLength) * sizeof(int));
    

    MPI_Barrier(MPI_COMM_WORLD);

    cout << "rank: " << rank << " textLength: " << textLength << " patternLength: " << patternLength << endl;

    // Broadcast the text and pattern lengths
    MPI_Bcast(&textLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&patternLength, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //divide the text and pattern into equal parts
    //if the text and pattern lengths are not divisible by the number of processes
    //then the last process will have the remaining elements
    int chunkSize = textLength / size;
    int remainder = textLength % size;

    int* chunkSizes = (int*)malloc(size * sizeof(int));
    int* displacements = (int*)malloc(size * sizeof(int));

    for(int i = 0; i < size; i++){
        chunkSizes[i] = chunkSize;
        if(i == size - 1){
            chunkSizes[i] += remainder;
        }
        displacements[i] = i * chunkSize;
    }
  
    MPI_Barrier(MPI_COMM_WORLD);

    // Broadcast the pattern
    MPI_Bcast(pattern, patternLength, MPI_CHAR, 0, MPI_COMM_WORLD);

    // Scatter the text
    char* localText = (char*)malloc((chunkSize + 1) * sizeof(char));

    cout << "rank: " << rank << " chunkSize: " << chunkSize << " displacements: " << displacements[rank] << endl;
    MPI_Scatterv(text, chunkSizes, displacements, MPI_CHAR, localText, chunkSize, MPI_CHAR, 0, MPI_COMM_WORLD);


    // Call z_algorithm
    z_algorithm(text, pattern, zArray);

    // Gather the results
    int* res = (int*)malloc((textLength + patternLength + 1) * sizeof(int));
    MPI_Gather(zArray, textLength + patternLength + 1, MPI_INT, res, textLength + patternLength + 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Write the results to the output file
    if(rank == 0){
        FILE* outputFile = fopen(outputFileName, "w");

        if(outputFile == NULL) {
            printf("Error: Unable to open write file\n");
            return -1;
        }

        int isMatchFound = 0;

        for(int i = 0; i < textLength + patternLength + 1; i++){
            if(res[i] == patternLength){
                fprintf(outputFile, "Pattern found at index: %ld\n", i - patternLength);
                isMatchFound = 1;
            }
        }

        if(!isMatchFound){
            fprintf(outputFile, "Pattern not found\n");
        }
        fclose(outputFile);

        printf("Output written to %s\n", outputFileName);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();

    return 0;
}

