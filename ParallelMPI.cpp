//Implementing Z algorithm for pattern matching in MPI
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <vector>
#include <time.h>
#include "mpi.h"

using namespace std;

const char* inputFileName; // = "../inputs/input1.txt";
const char* outputFileName; // = "../outputs/output_seq_1.txt";

int debug = 0;
int logDebug = 0;
int dataDebug = 0;

void isPrefixSuffix(char* text, char* pattern, int &pos, int &isPrefixRepeated){

    string patternStr = pattern;
    string textStr = text;

    int patternLength = patternStr.length();
    
    //truncate the text to the pattern length -1
    string truncatedText = textStr.substr(0, patternLength - 1);

    cout << "truncated text: "<< truncatedText << endl;
    cout << "pattern: " << patternStr << endl;

    //check all the suffixes of the pattern are present in the truncated text prefix
    int found = 0;
    int isSuffixFound = 0;
    string suffix = "";

    //list to store all the suffixes of the pattern
    vector<string> suffixes;

    for(int i = patternLength-1; i >=0 ; i--){
        suffix = patternStr.substr(i, patternLength - i);
        if(truncatedText.find(suffix) != string::npos){
            //found++;
            //cout << "suffix found: " << suffix << endl;
            suffixes.push_back(suffix);
            pos = suffix.length();
        }
    }

    //print the suffixes
    /*
    cout << "suffixes:";
    for(int i = 0; i < suffixes.size(); i++){
        cout << " " << suffixes[i];
    }
    cout << endl;
    */
    

    //print the found value 
    //if(dataDebug)
    cout << "found: " << found << endl;

    //isPrefixRepeated = pos;


    cout << "pos: " << pos << endl;
    cout << "isPrefixRepeated: " << isPrefixRepeated << endl;

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

    /*
    //print z
    if(dataDebug){
        printf("After func: n: %d\n", n);
        //print res
        for(int i = 0; i < n; i++){
            printf("%d ", z[i]);
        }
    }
    */

    if(logDebug){
        printf("successfully executed z_function\n");
    }
}

//z-algorithm
int* z_algorithm(char* text, char* pattern, int* res, int &found, int &isPrefixRepeated) {
    int n = strlen(text);
    int m = strlen(pattern);

    int* z = (int*)malloc((n + m + 1) * sizeof(int));

    //print pattern and text
    if(logDebug)
        printf("z-algo: pattern: %s, text: %s\n", pattern, text);

    //check if memory is allocated
    if(z == NULL) {
        printf("\tError: Unable to allocate memory for z array\n");
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
        printf("\tError: Unable to allocate memory for s array\n");
        return res;
    }

    strcpy(s, pattern);
    s[m] = '$';
    strcpy(s + m + 1, text);
    
    if(dataDebug)
        printf("\talgo: s: %s\n", s);

    z_function(s, z);

    isPrefixRepeated = 0;

    isPrefixSuffix(text, pattern, found, isPrefixRepeated);

    //print z values
    if(dataDebug)
        for(int i = 0; i < n + m + 1; i++){
            printf("%d ", z[i]);
        }
    printf("\n");

    //copy the z values to res after the pattern length
    for(int i = m + 1; i < n + m + 1; i++){
        res[i - m - 1] = z[i];
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
            fprintf(outputFile, "Pattern found at index: %ld\n", i+1);
            isMatchFound = 1;
        }
    }
    //write zArray to the output file
    for(int i = 0; i < zArrayLength; i++){
        fprintf(outputFile, "%d ", zArray[i]);
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
    int isPrefixRepeated = 0;

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

        /*
        //print localZArray
        if(debug && logDebug){
            printf("rank: %d, localZArray: ", rank);
            for(int i = 0; i < chunkSize + patternLength + 1; i++){
                printf("%d ", rank, i, localZArray[i]);
            }
            printf("\n");
        }
        */

        //call z_algorithm
        localZArray = z_algorithm(localText, pattern, localZArray, isDivided, isPrefixRepeated);

        //cout << "isDivided by rank" << rank << " is " << isDivided << endl;
        //cout << "isPrefixRepeated by rank" << rank << " is " << isPrefixRepeated << endl;
    
        //print localZArray
        if(debug && logDebug){
            printf("rank: %d, localZArray: ", rank);
            for(int i = 0; i < chunkSize; i++){
                printf("%d ", localZArray[i]);
            }
            printf("\n");
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
        localZArray = z_algorithm(localText, pattern, localZArray, isDivided, isPrefixRepeated);

        if(logDebug && isDivided != -1)
            cout << "isDivided by rank" << rank << " is " << isDivided << endl;
            cout << "isPrefixRepeated by rank" << rank << " is " << isPrefixRepeated << endl;
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
            MPI_Recv(&isPrefixRepeated, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if(isDivided != -1)
                cout << "combing: rank " << i << "  isDivided by  is " << isDivided << " && isPrefixRepeated: " << isPrefixRepeated << endl;

            //print Zarray with index
            if(1 || debug){
                printf("rank: %d, localZArray: ", i);
                for(int j = 0; j < recvSize; j++){
                    printf("%d: %d \n", j, zArray[j]);
                }
                printf("\n");
            }

            if(isDivided != -1){
                while(isDivided >0){
                    int currPos = recvSize - patternLength + isDivided;
                    if(currPos !=0){
                        //cout <<"first: " <<recvSize - isDivided << "\n";
                        cout <<"second: " <<recvSize - (patternLength - isDivided) << "\n";
                        //cout << "Zarray[recvSize - isDivided]: " << zArray[recvSize- isDivided] << " isDivided: " << isDivided <<"," << patternLength - isDivided<< endl;

                        if(zArray[currPos] == patternLength - isDivided){
                            //cout << "pattern found at index " << pos+1 << endl;
                            //update the zArray
                            zArray[currPos] = patternLength;
                            //cout<<"seconds works";
                        }
                        else{
                            cout << "pattern not found" << endl;
                        }
                        /*if(zArray[recvSize - isDivided] == isDivided){
                            //cout << "pattern found at index " << pos+1 << endl;
                            //update the zArray
                            zArray[recvSize - isDivided] = zArray[recvSize - isDivided] + (patternLength - isDivided);
                        }
                        else{
                            cout << "pattern not found" << endl;
                        }*/
                    }
                    //cout << "Zarray[recvSize - isDivided]: " << zArray[recvSize- isDivided] << " isDivided: " << isDivided << endl;
                    isDivided--;
                        cout << "Zarray[recvSize - isDivided]: " << zArray[recvSize- isDivided] << " isDivided: " << isDivided << endl;

                }
            }
            

            

            /*if(zArray[pos] == patternLength - isDivided -1){
                    //cout << "pattern found at index " << pos+1 << endl;
                    //update the zArray
                    zArray[pos] = patternLength;
                }
                else{
                    cout << "pattern not found" << endl;
                }
                */


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
        MPI_Send(localZArray, chunkSize, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&isDivided, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&isPrefixRepeated, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }


    //MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    
    if(debug || rank == 0){
        printf("successfully executed main\n");
    }
    return 0;
}
