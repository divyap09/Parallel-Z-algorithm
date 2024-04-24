//Implementing Z algorithm for pattern matching in MPI
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <time.h>
#include <fstream>
#include <chrono>
#include "mpi.h"

using namespace std;
const char* inputFileName; // = "../inputs/input1.txt";
const char* outputFileName; // = "../outputs/output_seq_1.txt";
int debug = 0;
int logDebug = 0;
int dataDebug = 0;
int writeToTxt = 0;
double startTime1, endTime1;

void isPrefixSuffix(char* text, char* pattern, int &pos){
    string patternStr = pattern;
    //string textStr = text;
    int patternLength = patternStr.length();
    
    //truncate the text to the pattern length -1
    string truncatedText = "";
    for(int i = 0; i < patternLength - 1; i++){
        truncatedText += text[i];
    }

    if(dataDebug){
        cout << "truncated text: "<< truncatedText << endl;
        cout << "pattern: " << patternStr << endl;
    }
   
    //check all the suffixes of the pattern are present in the truncated text prefix
    int isSuffixFound = 0;
    string suffix = "";

    //list to store all the suffixes of the pattern
    vector<string> suffixes;
    for(int i = patternLength-1; i >=0 ; i--){
        suffix = patternStr.substr(i, patternLength - i);
        if(truncatedText.find(suffix) != string::npos){
            suffixes.push_back(suffix);
            pos = suffix.length();
            break;
        }
    }
    
    //print the found value 
    if(dataDebug){
        cout << "pos: " << pos << endl;
    }

    patternStr.clear();
    truncatedText.clear();
}

//z-function
void z_function(char* s, int* z) {
    if(logDebug){
        printf("inside z_function\n");
        printf("s: %s\n", s);
    }
    int n = strlen(s);
    if(dataDebug && 0){
        printf("Before func: n: %d\n", n);
        for(int i = 0; i < n; i++){
            printf("%d ", z[i]);
        }
        printf("\n");
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
 
    if(logDebug){
        printf("successfully executed z_function\n");
    }
}

//z-algorithm
void z_algorithm(char* text, long n, char* pattern, long m, int* res) {
   
    int* z = (int*)malloc((n + m + 1) * sizeof(int));
    //print pattern and text
    if(logDebug)
        printf("z-algo: pattern: %s, text: %s\n", pattern, text);
    //check if memory is allocated
    if(z == NULL) {
        printf("\tError: Unable to allocate memory for z array\n");
        return;
    }
    //initialize z array with 0
    for(int i = 0; i < n + m + 1; i++){
        z[i] = 0;
    }
    //char s[n + m + 1];
    char* s = (char*)malloc((n + m + 1) * sizeof(char));
    //check if memory is allocated
    if(s == NULL) {
        printf("\tError: Unable to allocate memory for s array\n");
        return;
    }
    strcpy(s, pattern);
    s[m] = '$';
    strcpy(s + m + 1, text);
    
    if(dataDebug)
        printf("\talgo: s: %s\n", s);
    z_function(s, z);
    
    //print z values
    if(dataDebug){
        for(int i = 0; i < n + m + 1; i++){
            printf("%d ", z[i]);
        }
        printf("\n");
    }
        
    //copy the z values to res after the pattern length
    for(int i = m + 1; i < n + m + 1; i++){
        res[i - (m + 1)] = z[i];
    }
    //print res
    if(dataDebug){
        cout << "\tCopied z values to res:";
        for(int i = 0; i < n; i++){
            printf("%d ", res[i]);
        }
        cout << endl;
    }
       
    if(logDebug){
        printf("successfully executed z_algorithm\n");
    }
    return;
}

void writeOutputToBinary(int* zArray, long zArrayLength){
    //open the output file

    int len = strlen(outputFileName);
    char binOutput[len + 4];
    strcpy(binOutput, outputFileName);
    strcat(binOutput, ".bin");

    FILE* outputFile = fopen(binOutput, "wb");
    //check if the file is opened
    if(outputFile == NULL){
        printf("Error: Unable to open the output file\n");
        return;
    }
    //write the zArray to the output file
    fwrite(zArray, sizeof(int), zArrayLength, outputFile);
    //close the output file
    fclose(outputFile);
    cout << "Output written to " << binOutput << endl;
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
            fprintf(outputFile, "Pattern found at index: %ld\n", i+1);
            isMatchFound = 1;
        }
    }

    if(!isMatchFound){
        fprintf(outputFile, "pattern not found\n");
    }

    cout << "Output written to " << outputFileName << endl;

    //close the output file
    fclose(outputFile);
}

