#include "dberror.h"
#include "storage_mgr.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

extern void initStorageManager(void) { // Initialize the Storage Manager
    if (access(".", W_OK) != 0){ // Check if we have write permission for the current working directory
        printf("Storage manager doesn't have write permission in this folder.\nExiting...\n");
        exit(RC_WRITE_FAILED);
    } 
    // The following code initializes the structs in case there were stored values
    SM_FileHandle fileHandle;
    fileHandle.fileName = NULL;
    fileHandle.totalNumPages = 0;
    fileHandle.curPagePos = 0;
    fileHandle.mgmtInfo = NULL;

    SM_PageHandle pageHandle;
    pageHandle = NULL;
    printf("Storage manager initialized\n");
}

extern RC createPageFile (char *fileName){ // Creates a page file
    FILE *file = fopen(fileName, "w+"); // Opens the file for both reading and writing
    SM_FileHeader fHeader;
    fHeader.totalNumPages = 1;          // The created file has 1 page
    fHeader.curPagePos = 0;             // The current position of the created file is 0
    fwrite(&fHeader,1,sizeof(fHeader),file); // Write the binary representation of the struct fHeader to file
    char *charArray = calloc(PAGE_SIZE, 1);  // Creates a character array of PAGE_SIZE and sets all its elements to 0
    int write = fwrite(charArray, 1, PAGE_SIZE, file); // Write in file the binary representation of charArray 
    fclose(file);  // Closes the file
    if (write != PAGE_SIZE){  // Checks if all the binary representation of charArray was succesfully written to the file
        return RC_WRITE_FAILED;			
    }
    free(charArray); // Frees the memory previously allocated by calloc
    return(RC_OK);
}

extern RC openPageFile (char *fileName, SM_FileHandle *fHandle){ // Opens a page file
    FILE *file = fopen(fileName, "r+"); // Opens a file for both reading and writing
    if(!file){  // Checks if the file exists
        return RC_FILE_NOT_FOUND;   
    }
    SM_FileHeader fHeader;
    fread(&fHeader, 1, sizeof(fHeader), file);  // Reads the binary representation of fHeader from file and stores it in fHeader
    // The following code updates the page information
    fHandle -> fileName = fileName;
    fHandle -> totalNumPages = fHeader.totalNumPages;
    fHandle -> curPagePos = fHeader.curPagePos;
    fHandle -> mgmtInfo = file;
    return RC_OK;
}

extern RC closePageFile(SM_FileHandle *fHandle) { // Closes the page file
    FILE *file = fHandle->mgmtInfo; // Opens the file for both reading and writing
    if(!file){  // Checks if the file exists 
        return RC_FILE_NOT_FOUND;   
    }   
    int status = fclose(file); 
    if(status != 0){ // Checks if the file has been closed
        return RC_FILE_NOT_FOUND;
    } 
    return RC_OK;
}

extern RC destroyPageFile (char *fileName){ // Destroyes the page file
    FILE *file = fopen(fileName, "r+"); // Opens the file for both reading and writing
    if(!file){  // Checks if the file exists
        return RC_FILE_NOT_FOUND;
    }
    int status = fclose(file);
    if(status != 0){ // Checks if the file has been closed
        return RC_FILE_NOT_FOUND;
    }
    remove(fileName); // Removes the file
    return RC_OK;
}

extern RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
    if(fHandle->totalNumPages < pageNum || pageNum < 0){  // If the file has less than pageNum pages
        return RC_READ_NON_EXISTING_PAGE;   // Non existing page
    }
    SM_FileHeader fHeader;
    FILE *file = fHandle->mgmtInfo; // Opens the file for both reading and writing
    if(!file){  // If file is NULL
        return RC_FILE_NOT_FOUND;   // File not found
    }
    int position = PAGE_SIZE * pageNum + sizeof(fHeader); // We declare the position as the size of one page times the page number
    
    if(fseek(file, position, SEEK_SET) != 0){ // If the seek is not successful (different than 0)
       return RC_READ_NON_EXISTING_PAGE;    // Non existing page
    }
    // If the seek is successful (equal to 0)
    fread(memPage, 1, PAGE_SIZE, file); // Read the page
    fHandle -> curPagePos = pageNum; // Update the current page position to the page number
    fread(&fHeader, sizeof(fHeader), 1, file);
    fHeader.curPagePos = pageNum; 
    return RC_OK;   
}

extern int getBlockPos (SM_FileHandle *fHandle){ 
    return (fHandle -> curPagePos);
}

extern RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    return readBlock(0, fHandle, memPage); // We might need to indicate 0 instead of 1 (lets try it)
}

extern RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int position = getBlockPos(fHandle) - 1;
    if(position < 0){
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(position, fHandle, memPage);
}

extern RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int position = getBlockPos(fHandle);
    return readBlock(position, fHandle, memPage);
}

extern RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int position = getBlockPos(fHandle) + 1;
    if(position > fHandle -> totalNumPages){
        return RC_READ_NON_EXISTING_PAGE;
    }
    return readBlock(position, fHandle, memPage);
}

extern RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int numPages = fHandle -> totalNumPages;
    return readBlock(numPages, fHandle, memPage);
}

extern RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage){
    if(pageNum < 0 || pageNum > fHandle->totalNumPages){ // If pageNum is out of range
        return RC_READ_NON_EXISTING_PAGE;
    }
    FILE *file = fHandle->mgmtInfo; // Opens the file for both reading and writing
    if(!file){  // If file is NULL
        return RC_FILE_NOT_FOUND;
    }
    SM_FileHeader fHeader;
    fread(&fHeader, sizeof(fHeader), 1, file);
    int position = PAGE_SIZE * pageNum + sizeof(fHeader);
    fseek(file, position, SEEK_SET);
    int write = fwrite(memPage, 1, PAGE_SIZE, file);
    if(write != PAGE_SIZE){
        return RC_WRITE_FAILED;
    }
    fHandle -> curPagePos = pageNum;
    fHeader.curPagePos = pageNum;
    return RC_OK;
}

extern RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage){
    int position = getBlockPos(fHandle);
    return writeBlock(position, fHandle, memPage);
}

extern RC appendEmptyBlock (SM_FileHandle *fHandle){
    FILE *file = fHandle->mgmtInfo; // Opens the file for both reading and writing
    if(!file){ // If file is NULL
        return RC_FILE_NOT_FOUND;
    }
    int position = getBlockPos(fHandle);
    SM_FileHeader fHeader;

    fread(&fHeader, sizeof(fHeader), 1, file);

    int lastPos = PAGE_SIZE * fHandle->totalNumPages + sizeof(fHeader);

    fseek(file, lastPos , SEEK_SET);

    char * charArray =  calloc(PAGE_SIZE, 1);

    int status = fwrite(charArray, 1, PAGE_SIZE, file);

    if(status != PAGE_SIZE){
        return RC_WRITE_FAILED;
    }
    fHandle -> totalNumPages++;
    fHeader.totalNumPages++;

    int pos = PAGE_SIZE * position + sizeof(fHeader);

    fseek(file, pos, SEEK_SET); // We go back to the previous current position
    free(charArray);
    return RC_OK;
}

extern RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle){
    FILE *file = fHandle->mgmtInfo; // Opens the file for both reading and writing
    if(!file){
        return RC_FILE_NOT_FOUND;
    }
    while(fHandle->totalNumPages < numberOfPages){
        RC status = appendEmptyBlock(fHandle);
        if(status != RC_OK){
            return RC_WRITE_FAILED;
        }
    }
    return RC_OK;
}





