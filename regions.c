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
  char *string;           //stores name of this block of memory
  unsigned char *region;  //stores the location where the memory is located for this region
  r_size_t regionSize;    //size of the region
  Index *index;           //pointer to the index for this region
  Node *next;   //points to the next node, if any exists
};


// VARIABLES---------------------------------------------------------------------------

//Node 
static Node *top = NULL;          //top of the linked list
static int numNodes = 0;          //number of nodes in the list
static Node *workingRegion = NULL;   // track which of the regions is currently chosen!

//Index
static Index *workingIndex = NULL;

// PROTOTYPES---------------------------------------------------------------------------

//old methods, use parts of these
void validateList();
void validateIndex();
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

//invariant for the linked list containing nodes
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
    assert(top->next != NULL && top != NULL); 
  }
}

//invariant for the index list
void validateIndex()
{
  if (workingRegion != NULL) //make sure at minimum the two dummy nodes are set up
  {
    assert(workingRegion->index != NULL);
    assert(workingRegion->index->nextI != NULL);
  }
}

//create a region with a given name and size
//with a unique name, and requires a size greater than 0. 
//If true then choose this region with rchoose
Boolean rinit( const char *region_name, r_size_t region_size )
{
  Boolean rc = false;
  Node *newNode = NULL;
  Boolean inList = false;
  Boolean dummyNodesSet = false;
  r_size_t regionSize = 0;

  validateList(); //can't validate the index yet'

  if (region_size > 0)
  {

    regionSize = nearestEight(region_size);//upscale the region size

    assert(regionSize %8 ==0);
    
    if (regionSize %8 ==0)
    {
      if ( region_name != NULL) //making sure region_name is initialized
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
            newNode->string = malloc( strlen(region_name) + 1 );
            
            assert( newNode->string != NULL );
            if ( newNode->string != NULL )
            {
              strcpy( newNode->string, region_name );
              numNodes++;

              //make the region here:
              newNode->region = malloc( nearestEight(region_size));
              newNode->regionSize = regionSize;
              
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
  }

  validateList();
  validateIndex(); //make sure the index is properly set up

  return rc;
}


//create two index nodes, making it easier to find the open space during the ralloc function
Boolean setUpDummyNodes()
{
  Boolean result = false;
  Index *dummyHead = NULL;
  Index * dummyTail = NULL;

  validateList();   //can't validate index data yet because workingRegion is set up but nodes aren't

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

  validateList();
  validateIndex();

  return result;
}


//choose a currently-chosen region, or NULL with no region chosen
//go through the list of regions and find one with the correct name
Boolean rchoose( const char *region_name )
{
  Boolean found = false;
  validateList();

  if (workingRegion != NULL)
  {
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

  }

  validateList();
  return found;
}


const char *rchosen()
{
  validateList();
  validateIndex();

  char * result = NULL;

  if (workingRegion != NULL)
  {
    result = workingRegion->string;
  }


  validateList();
  validateIndex();

  return result;
}


void * openSpace(r_size_t reqSpace)
{
  void * result = NULL;// point to start
  Index * curr = NULL;
  Index * prev = NULL;
  unsigned char * nextOpenPtr = NULL;
  r_size_t freeSpace = 0;
  Boolean found = false;

  validateList();
  validateIndex();

  curr = workingRegion->index->nextI; //keeping track of current and prev nodes to determine free space
  prev = workingRegion->index;

  while (curr != NULL && found == false) //make sure to stop when the first space large enough is found
  {
    nextOpenPtr = prev->location+prev->size;
    freeSpace = curr->location - nextOpenPtr;

    if (freeSpace >= reqSpace) //we have enough contiguous space in the buffer to fit the ralloc
    {
      result = nextOpenPtr;    //return pointer to this open location
      found = true;            //condition for stopping the while loop when the first space has been found
    }

    prev = prev->nextI;
    curr = curr->nextI;
  }

  validateList();
  validateIndex();

  return result;
}

void *ralloc( r_size_t block_size )
{
  void * result = NULL;       //gives us the pointer to the start of the region
  r_size_t blockSize;         //how big the block needs to be
  Index * newIndex = NULL;    //pointer to a new index
  void * openLocation = NULL; //where the new index will point too in the buffer region
  Index * current = NULL;     //current index, for insertion algorithm
  Index * head = NULL;        //top index
  Boolean zeroDone = false;   //zeroing exit condition

  validateList();
  validateIndex();

  if (workingRegion != NULL) //making sure a region has been selected
  {

    if (block_size > 0 )
    {

      blockSize = nearestEight(block_size); //return the next closest multiple of 8 of the requested size
      assert(blockSize > 0);
      assert(blockSize%8 == 0);

      if (blockSize %8 == 0)
      {

        //search for a space big enough in the memory region to point too, NULL if failed
        openLocation = openSpace(blockSize);

        if (openLocation != NULL) //if open location fails we can't add the memory space
        {

          newIndex = malloc(sizeof(Index));

          assert(newIndex);
          if (newIndex != NULL)
          {
            newIndex->size = blockSize;
            newIndex->location = openLocation;
            workingIndex = newIndex;

            zeroDone = zeroOut( newIndex);      //make sure all of the block contents at this area are zeroed

            if (zeroDone == true)
            {
              //INSERTION, ordered linkedlist
              current = workingRegion->index;
              head = workingRegion->index;      //keep track of the top of the linked list

              while (current->nextI != NULL && current->nextI->location < newIndex->location)
              {
                  current = current->nextI;     //update conditions
              }
              
              newIndex->nextI = current->nextI; //found the correct location and adding it
              current->nextI = newIndex;

              result = openLocation;
            }
            else 
            {
              free(newIndex);
            }
          }
        }
        else 
        {
          result = NULL; //failed allocating this space
        }
      }
    }
  }

  validateList();
  validateIndex();

  return result;
}

Boolean zeroOut(Index * newIndex)
{
  Boolean result = true;
  unsigned char * sweeper;
  
  validateList();
  validateIndex();

  sweeper = newIndex->location;
  for ( int i = 0; i< newIndex->size; i++)
  {
    *sweeper = 0;
    sweeper++;
  }

  validateList();
  validateIndex();

  return result;
}


r_size_t rsize( void *block_ptr )
{
  r_size_t result = 0;

  validateList();
  validateList();

  if ( workingRegion != NULL)
    {
    Index *curr = workingRegion->index;

    while (curr != NULL)
    {
      if ( curr->location == block_ptr)
      {
        result = curr->size;
      }
      curr = curr->nextI;
    }
  }

  validateList();
  validateList();

  return result;
}


Boolean rfree( void *block_ptr )
{
    Boolean result = false;
    Index *curr = NULL;
    Index *prev = NULL;

    validateIndex();
    validateList();

    if (workingRegion != NULL)
    {

      prev = workingRegion->index;
      curr = workingRegion->index->nextI; //start from the first real index

      while (curr!= NULL) //search through the list 
      {
        if (curr->location == block_ptr)
        {
          prev->nextI = curr->nextI;
          result = true;
          curr->location = NULL;
          curr->size = 0;

          free(curr);
          curr = NULL; //create our while loop stopping condition
        }
        else //else has to contain while loop updating because otherwise curr->nextI is acting on NULL
        {
          curr = curr->nextI;
          prev = prev->nextI;
        }
      }
      
      
    }

    validateList(); //can't validate the index if there is no region selected'
    validateIndex();
    return result;
}

void rdestroy( const char *region_name )
{
  validateList();
  validateIndex();

  if (top != NULL) //make sure that there are nodes to check
  {

    if ( region_name == workingRegion->string)
    {
      workingRegion = workingRegion->next; //change the working region to the next one so we can delete this region
    }

    Boolean deleted = false;
    Node *curr = top; 
    Node *prev = NULL;  //need to keep track of previous to go over if we find the node
    Index *currIndex;
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
      currIndex = curr->index;
      while (currIndex != NULL)
      {
        prevIndex = currIndex;
        currIndex = currIndex->nextI;
        
        free(prevIndex); //get rid of all of the stuff inside
        prevIndex = NULL;
      }
      

      free( curr );
      curr = NULL;
      assert(!curr);

      region_name = NULL;
      deleted = true;
      numNodes--;
      curr = NULL;

    }

    workingRegion = top;  //point the working region to the top most node (region)
    validateList();       //postconds
    validateIndex();
  }
}


void rdump()
{

  if ( workingRegion != NULL) //make sure there is actually something to dump
  {
    validateList();
    validateIndex();


    Node *curr = top;
    Index *currIndex = NULL;
    double usedSpace = 0;
    int i = 1;

    if (curr != NULL)
    {
      while (curr != NULL)
      {
        printf("\n____| Region # %i: \"%s\" |___\n", i, curr->string); //print the title of the region, show index entries (blocks) below

        if (curr->index != NULL)
        {
          currIndex = curr->index;  //get the next regions index
          usedSpace = 0;            //reset the used space counter after each region has been through

          while (currIndex != NULL)
          {
            if ( currIndex->size != 0)
            {
              printf("|----%p: %i bytes.\n", currIndex->location, currIndex->size );
              usedSpace = usedSpace + currIndex->size;
            }
            currIndex = currIndex->nextI;
          }
          printf("Used: %i, Unused: %i\n", (int)usedSpace, (int)curr->regionSize-(int)usedSpace);
          printf("Free space: %.*f%% \n", 2, 100 - (usedSpace/curr->regionSize*100));

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
    validateIndex();
  }
  else 
  {
    printf("No regions created.\n");
  }
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
  }

  validateList(); //post conds

  return found;
}