int main(int argc, char* argv[]) {
    int rank, size;
    int* zArray;
    char *text;
    char* pattern;
    string temptext, temppattern;

    long textLength, patternLength, zArrayLength, chunkSize;
    int* localZArray;
    int isDivided = -1, isPrefixRepeated = 0;
    long localChunkSize;

    srand(time(NULL));
    
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(rank == 0){
        //check if the input file is provided
        if(argc < 3){
            printf("Error: Please provide the input file and output file\n");
            MPI_Finalize();
            return 0;
        }
        inputFileName = argv[1];
        outputFileName = argv[2];
        //open the input file
        ifstream inputFile(inputFileName);
        //check if the file is opened
        if(!inputFile.is_open()){
            printf("Error: Unable to open the input file\n");
            MPI_Finalize();
            return 0;
        }

        //reading the text
        getline(inputFile, temptext);
        getline(inputFile, temppattern);

        textLength = temptext.length();
        patternLength = temppattern.length();

        //close the input file
        inputFile.close();

        //allocate memory for text
        text = (char*)malloc((textLength + 1) * sizeof(char));
        //check if memory is allocated
        if(text == NULL){
            printf("Error: Unable to allocate memory for text\n");
            return 0;
        }

        //copy the text to text
        strcpy(text, temptext.c_str());
        text[textLength] = '\0';

        //allocate memory for pattern
        pattern = (char*)malloc((patternLength + 1) * sizeof(char));
        //check if memory is allocated
        if(pattern == NULL){
            printf("Error: Unable to allocate memory for pattern\n");
            return 0;
        }

        //copy the pattern to pattern
        strcpy(pattern, temppattern.c_str());
        pattern[patternLength] = '\0';

        cout << "textLength: " << textLength << endl;
        cout << "patternLength: " << patternLength << endl;
        cout << "size: " << size << endl;

        if(dataDebug){
            printf("text: %s\n", text);
            printf("pattern: %s\n", pattern);
        }

        temptext.clear();
        temppattern.clear();
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&patternLength, 1, MPI_LONG, 0, MPI_COMM_WORLD);

    if(rank != 0){
        pattern = (char*)malloc((patternLength + 1) * sizeof(char));
        //check if memory is allocated
        if(pattern == NULL){
            printf("Error: Unable to allocate memory for pattern\n");
            return 0;
        }
    }

    MPI_Bcast(pattern, patternLength+1, MPI_CHAR, 0, MPI_COMM_WORLD);
    //pattern[patternLength] = '\0';

    if(dataDebug){
        cout << "Rank " << rank << " received pattern: " << pattern << endl;
    }
 
    if(rank == 0){
        //calculate chunk size and remaining size for each process
        //but the chunk size will have additional pattern length
        chunkSize = textLength / size;
        long remainingSize = textLength % size;
        
        for(int i = 1;i < size;i++){
            localChunkSize = chunkSize;
            
            if(i == size - 1){
                localChunkSize += remainingSize;
            }

            //send the chunk size to each process
            MPI_Send(&localChunkSize, 1, MPI_LONG, i, 0, MPI_COMM_WORLD);
            MPI_Send(text + i * chunkSize, localChunkSize, MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }

        localChunkSize = chunkSize;
        char* localText = (char*)malloc((localChunkSize+1) * sizeof(char));

        //check if memory is allocated
        if(localText == NULL){
            printf("Error: Unable to allocate memory for localText\n");
            return 0;
        }

        //copy the text to localText using memcpy
        strncpy(localText, text, localChunkSize);
        //memcpy(localText, text, localChunkSize);
        localText[localChunkSize] = '\0';

        if(logDebug){
            cout << "Rank " << rank << " received text: " << localText << endl;
        }

        localZArray = (int*)malloc(localChunkSize * sizeof(int));

        if(localZArray == NULL){
            printf("Error: Unable to allocate memory for localZArray\n");
            return 0;
        }
        //cout << "Rank 0 calling z_algorithm" << endl;
        startTime1 = clock();
        z_algorithm(localText, localChunkSize, pattern, patternLength, localZArray);
        
        //free(localText);       
    }
    else{
        //cout <<"Rank "<< rank << " pattern" << pattern << endl;
        MPI_Recv(&chunkSize, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        char *localText = (char*)malloc((chunkSize + 1) * sizeof(char));
        //check if memory is allocated
        if(localText == NULL){
            printf("Error: Unable to allocate memory for localText\n");
            return 0;
        }
        MPI_Recv(localText, chunkSize, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        localText[chunkSize] = '\0';

        if(logDebug){
            cout << "Rank " << rank << " received text: " << localText << endl;
        }

        localZArray = (int*)malloc(chunkSize * sizeof(int));
        //check if memory is allocated
        if(localZArray == NULL){
            printf("Error: Unable to allocate memory for localZArray\n");
            return 0;
        }
        z_algorithm(localText, chunkSize, pattern, patternLength, localZArray);
        isPrefixSuffix(text, pattern, isDivided);
        if(logDebug && isDivided != -1)
            cout << "isDivided by rank" << rank << " is " << isDivided << endl;
    }

    if(logDebug)
        cout << "Preprocessing done by rank " << rank << endl;

    if(rank == 0){
        zArrayLength = textLength;
        zArray = (int*)malloc(zArrayLength * sizeof(int));

        //check if memory is allocated
        if(zArray == NULL){
            printf("Error: Unable to allocate memory for zArray\n");
            return 0;
        }

        //copy the localZArray to zArray
        for(int i = 0; i < chunkSize; i++){
            zArray[i] = localZArray[i];
        }

        int recvSize = chunkSize;

        for(int i = 1; i < size; i++){
            MPI_Recv(&localChunkSize, 1, MPI_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if(debug)
                cout << "localChunkSize: " << localChunkSize << endl;

            localZArray = (int*)malloc(localChunkSize * sizeof(int));
            //check if memory is allocated
            if(localZArray == NULL){
                printf("Error: Unable to allocate memory for localZArray\n");
                return 0;
            }

            MPI_Recv(localZArray, localChunkSize, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&isDivided, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if(isDivided != -1 && debug)
                cout << "combining: rank " << i << "  isDivided by  is " << isDivided << endl;
            
            if(isDivided != -1){
                while(isDivided >0){
                    int currPos = recvSize - patternLength + isDivided;
                    if(currPos !=0){
                        if(debug)
                            cout <<"Curr data: " <<recvSize - (patternLength - isDivided) << "\n";
                        //cout << "Zarray[recvSize - isDivided]: " << zArray[recvSize- isDivided] << " isDivided: " << isDivided <<"," << patternLength - isDivided<< endl;
                        if(zArray[currPos] == patternLength - isDivided){
                            //cout << "pattern found at index " << pos+1 << endl;
                            //update the zArray
                            zArray[currPos] = patternLength;
                            //cout<<"seconds works";
                        }
                        else{
                            if(debug)
                                cout << "pattern not found" << endl;
                        }
                    }
                    isDivided--;
                    if(debug)
                        cout << "Zarray[recvSize - isDivided]: " << zArray[recvSize- isDivided] << " isDivided: " << isDivided << endl;
                }
            }
            
            //print localZArray
            if(debug && dataDebug){
                printf("rank: %d, localZArray: ", rank);
                for(int j = 0; j < localChunkSize; j++){
                    printf("%d ", localZArray[j]);
                }
                printf("\n");
            }
            //copy the localZArray to zArray
            for(int j = 0; j < localChunkSize; j++){
                zArray[recvSize + j] = localZArray[j];
            }
            
            recvSize += localChunkSize;
        }
       
        endTime1 = clock();

        double cpu_time_used = ((double) (endTime1 - startTime1)) / CLOCKS_PER_SEC;

        printf("Time taken: %f (s)\n", cpu_time_used);
        
        if(dataDebug){
            for(int i = 0; i < zArrayLength; i++){
                printf("%d ", zArray[i]);
            }
            printf("\n");
        }
        
        if(writeToTxt)
            writeOutputToTxt(zArray, zArrayLength, patternLength);
        else
            writeOutputToBinary(zArray, zArrayLength);

        //count occurrences of pattern
        int count = 0;
        for(int i = 0; i < zArrayLength; i++){
            if(zArray[i] == patternLength){
                count++;
            }
        }

        cout << "Pattern found " << count << " times" << endl;
        
        //free the memory allocated for zArray
        free(zArray);

        
    }
    else{
        if(logDebug){
            printf("Rank %d sending data\n", rank);
            cout << "chunkSize: " << chunkSize << endl;
        }

        //send local chunk size
        MPI_Send(&chunkSize, 1, MPI_LONG, 0, 0, MPI_COMM_WORLD);
        MPI_Send(localZArray, chunkSize, MPI_INT, 0, 0, MPI_COMM_WORLD);  
        MPI_Send(&isDivided, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    // Finalize the MPI environment.
    MPI_Finalize();

    if(debug && rank == 0)
        cout <<"Successful exit by rank " << rank << endl;
    return 0;
}
