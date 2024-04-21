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
int logDebug = 0;
int dataDebug = 0;

int isPrefixSuffix(char* text, char* pattern){
    string s1(text);
    string s2(pattern);

    int n1 = s1.length();
    int n2 = s2.length();
    int pos = -1;
    for (int i = 0; i < min(n1, n2); i++) {
        if (s1.substr(0, i+1) == s2.substr(n2-i-1)) {
            pos = i;
            return pos;
        }
    }
    return pos;
}


//z-function
void z_function(char* s, int* z) {
    if(logDebug){
        printf("inside z_function\n");
        printf("s: %s\n", s);
    }

    int n = strlen(s);
    if(dataDebug){
        for(int i = 0; i < n; i++){
            printf("before func: z[%d]: %d\n", i, z[i]);
        }
    }

    if(logDebug){
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

    //print z
    if(dataDebug){
        for(int i = 0; i < n; i++){
            printf("func: z[%d]: %d\n", i, z[i]);
        }
    }

    if(logDebug){
        printf("successfully executed z_function\n");
    }
}

//z-algorithm
int* z_algorithm(char* text, char* pattern, int* res, int &found) {
    int n = strlen(text);
    int m = strlen(pattern);

    int* z = (int*)malloc((n + m + 1) * sizeof(int));

    //print pattern and text
    if(logDebug)
        printf("algo: pattern: %s, text: %s\n", pattern, text);

    //check if memory is allocated
    if(z == NULL) {
        printf("Error: Unable to allocate memory for z array\n");
        return res;
    }
    //initialize z array with 0
    for(int i = 0; i < n + m + 1; i++){
        z[i] = 0;
    }

    //char s[n + m + 1];
    char* s = (char*)malloc((n + m + 1) * sizeof(char));

    //check if memory is allocated
    if(s == NULL) {
        printf("Error: Unable to allocate memory for s array\n");
        return res;
    }

    strcpy(s, pattern);
    s[m] = '$';
    strcpy(s + m + 1, text);
    
    if(dataDebug)
        printf("algo: s: %s\n", s);

    z_function(s, z);

    found = isPrefixSuffix(text, pattern);

    //print z values
    if(dataDebug)
        for(int i = 0; i < n + m + 1; i++){
            printf("%d ", z[i]);
        }

    //copy the z values to res after the pattern length
    for(int i = m + 1; i < n + m + 1; i++){
        res[i - m - 1] = z[i];
    }

    //print res
    if(dataDebug){
        cout << "Copied z values to res:" << endl;
        for(int i = 0; i < n; i++){
            printf("res[%d]: %d\n", i, res[i]);
        }
        cout << endl;
    }
       

    if(logDebug){
        printf("successfully executed z_algorithm\n");
    }

    return res;

}

//write the output to the output file
void writeOutputToTxt(int* zArray, long zArrayLength, int patternLength){
    //open the output file
    FILE* outputFile = fopen(outputFileName, "w");

    //check if the file is opened
    if(outputFile == NULL){
        printf("Error: Unable to open the output file\n");
        return;
    }

    int isMatchFound = 0;

    //write the zArray to the output file
    for(int i = 0; i < zArrayLength; i++){
        if(zArray[i] == patternLength){
            fprintf(outputFile, "pattern found at index %d\n", i+1);
            isMatchFound = 1;
        }
    }

    if(!isMatchFound){
        fprintf(outputFile, "pattern not found\n");
    }

    //close the output file
    fclose(outputFile);
}

int main(int argc, char* argv[]) {
    int rank, size;
    int* zArray;
    char* text;
    char* pattern;
    long textLength, patternLength, zArrayLength;
    int* localZArray;
    long chunkSize;
    int isDivided = -1;

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(rank == 0){
        //check if the input file is provided
        if(argc < 3){
            printf("Error: Please provide the input file and output file\n");
            return 0;
        }

        inputFileName = argv[1];
        outputFileName = argv[2];

        //open the input file
        FILE* inputFile = fopen(inputFileName, "r");

        //check if the file is opened
        if(inputFile == NULL){
            printf("Error: Unable to open the input file\n");
            return 0;
        }

        //read the text and pattern from the input file
        fscanf(inputFile, "%ld %ld", &textLength, &patternLength);

        //allocate memory for text and pattern
        text = (char*)malloc((textLength + 1) * sizeof(char));
        pattern = (char*)malloc((patternLength + 1) * sizeof(char));

        //check if memory is allocated
        if(text == NULL || pattern == NULL){
            printf("Error: Unable to allocate memory\n");
            return 0;
        }
        
        //read the text and pattern from the input file
        fscanf(inputFile, "%s", text);
        fscanf(inputFile, "%s", pattern);

        //close inputFile
        fclose(inputFile);
        
        if(debug){
            printf("text: %s\n", text);
            printf("pattern: %s\n", pattern);
        }
        
    }

    //receive the text, pattern length and pattern from the root process
    MPI_Bcast(&textLength, 1, MPI_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(&patternLength, 1, MPI_LONG, 0, MPI_COMM_WORLD);
    MPI_Bcast(pattern, patternLength, MPI_CHAR, 0, MPI_COMM_WORLD);

    if(rank == 0){
        //calculate chunk size and remaining size for each process
        chunkSize = textLength / size;
        long remainingSize = textLength % size;

        //send the chunk size and remaining size to each process
        for(int i = 1; i < size; i++){
            long localChunkSize = chunkSize;
            if(i == size - 1){
                localChunkSize += remainingSize;
            }

            MPI_Send(&localChunkSize, 1, MPI_LONG, i, 0, MPI_COMM_WORLD);
            MPI_Send(text + i * chunkSize, localChunkSize, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }

        if(debug && logDebug)
            printf("rank: %d, chunkSize: %ld\n", rank, chunkSize);

        //allocate memory for local text
        char* localText = (char*)malloc((chunkSize) * sizeof(char));

        //check if memory is allocated
        if(localText == NULL){
            printf("Error: Unable to allocate memory\n");
            return 0;
        }

        //copy the text to local text
        strncpy(localText, text, chunkSize);

        //add null character at the end of the local text
        localText[chunkSize] = '\0';

        //print localText
        if(debug && logDebug)
            printf("rank: %d, localText: %s\n", rank, localText);

        //declare local zArray
        localZArray = (int*)malloc((chunkSize) * sizeof(int));

        //check if memory is allocated
        if(localZArray == NULL){
            printf("Error: Unable to allocate memory\n");
            return 0;
        }

        //print localZArray
        if(debug && logDebug){
            for(int i = 0; i < chunkSize + patternLength + 1; i++){
                printf("%d ", rank, i, localZArray[i]);
            }
            printf("\n");
        }

        //call z_algorithm
        localZArray = z_algorithm(localText, pattern, localZArray, isDivided);

        cout << "isDivided by rank" << rank << " is " << isDivided << endl;
    

        //print zArray
        if(debug && logDebug)
            for(int i = 0; i < chunkSize + patternLength + 1; i++){
                printf("rank: %d, zArray[%d]: %d\n", rank, i, localZArray[i]);
            }
    }
    else{
        //receive the chunk size from the root process
        
        MPI_Recv(&chunkSize, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //print the chunk size
        if(debug)
            printf("rank: %d, chunkSize: %ld\n", rank, chunkSize);

        //allocate memory for local text
        char* localText = (char*)malloc((chunkSize) * sizeof(char));

        //check if memory is allocated
        if(localText == NULL){
            printf("Error: Unable to allocate memory\n");
            return 0;
        }

        //receive the text from the root process
        MPI_Recv(localText, chunkSize, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //add null character at the end of the local text
        localText[chunkSize] = '\0';

        //print localText
        if(debug)
            printf("rank: %d, localText: %s\n", rank, localText);

        //declare local zArray
        localZArray = (int*)malloc((chunkSize) * sizeof(int));

        //check if memory is allocated
        if(localZArray == NULL){
            printf("Error: Unable to allocate memory\n");
            return 0;
        }
        //call z_algorithm
        localZArray = z_algorithm(localText, pattern, localZArray, isDivided);

        if(isDivided != -1)
            cout << "isDivided by rank" << rank << " is " << isDivided << endl;
    }

    cout << "gathering zArrays" << endl;

    //gathering all the local zArrays to zArray which can be of different sizes
    if(rank == 0){

        //allocate memory for zArray
        zArrayLength = textLength;
        zArray = (int*)malloc(zArrayLength * sizeof(int));

        //check if memory is allocated
        if(zArray == NULL){
            printf("Error: Unable to allocate memory\n");
            return 0;
        }

        //copy the localZArray to zArray
        for(int i = 0; i < chunkSize; i++){
            zArray[i] = localZArray[i];
        }

        int recvSize = chunkSize;

        //recv chunkSize from each process and print it
        for(int i = 1; i < size; i++){
            long localChunkSize;
            MPI_Recv(&localChunkSize, 1, MPI_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if(debug)
                printf("rank: %d, localChunkSize: %ld\n", i, localChunkSize);

            //allocate memory for localZArray
            int* localZArray = (int*)malloc((localChunkSize) * sizeof(int));

            //receive the localZArray from each process
            MPI_Recv(localZArray, localChunkSize, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&isDivided, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if(isDivided != -1)
                cout << "combinging: isDivided by rank" << i << " is " << isDivided << endl;

            //isDivided meaning there is a probability that the text is divided between two processes
            //to verify that we check the zArray[recvSize - patternLength] == patternLength - isDivided 
            //eg. if patternLength = 3 and isDivided = 1 then we check zArray[recvSize - isDivided] == patternLength - isDivided
            if(isDivided != -1){
                int pos = recvSize - (patternLength - (isDivided + 1));
                cout << pos << " " << patternLength - isDivided << endl;
                cout << zArray[pos] << " " << patternLength - (isDivided+1) << endl;

                if(zArray[pos] == patternLength - isDivided -1){
                    cout << "pattern found at index " << pos+1 << endl;
                    //update the zArray
                    zArray[pos] = patternLength;
                }
                else{
                    cout << "pattern not found" << endl;
                }
            }


            //print localZArray
            if(debug && dataDebug){
                printf("rank: %d, localZArray: ", i);
                for(int j = 0; j < localChunkSize; j++){
                    printf("%d ", i, j, localZArray[j]);
                }
                printf("\n");
            }

            //copy the localZArray to zArray
            for(int j = 0; j < localChunkSize; j++){
                zArray[recvSize + j] = localZArray[j];
            }

            //update recvSize
            recvSize += localChunkSize;
        }

        //print zArray
        if(debug){
            printf("final zArray: ");
            for(int i = 0; i < zArrayLength; i++){
                printf("%d ", zArray[i]);
            }
            printf("\n");
        }  

        //write the output to the output file
        writeOutputToTxt(zArray, zArrayLength, patternLength);
    }
    else{
        //send the chunkSize to the root process
        MPI_Send(&chunkSize, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD);

        //send the localZArray to the root process
        MPI_Send(localZArray, chunkSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&isDivided, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }


    //MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    
    if(debug || rank == 0){
        printf("successfully executed main\n");
    }
    return 0;
}
