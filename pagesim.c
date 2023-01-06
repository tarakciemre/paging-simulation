#include <math.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>

/* ==LOGIC==
1- [x] First, initialize the page tables.
2- [x] Page tables will keep a data structure
        This data structure will point at another table
        And keep a valid bit
3- [x] At the beginning, all will be invalid
4- [x] Check if the address is valid
5- [ ] Access the value in the given address
4- [] As an index is accessed, a frame will be assigned to the value of the thing and valid bit will be made 1
5- [] If a value is replaced, the valid bit of it will be changed to invalid.
*/

typedef struct page
{
    int frame;
    bool valid;
} page;

typedef struct validRange
{
    unsigned int startingAddress;
    unsigned int endingAddress;
    struct validRange *next;
} validRange;

typedef struct formattedAddress
{
    unsigned int outerIndex;
    unsigned int innerIndex;
    unsigned int frameOffset;
} formattedAddress;

typedef struct formattedPhysicalAddress
{
    unsigned int frame;
    unsigned int frameOffset;
} formattedPhysicalAddress;

typedef struct pageNode
{
    struct pageNode *next;
    page *page;
} pageNode;

void printQueue();
void accessRandomAddress();
page *removeNodeFromHead();
int getPhysicalLocation(formattedPhysicalAddress *input);
bool usePage(page *input);
bool insertNodeToEnd(page *input);
void pushAndDelete(page *input);

pageNode *pageNodeHead;
bool rflag = false;
char inputFile1[20] = "in1.txt";
char inputFile2[20] = "in2.txt";
char outFileName[20] = "in2.txt";
FILE *outFile;
int replacementPolicy = 1; // 0 for FIFO, 1 for LRU
int maxFrameCount = 3;
page outerTable[1024][1024];
int frameIndex = 0;
validRange *rangeHead = NULL;
void insertValidRange(unsigned int startingAddress, unsigned int endingAddress)
{
    validRange *newRange = malloc(sizeof(validRange));
    newRange->startingAddress = startingAddress;
    newRange->endingAddress = endingAddress;
    newRange->next = NULL;

    if (rangeHead == NULL)
    {
        rangeHead = newRange;
    }
    else
    {
        validRange *current = rangeHead;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = newRange;
    }
}

void createRanges(char input[])
{
    // For each line in the thing, call insertValidRange function.
}

void callValues()
{
}

bool checkAddress(unsigned int input);
formattedAddress *divideHexadecimalNumber(unsigned int input);
void tryToAccessValue(unsigned int input)
{
    // The address is valid. Now access the address
    if (!checkAddress(input))
    {

        if (rflag)
        {
            fprintf(outFile, "0x%08x ", input);
        }
        fprintf(outFile, "0x%08x e\n", input);
        return;
    }
    formattedAddress *virtualAddressDivision = divideHexadecimalNumber(input);
    unsigned int outerIndex = virtualAddressDivision->outerIndex;
    unsigned int innerIndex = virtualAddressDivision->innerIndex;
    page *accessedPage = &outerTable[outerIndex][innerIndex];
    bool valid = accessedPage->valid;
    // CASE 1: Hit
    if (valid)
    {
        if (replacementPolicy == 1)
        {
            usePage(accessedPage);
        }

        formattedPhysicalAddress *physicalAddress = malloc(sizeof(formattedPhysicalAddress));
        physicalAddress->frameOffset = virtualAddressDivision->frameOffset;
        physicalAddress->frame = accessedPage->frame;

        unsigned int location = getPhysicalLocation(physicalAddress);
        free(physicalAddress);

        if (rflag)
        {
            fprintf(outFile, "0x%08x ", input);
        }
        fprintf(outFile, "0x%08x\n", location);
        return;
    }
    // CASE 2: Miss, but there are empty frames
    if (maxFrameCount > frameIndex)
    {
        accessedPage->frame = frameIndex;
        insertNodeToEnd(accessedPage);
        // Print the accessed location
        formattedPhysicalAddress *physicalAddress = malloc(sizeof(formattedPhysicalAddress));

        physicalAddress->frameOffset = virtualAddressDivision->frameOffset;
        physicalAddress->frame = frameIndex;

        unsigned int location = getPhysicalLocation(physicalAddress);
        free(physicalAddress);

        if (rflag)
        {
            fprintf(outFile, "0x%08x ", input);
        }
        fprintf(outFile, "0x%08x x\n", location);

        frameIndex += 1;
        return;
    }
    // -- CASE 3: Miss, and there are not empty frames
    // -- -- CASE 3.1: FIFO
    if (replacementPolicy == 2)
    {
        pushAndDelete(accessedPage);
        formattedPhysicalAddress *physicalAddress = malloc(sizeof(formattedPhysicalAddress));

        physicalAddress->frameOffset = virtualAddressDivision->frameOffset;
        physicalAddress->frame = accessedPage->frame;

        unsigned int location = getPhysicalLocation(physicalAddress);
        free(physicalAddress);

        if (rflag)
        {
            fprintf(outFile, "0x%08x ", input);
        }
        fprintf(outFile, "0x%08x x\n", location);
        return;
    }

    // -- -- CASE 3.2: LRU
    pushAndDelete(accessedPage);
    formattedPhysicalAddress *physicalAddress = malloc(sizeof(formattedPhysicalAddress));

    physicalAddress->frameOffset = virtualAddressDivision->frameOffset;
    physicalAddress->frame = accessedPage->frame;

    unsigned int location = getPhysicalLocation(physicalAddress);
    free(physicalAddress);

    if (rflag)
    {
        fprintf(outFile, "0x%08x ", input);
    }
    fprintf(outFile, "0x%08x x\n", location);
    return;
}
int main(int argc, char *argv[])
{
    // initialize page table
    for (int i = 0; i < 1024; i++)
    {
        for (int j = 0; j < 1024; j++)
        {
            outerTable[i][j].valid = false;
        }
    }

    if (argv[3][0] != '-')
    {
        // Input from file
        strcpy(inputFile1, argv[1]);
        strcpy(inputFile2, argv[2]);
        maxFrameCount = atoi(argv[3]);
        strcpy(outFileName, argv[4]);
        replacementPolicy = atoi(argv[6]);
        outFile = fopen(outFileName, "w");

        // Read files
        FILE *fp = fopen(inputFile1, "r");

        char word[30];
        // traverses for all words independently
        int i = 0;
        unsigned int beginning;
        unsigned int ending;
        while (fscanf(fp, "%s", word) != EOF)
        {
            unsigned int number = (int)strtol(word, NULL, 0);
            i += 1;
            if (i % 2 != 0)
            {
                beginning = number;
            }
            else
            {
                ending = number;
                insertValidRange(beginning, ending);
            }
        }
        // close the file
        fclose(fp);

        fp = fopen(inputFile2, "r");

        while (fscanf(fp, "%s", word) != EOF)
        {
            int number = (int)strtol(word, NULL, 0);
            tryToAccessValue(number);
        }
        // close the file
        fclose(fp);
    }
    else
    {
        // Random access
        maxFrameCount = atoi(argv[1]);
        strcpy(outFileName, argv[2]);
        replacementPolicy = atoi(argv[4]);
        unsigned int number = (unsigned int)strtol(argv[6], NULL, 0);
        int addressCount = atoi(argv[8]);
        outFile = fopen(outFileName, "w");
        rflag = true;

        insertValidRange(0, number);

        for (int i = 0; i < addressCount; i++)
        {
            accessRandomAddress();
        }
    }

    printf("\n");
}

