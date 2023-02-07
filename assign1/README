# CS525 Coding assignment 1 : Storage manager
## Group
|Student name| iit mail|
|---|---|
|Augusto Diaz-Leante|[adiazleanteferreiro@hawk.iit.edu](mailto:adiazleanteferreiro@hawk.iit.edu)|
|Jorge Mendez-Benegassi|[XXX@hawk.iit.edu](mailto:XXx@hawk.iit.edu)|
|Fernando Heredero|[XXX@hawk.iit.edu](mailto:xxx@hawk.iit.edu)|

## Compile and run
- To obtain the code on the machine a git clone needs to be done.
- To compile the test file you just need to type `make` in the cli of the server.
- After compiling and running the file, a Valgrind Heap Summary is included to show if there is any memory leakes.
- To clean the environment you can use the command: `make clean`.

## New tests scenarios
We have added a method in the main to test an scenario (testSeveralPages). These methods does the following tests:
- Create and open a file.
- Write information in the created page.
- Check if the written information is correct using the ReadFirstBlock method.
- This tests are the same as on other methods to check correct performance of the basic methods.
- We check if the appendEmptyBlock is working properly by checking if the totalNumPages value is updated to contain the new page.
- Read the next block from the cursor (in this case as we opened the file, it's the first block).
- Check if the new page inserted with the appendEmptyBlock is empty.
- Check if the getBlockPos returns the correct position.
- Check if the ensureCapacity method works properly by adding 10 new pages and checking the value of totalNumPages and curPagePos (it shouldn't change).
- Close the file and free the memory as it is done on previous tests to prevent memory leaks.

## Code explanation
### Initialization
Initialization include determining whether the program is permitted to write to the folder and setting any stored values for the structures to null. If these conditions are not met, we are removed from the program.

### New Structure associated to file

To know the number of pages of the document and the current page before the document was closed, we created a new structure called SM_FileHeader, declared in the storage_mgr.h file. The struct has

typedef struct SM_FileHeader { 
	int totalNumPages;
	int curPagePos;
} SM_FileHeader;

**It is important to note that to compute the position of the pages in the methods of the program we need to consider this structure.

### What's in stored in `void *mgmtInfo`
Once a file is opened on the openPageFile method the pointer to the opened stream is stored in the `mgmtInfo` variable.

This allowed us to more quickly access to the file associated to the `SM_FileHandle`.

### Considerations
An important consideration is that some methods reuse some of the other methods, for example: the `readNextBlock` method will use the `readBlock` method with the corresponding page number; the `ensureCapacity` method uses a while loop to call the `appendEmptyBlock` until the given number of pages is reached.

This was done to prevent unnecessary duplicates among methods.

