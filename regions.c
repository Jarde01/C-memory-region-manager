#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "regions.h"

typedef struct BUFFER_INDEX Index;
struct BUFFER_INDEX
{
  unsigned char *location;  //pointer to start of thing
  r_size_t size;   //size of region in bytes
  Index *nextI;
};

//Linked list node definition (a3)
typedef struct NODE Node;
struct NODE
{
  char *string; //stores name of this block of memory
  unsigned char *region; //stores the location where the memory is located for this region
  r_size_t regionSize;
  Index *index;
  int numBlocks;
  Node *next;   //pints to the next node, if any exists
};


// VARIABLES---------------------------------------------------------------------------

//Node 
static Node *top = NULL;          //top of the linked list
static int numNodes = 0;          //number of nodes in the list
static Node *workingRegion = NULL;   // track which of the regions is currently chosen!

//static Index *topI = NULL;
static Index *workingIndex = NULL;

//static Index *currIndex;    //top of the index

// PROTOTYPES---------------------------------------------------------------------------

//old methods, use parts of these
void validateList();
void validateIndex(); //this won't work'
Boolean search( char const * const target );
void testSearch(Boolean guess, char * search);
void *openSpace( r_size_t reqSpace );
Boolean setUpDummyNodes();
Boolean zeroOut(Index * newIndex);



// FUNCTIONS---------------------------------------------------------------------------

//return the number that is the nearest multiple of 8, rounded up
int nearestEight(int start)
{
    int result = start;
    while (result%8 != 0)
    {
        result++;
    }
    return result;
}

void validateList()
{
  if (numNodes == 0)
  {
    assert(top == NULL);
  }
  else if (numNodes == 1)
  {
    assert(top->next == NULL);
  }
  else
  {
    assert(top->next != NULL && top != NULL); //order shouldn't matter technically
  }
}

//invariant for the index list, unsure if I will/can use this
void validateIndex(Node * current)
{
  if (current->numBlocks == 0)
  {
    assert(current->index == NULL);
  }
  else if (current->numBlocks == 1)
  {
    assert(current->index->nextI == NULL);
  }
  else
  {
    assert(current->index->nextI != NULL && current->index != NULL); //order shouldn't matter technically
  }
}

//create a region with a given name and size
//unique name, size greater than 0. 
//If true then choose this region with rchoose
Boolean rinit( const char *region_name, r_size_t region_size )
{
  Boolean rc = false;
  Node *newNode = NULL;
  Boolean inList = false;
  int numNodesBefore = numNodes;
  Boolean dummyNodesSet = false;

  validateList();

  assert(region_size >0 ); //make sure the region size is valid
  if (region_size > 0)
  {

    assert( region_name != NULL );
    if ( region_name )
    {
      inList = search(region_name); //search for the value in the list
      if (inList == false)
      {

        newNode = malloc( sizeof( Node ) );

        assert( newNode != NULL );
        if ( newNode != NULL )
        {
          //make current top point to the next item in the list
          newNode->next = top;
          top = newNode;

          //point the index to null
          newNode->index = NULL;
          newNode->numBlocks = 0;
          
          newNode->string = malloc( strlen(region_name) + 1 );
          
          assert( newNode->string != NULL );
          if ( newNode->string != NULL )
          {
            strcpy( newNode->string, region_name );
            numNodes++;

            //make the region here:
            newNode->region = malloc( nearestEight(region_size));
            newNode->regionSize = region_size; //give it a size variable
            
            assert( newNode->string != NULL );
            if (newNode->region != NULL)
            {
              rc = true;
              workingRegion = newNode; //change current active region
              
              //add the trailing and heading index nodes
              dummyNodesSet = setUpDummyNodes();

            }
          
            else
            {
              free( newNode);
            }
          }
        }
      }
    }
  }

  //make sure number of nodes hasn't changed
  assert( rc == false || numNodesBefore < numNodes );
  validateList();
  
  return rc;
}