formattedAddress *divideHexadecimalNumber(unsigned int input)
{
    unsigned int divider1 = pow(2, 22);
    unsigned int value1 = input / divider1;
    unsigned int divider2 = pow(2, 12);
    unsigned int value2 = (input - (value1 * divider1)) / divider2;
    unsigned int value3 = input - (value1 * divider1) - (value2 * divider2);

    formattedAddress *newDivision = malloc(sizeof(formattedAddress));
    newDivision->outerIndex = value1;
    newDivision->innerIndex = value2;
    newDivision->frameOffset = value3;
    return newDivision;
}

bool checkAddress(unsigned int input)
{
    validRange *currentRange = rangeHead;

    while (currentRange != NULL)
    {
        // check if the value is between the current range values
        if (input >= currentRange->startingAddress && input < currentRange->endingAddress)
        {
            return true;
        }
        currentRange = currentRange->next;
    }
    return false;
}

page *removeNodeFromHead()
{
    if (pageNodeHead == NULL)
    {
        return NULL;
    }
    pageNodeHead->page->valid = false;
    page *temp = pageNodeHead->page;
    pageNodeHead = pageNodeHead->next;
    return temp;
}

bool usePage(page *input)
{
    pageNode *current = pageNodeHead;
    if (current == NULL)
    {
        return false;
    }
    if (current->next == NULL)
    {
        return true;
    }

    if (current->page == input)
    {
        pageNode *target = current;
        pageNodeHead = current->next;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = target;
        target->next = NULL;
        return true;
    }
    while (current->next->page != input)
    {
        current = current->next;
    }
    pageNode *target = current->next;
    current->next = target->next;
    while (current->next != NULL)
    {
        current = current->next;
    }
    current->next = target;
    target->next = NULL;
}

bool insertNodeToEnd(page *input)
{
    pageNode *current = pageNodeHead;
    pageNode *newPageNode = malloc(sizeof(pageNode));
    newPageNode->page = input;
    input->valid = true;
    newPageNode->next = NULL;

    if (pageNodeHead == NULL)
    {
        pageNodeHead = newPageNode;
    }
    else
    {
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = newPageNode;
        return true;
    }
    return true;
}

void pushAndDelete(page *pageInput)
{
    page *removedPage = removeNodeFromHead();
    int tempFrameOffset = removedPage->frame;
    pageInput->frame = tempFrameOffset;
    insertNodeToEnd(pageInput);
}

int getPhysicalLocation(formattedPhysicalAddress *input)
{
    unsigned int multiplier = pow(2, 12);

    return (input->frame * multiplier) + input->frameOffset;
}

void printQueue()
{
    pageNode *current = pageNodeHead;
    while (current != NULL)
    {
        printf("%d => ", current->page->frame);
        current = current->next;
    }
    printf("\n");
}

void accessRandomAddress()
{
    tryToAccessValue(rand() % rangeHead->endingAddress);
}