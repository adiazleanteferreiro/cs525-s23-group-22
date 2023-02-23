#include "dberror.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>


extern RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, const int numPages, ReplacementStrategy strategy, void *stratData) {
 // Initialize buffer pool attributes
    bm->pageFile = (char*) pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;
	    
    if (bm->mgmtData == NULL) {
        return RC_ERROR;
    }
    
    BM_PageFrame *pageFrames = calloc(numPages, sizeof(BM_PageFrame));
    
    // Initialize page frames
    for (int i = 0; i < numPages; i++) {
        pageFrames[i].data = (char*) calloc(PAGE_SIZE, sizeof(char));
        pageFrames[i].pageNum = NO_PAGE;
        pageFrames[i].dirty = false;
        pageFrames[i].fixCount = 0;
        pageFrames[i].pageHandle = NULL;
        pageFrames[i].next = NULL;
        pageFrames[i].prev = NULL;
    }

	bm->mgmtData = pageFrames //we assign the memory block to the mgmtData attribute of the buffer pool
   
	//stratData??
	/*
	// Initialize LRU-K replacement strategy data for each page frame
        if (strategy == RS_LRU_K) {
            pageFrames[i].k = k;
            pageFrames[i].referenceBit = calloc(k, sizeof(bool));
            for (int j = 0; j < k; j++) {
                pageFrames[i].referenceBit[j] = false;
            }
        }
	*/

    return RC_OK;
}


extern RC shutdownBufferPool(BM_BufferPool *const bm){

    // Check if the buffer pool is already shutdown or not
	BM_PageFrame *pageFrames = (BM_PageFrame*)bm->mgmtDat

    if (pageFrames == NULL) {
        return RC_BUFFER_POOL_ALREADY_SHUTDOWN;
    }

    // Flush all the dirty pages before shutdown
    forceFlushPool(bm);

    // Check if there are any pinned pages before shutdown
    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrames[i].isDirty == true || pageFrames[i].fixCount != 0) {
            return RC_PINNED_PAGES_EXIST;
        }
    }

    for (int i = 0; i < bm->numPages; i++) {
        free(pageFrames[i].data);
    }
    free(pageFrames);

    // Free all the allocated memory for buffer pool and Reset buffer pool attributes
    bm->pageFile = NULL;
    free(bm->mgmtData);
    free(bm);

    return RC_OK;
}

extern RC forceFlushPool(BM_BufferPool *const bm){

	char *filename = (char *) bm->pageFile;
    SM_FileHandle fh;
    if (openPageFile(filename, &fh) != RC_OK) {
        return RC_FILE_NOT_FOUND;
    }
	
	BM_PageFrame *pageFrames = (BM_PageFrame *) bm->mgmtData;
    RC rc;
    
    if (pageFrames == NULL) {
        return RC_WRITE_FAILED;
    }

    for (int i = 0; i < bm->numPages; i++) {
        if (pageFrames[i].isDirty == true && pageFrames[i].fixCount == 0) {
            rc = writeBlock(pageFrames[i]->pageNum, &fh, pageFrames[i]->pageHandle->data);; //method that writes a specific page back to disk if it is dirty. 
            if (rc != RC_OK) {
                return rc;
            }
        }
    }

    return RC_OK;
}





// Buffer Manager Interface Access Pages
extern RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page) {
    int pageNum = page->pageNum;
    BM_PageFrame *pageFrame = ((BM_PageFrame*)bm->mgmtData) + pageNum;
    
    if (pageFrame != NULL) {
        pageFrame->dirty = true;
        return RC_OK;
    }

    return RC_WRITE_FAILED;
}


extern RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page) {
    // Get the page frame associated with the requested page
    BM_PageFrame *pageFrame = ((BM_PageFrame*)bm->mgmtData) + page->pageNum;
    
    // Check if the requested page is currently pinned
    if (pageFrame != NULL && pageFrame->fixCount > 0) {
        pageFrame->fixCount--;
        
        // Update the page's data and pageNum fields
        pageFrame -> pageHandle = page
        pageFrame->pageNum = page->pageNum;
        pageFrame->data = page->data;
        return RC_OK;

    }
    return RC_WRITE_FAILED;
}

extern RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page) {
    
    BM_PageFrame *pageFrame = ((BM_PageFrame*)bm->mgmtData) + page->pageNum;

    if (pageFrame == NULL) {
        return RC_WRITE_FAILED;
    }
    
    SM_FileHandle fh;
   
    
    rc = (openPageFile(bm->pageFile, &fh) != RC_OK);
    if (rc != RC_OK) {
        return rc;
    }
   
    rc = ensureCapacity(page->pageNum, &fh);
    if (rc != RC_OK) {
        return rc;
    }

    // Write page content to disk
    rc = writeBlock(page->pageNum, &fh, pageFrame->data);
    if (rc != RC_OK) {
        return rc;
    }

    // Update dirty flag
    pageFrame->dirty = false;
    
    // Close page file
    closePageFile(&fh);
    
    return RC_OK;
}


extern RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum) {
    // Check if pageNum is valid
    if (pageNum < 0 || pageNum >= bm->numPages) {
        return RC_INVALID_PAGE_NUM;
    }
    
    
    BM_PageFrame *pageFrame = ((BM_PageFrame*)bm->mgmtData) + pageNum; // We add pageNum to the starting address to get a pointer to the correct page frame in memory.

    // Check if the page is already pinned
    if (pageFrame->fixCount > 0) {
        page->pageNum = NO_PAGE;
        page->data = NULL;
        return RC_PAGE_IS_PINNED;
    }
    
    // Read the page from disk if it is not in the buffer pool
    if (pageFrame->pageNum == NO_PAGE) {
        SM_FileHandle fh;
        RC rc;
        
        rc = openPageFile(bm->pageFile, &fh);
        if (rc != RC_OK) {
            return rc;
        }
        
        rc = ensureCapacity(pageNum + 1, &fh);
        if (rc != RC_OK) {
            return rc;
        }
        
        rc = readBlock(pageNum, &fh, pageFrame->data);
        if (rc != RC_OK) {
            return rc;
        }
        
        rc = closePageFile(&fh);
        if (rc != RC_OK) {
            return rc;
        }
        
        pageFrame->pageNum = pageNum;
        pageFrame->dirty = false;
    }
    
    pageFrame->fixCount++;
    pageFrame->data = page->data;
    
    return RC_OK;
}



// Statistics Interface
PageNumber *getFrameContents (BM_BufferPool *const bm);
bool *getDirtyFlags (BM_BufferPool *const bm);
int *getFixCounts (BM_BufferPool *const bm);
int getNumReadIO (BM_BufferPool *const bm);
int getNumWriteIO (BM_BufferPool *const bm);

#endif