Boolean setUpDummyNodes()
{
  Boolean result = false;
  Index *dummyHead = NULL;
  Index * dummyTail = NULL;

  assert(workingRegion->index == NULL);
  if (workingRegion->index == NULL)
  {

    dummyHead = malloc(sizeof(Index));
    dummyTail = malloc(sizeof(Index));

    assert(dummyHead);
    assert(dummyTail);

    if (dummyHead != NULL && dummyTail != NULL)
    {

      dummyHead->location = workingRegion->region; //point dummy head to beginning of the index
      dummyTail->location = ( workingRegion->region + workingRegion->regionSize ); //point dummy tail to end of region

      dummyHead->size = 0;
      dummyTail->size = 0;

      //connect the dummy nodes to the working regions index
      workingRegion->index = dummyHead;
      dummyHead->nextI = dummyTail;
      dummyTail->nextI = NULL;

      result = true;
    }

    else
    {
      free(dummyHead);
      free(dummyTail);
    }

  }

  return result;
}


//choose a currently-chosen region, or NULL with no region chosen
//go through the list of regions and find one with the correct name
Boolean rchoose( const char *region_name )
{
  Boolean found = false;

  validateList();

  Node *curr = top;

  if (curr)
  {
    while ( curr != NULL && !found )
    {
      if ( strcmp( region_name, curr->string ) == 0 )
      {
        found = true;
        workingRegion = curr; //point workingRegion to chosen location
        workingIndex = workingRegion->index; //blue apples (delete)
      }
      
      else
      {
        curr = curr->next;
      }
    }
  }

  validateList();
  return found;
}


const char *rchosen()
{
    //return pointer somewhere
    char * result = NULL;

    if (workingRegion != NULL)
    {
      result = workingRegion->string;
    }
    else 
    {
      result = "NULL"; //no region selected
    }

    return result;
}


void * openSpace(r_size_t reqSpace)
{
  void * result = NULL;// point to start
  Index * curr = NULL;
  Index * prev = NULL;
  unsigned char * nextOpenPtr = NULL;
  r_size_t freeSpace = 0;

  curr = workingRegion->index->nextI; //point to the second node
  prev = workingRegion->index;

  while (curr!= NULL)
  {
    nextOpenPtr = (prev->location + prev->size);
    printf("YOYOBOY - - next open: %p\n", nextOpenPtr);

    freeSpace = (curr->location - nextOpenPtr);
    printf("YOYOBOY - - freeSpace: %i\n", (int) freeSpace);

    printf("freeSpace; %i , required space: %i\n", freeSpace,  reqSpace);
    if ( freeSpace >= reqSpace) //there is enough space for the wanted amount of memory
    {
      result = nextOpenPtr; //point to the open space
    }

    //updating conditions
    prev = curr;
    curr = curr->nextI;
  }

  //printf("(openSpace) Result: %p\n", result);
  return result;
}

void *ralloc( r_size_t block_size )
{
  //ALLOCATE: ******************************
  void * result = NULL;
  r_size_t blockSize;
  Index * newIndex = NULL;
  void * openLocation = NULL;
  Index * current = NULL;
  Index * head = NULL;
  Boolean zeroDone = false;

  if (block_size > 0 )
  {

    blockSize = nearestEight(block_size);
    assert(blockSize > 0);
    assert(blockSize%8 == 0);

    if (blockSize %8 == 0)
    {

      //search for a space big enough in the memory region to point too, NULL if failed
      openLocation = openSpace(blockSize);
      printf("(ralloc) Open location: %p\n", openLocation);
      result = openLocation;

      if (openLocation != NULL) //if open location fails we can't add the memory space
      {

        newIndex = malloc(sizeof(Index));

        assert(newIndex);
        if (newIndex != NULL)
        {
          newIndex->size = blockSize;
          newIndex->location = openLocation;
          workingIndex = newIndex;

          zeroDone = zeroOut(newIndex);

          if (zeroDone == true)
          {
            //INSERTION, ordered linkedlist
            current = workingRegion->index;
            head = workingRegion->index;

            current = workingRegion->index;
            while (current->nextI != NULL && current->nextI->location < newIndex->location)
            {
                current = current->nextI; //update conditions
            }
            
            newIndex->nextI = current->nextI; //found the correct location and adding it
            current->nextI = newIndex;
          }
          else 
          {
            free(newIndex);
          }
        }
      }
      
    }
  }
  return result;
}

