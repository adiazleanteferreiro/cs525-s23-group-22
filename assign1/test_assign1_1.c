#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void testCreateOpenClose(void);
static void testSinglePageContent(void);

/* main function running all tests */
int
main (void)
{
  testName = "";
  
  initStorageManager();

  testCreateOpenClose();
  testSinglePageContent();
  testSeveralPages(); 

  return 0;
}


/* check a return code. If it is not RC_OK then output a message, error description, and exit */
/* Try to create, open, and close a page file */
void
testCreateOpenClose(void)
{
  SM_FileHandle fh;

  testName = "test create open and close methods";

  TEST_CHECK(createPageFile (TESTPF));
  
  TEST_CHECK(openPageFile (TESTPF, &fh));
  ASSERT_TRUE(strcmp(fh.fileName, TESTPF) == 0, "filename correct");
  ASSERT_TRUE((fh.totalNumPages == 1), "expect 1 page in new file");
  ASSERT_TRUE((fh.curPagePos == 0), "freshly opened file's page position should be 0");

  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));

  // after destruction trying to open the file should cause an error
  ASSERT_TRUE((openPageFile(TESTPF, &fh) != RC_OK), "opening non-existing file should return an error.");

  TEST_DONE();
}

/* Try to create, open, and close a page file */
void
testSinglePageContent(void)
{
  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "test single page content";

  // allocate memory for a page
  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  // create a new page file
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");
  
  // read first page into handle
  TEST_CHECK(readFirstBlock (&fh, ph));
  // the page should be empty (zero bytes)
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
  printf("first block was empty\n");
    
  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  TEST_CHECK(writeBlock (0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first block\n");

  // destroy new page file
  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));  

  // free page memory
  free(ph);
  
  TEST_DONE();
}

void 
testSeveralPages(void) {

  SM_FileHandle fh;
  SM_PageHandle ph;
  SM_PageHandle ph2;

  int i;

  testName = "test serveral page content";

  // allocate memory for each page
  ph = (SM_PageHandle) malloc(PAGE_SIZE);
  ph2 = (SM_PageHandle) malloc(PAGE_SIZE);


  // create a new pages file
  TEST_CHECK(createPageFile (TESTPF));
  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("created and opened file\n");

  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';

  TEST_CHECK(writeBlock (0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first block\n");

  // append a new block
  TEST_CHECK(appendEmptyBlock(&fh));
  printf("second block is appended\n");
  ASSERT_EQUALS_INT(2, fh.totalNumPages, "expected 2 pages in the file");
  printf("The file has two pages \n");

  // check that the second page is empty 
  TEST_CHECK(readNextBlock(&fh, ph2));

  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph2[i] == 0), "expected zero byte in new page freshly initialized page for the appended block");
  printf("Second block was empty\n");

  int current = fh.curPagePos;

  ASSERT_EQUALS_INT(getBlockPos(&fh), fh.curPagePos, "Obtained position is not correct");
  printf("Position obtained correctly\n");

  printf("Checking capacity method correctly\n");
  TEST_CHECK((ensureCapacity(10, &fh)));
  ASSERT_EQUALS_INT(10, fh.totalNumPages, "expected 10 pages in the file capacity");
  ASSERT_EQUALS_INT(current, fh.curPagePos, "curPagePos should be 1 even if we added pages");

  printf("Capacity file has the correct number of pages\n");

  // Destroy the new page file
  TEST_CHECK(closePageFile (&fh));
  TEST_CHECK(destroyPageFile (TESTPF));  

  free(ph);
  free(ph2);
  
  TEST_DONE();
}