Boolean zeroOut(Index * newIndex)
{
  Boolean result = true;

  printf("Start ptr: %p\n", workingRegion->region);
  printf("int size: %i\n", (int)newIndex->size);
  
  for (int i = 0; i < (int)newIndex->size; i++)
  {
    workingRegion->region[i] = 0; //zeroing out all of the spaces
    if ( workingRegion->region[i] != 0)
    {
      result = false;
    }
    //printf("%i: %p: %c\n", i, &workingRegion->region[i], *workingRegion->region);
  }

  return result;
}

r_size_t rsize( void *block_ptr )
{
  //counts all of the bytes in memory that contain 0's?'
  return 0;
}

Boolean rfree( void *block_ptr )
{
    Boolean result = false;
    return result;
}

void rdestroy( const char *region_name )
{
  //destroy a certain block of memory
  validateList();
  Boolean deleted = false;
  Node *curr = top; 
  Node *prev = NULL;  //need to keep track of previous to go over if we find the node
  Index *currIndex = curr->index;
  Index *prevIndex;


  assert(region_name != NULL);
  
  while ( curr != NULL && strcmp( region_name, curr->string ) != 0 ) //while we haven't found the string yet
  {
    prev = curr;
    curr = curr->next;
  }

  if ( curr != NULL ) //we have found the desired node/region
  {
    if ( prev != NULL )
    {
        prev->next = curr->next; //skipping over the node in the list
    }
    else
    {
      top = curr->next;
    }

    //freeing up all of the node memory stuff
    free( curr->string );
    curr->string = NULL;
    assert(!curr->string);

    free( curr->region);
    curr->region = NULL;
    assert(!curr->region);

    //freeing up the indices
    if (currIndex != NULL)
    {
      while (currIndex != NULL)
      {
        prevIndex = currIndex;
        currIndex = currIndex->nextI;
        
        free(prevIndex); //get rid of all of the stuff inside
      }
    }

    free( curr );
    curr = NULL;
    assert(!curr);

    region_name = NULL;
    deleted = true;
    numNodes--;
    workingRegion = NULL;
  }

  validateList(); //postconds
}


void rdump()
{
  validateList();
  //validateIndex(workingRegion)

  Node *curr = top;
  Index *currIndex = workingRegion->index;

  double usedSpace = 0;

  int i = 1;
  if (curr != NULL)
  {
    while (curr != NULL)
    {
      printf("\n____| Region # %i: \"%s\" |___\n", i, curr->string);

      if (curr->index != NULL)
      {
        currIndex = curr->index;

        //printf("|----Starting at: %p\n", workingRegion->region);
        while (currIndex != NULL)
        {
          if ( currIndex->size != 0)
          {
            printf("|----%p: %i bytes.\n", currIndex->location, currIndex->size );
            usedSpace = usedSpace + currIndex->size;
          }
          currIndex = currIndex->nextI;
        }
        printf("Used: %i, Unused: %i\n", (int)usedSpace, (int)workingRegion->regionSize-(int)usedSpace);
        printf("Free space: %.*f%% \n", 2, 100 - (usedSpace/workingRegion->regionSize*100));

      }
      else
      {
        printf("No blocks.\n");
      }

      i++;
      curr = curr->next;
    }
  }
  else 
  {
    printf("No memory regions created.");
  }

  validateList();
}

//linked list A3 functions:

// tells us whether or not the given string is in the list
Boolean search( char const * const target )
{
  Boolean found = false;
  Node *curr = top;

  validateList(); //preconds/invariant

  assert(target != NULL);
  if (target)
  {
    
    while ( curr != NULL && !found )
    {
      if ( strcmp( target, curr->string ) == 0 ) //if string is found in the list
      {
        found = true;
      }
      
      else
      {
        curr = curr->next;
      }
    }
    validateList(); //post conds
  }

  return found;
}