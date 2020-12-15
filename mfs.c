
// KRISHNA PATEL
// ID NUMBER: 1001273790

// INARA RUPANI
// ID NUMBER: 1001534052


#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

#define MAX_NUM_ARGUMENTS 5     // Mav shell only supports five arguments

uint16_t BPB_BytesPerSec;
uint8_t BPB_SecPerClus;
uint16_t BPB_RsvdSecCnt;
uint8_t BPB_NumFATS;
uint16_t BPB_RootEntCnt;
uint32_t BPB_FATSz32;

// For directories
typedef struct __attribute__((packed))  directoryentry{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstCluseterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstCluseterLow;
  uint32_t DIR_FileSize;
} DirectoryEntry;

/*
 * current sector number that points to block of data
 * returns : value of address for that block of data
 * finds starting address of the block of data
*/
int LBAToOffset(int32_t sector)
{
  return ((sector - 2) * BPB_BytesPerSec)   +(BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATS * BPB_FATSz32 * BPB_BytesPerSec);
}

/*
 * Given a logical block address, look up into the first FAT and
 * returns : the logical block address of the block in the file
 * -1 no further blocks
*/
int16_t NextLB(uint32_t sector, FILE *fp)
{
  uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector *4);
  int16_t val;
  fseek(fp,FATAddress, SEEK_SET);
  fread(&val, 2,1,fp);
  return val;
}
// I wrote my own function because
// something kept smashing stack
char upperCase(char c)
{
  if( (c >= 'a') && (c <='z'))
  {
    c = c -0x20;
  }
  return c;
}
// this stops junk from being printed
int validFileChar(char c)
{
  if( ((c >= 'A') && (c <='Z')) ||
      ((c >= 'a') && (c <='z')) ||
      ((c >= '0') && (c <= '9')))
    {
      return 1;
    }
    return 0;
}
void loadRequireData(FILE *fp)
{

  // gets BPB_BytesPerSec
  fseek(fp,11,SEEK_SET);
  fread(&BPB_BytesPerSec,2,1,fp);
  // sprintf(str,"BPB_BytesPerSec is: %d\nBPB_BytesPerSec is: %x\n\n",BPB_BytesPerSec,BPB_BytesPerSec);
  // printf("%s",str );

  // gets BPB_SecPerClus
  fseek(fp,13,SEEK_SET);
  fread(&BPB_SecPerClus,1,1,fp);
  // sprintf(str,"BPB_SecPerClus is: %d\nBPB_SecPerClus is: %x\n\n",BPB_SecPerClus,BPB_SecPerClus);
  // printf("%s",str );

  // gets BPB_RsvdSecCnt
  fseek(fp,14,SEEK_SET);
  fread(&BPB_RsvdSecCnt,2,1,fp);
  // sprintf(str,"BPB_RsvdSecCnt is: %d\nBPB_RsvdSecCnt is: %x\n\n",BPB_RsvdSecCnt,BPB_RsvdSecCnt);
  // printf("%s",str );

  // gets BPB_NumFATS
  fseek(fp,16,SEEK_SET);
  fread(&BPB_NumFATS,1,1,fp);
  // sprintf(str,"BPB_NumFATS is: %d\nBPB_NumFATS is: %x\n\n",BPB_NumFATS,BPB_NumFATS);
  // printf("%s",str );

  // gets BPB_RootEntCnt
  fseek(fp,17,SEEK_SET);
  fread(&BPB_RootEntCnt,2,1,fp);
  // printf("BPB_RootEntCnt is %d\n",BPB_RootEntCnt );

  // gets BPB_FATSz32
  fseek(fp,36,SEEK_SET);
  fread(&BPB_FATSz32,4,1,fp);
  // sprintf(str,"BPB_FATSz32 is: %d\nBPB_FATSz32 is: %x\n\n",BPB_FATSz32,BPB_FATSz32);
  // printf("%s",str );

}
FILE* openFAT32File(FILE* fp, char *fname)
{
  if(!fp)
  {
    if(fname == NULL)
    {
      printf("Enter a VALID fileName\n");
      return NULL;
    }
    else
    {
      fp = fopen(fname,"r");
      if(fp == NULL)
      {
        perror("Error opening file fileSystem\n");
        return NULL;
      }
      else
      {
        loadRequireData(fp);
        return fp;
      }
    }
  }
  else
  {
    printf("Error: File system image already open.\n" );
    return fp;
  }

  return fp;
}
// size of 2 fats + (reserved section size)
uint32_t root_cluster()
{
  uint32_t rootCluster = (BPB_NumFATS * BPB_FATSz32* BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
  return rootCluster;
}

void getDirectoryEntries(FILE *fp, DirectoryEntry *dir, uint32_t root_cluster)
{
  fseek(fp,root_cluster,SEEK_SET);
  fread(&dir[0],16,sizeof(DirectoryEntry),fp);
}
void printFiles(DirectoryEntry *dir)
{

  int i =0;
  for(i= 0; i <16; i++)
  {
    if(dir[i].DIR_Attr == 0x01 ||
       dir[i].DIR_Attr == 0x10  ||
       dir[i].DIR_Attr == 0x20)
   {
     char expanded_name[12];
     int j = 0;
     for (j =0 ; j <11; j++)
     {
       expanded_name[j] = dir[i].DIR_Name[j];
     }
     expanded_name[11] = 0;
     for( j = 0; j < 11; j++ )
     {
       expanded_name[j] = upperCase( expanded_name[j]);
     }
     if(validFileChar(expanded_name[0]))
     {
       printf("%s\n",expanded_name );
     }
   }
  }
}

// Converts the string to upperCase and expands the token
char* expanded_upperCase_name_compare(char* str, int size)
{
  // this character in the  fname determines what user is
  // requesting  file or folder
  int dotidx = -1;
  int i = 0;
  for(i = 0; i < 11;i++)
  {
    if(str[i] == '.')
    {
      dotidx = i;
      break;
    }
  }
  if(dotidx != -1)
  {
    char *expanded_name = malloc(sizeof(char) * 12);
    int idx =0;
    for(i = 0; i < (size - 4); i++)
    {
      expanded_name[i] = upperCase( str[idx]);
      idx++;
    }
    for(i=i; i < 8; i++ )
    {
      expanded_name[i] =' '; // place spaces
    }
    expanded_name[i] =' ';
    idx++; // skip the dot
    for(; i < 11;i++)
    {
      expanded_name[i] = upperCase( str[idx]);
      idx++;;
    }
    expanded_name[i] = 0;
    return expanded_name;
  }
  else // its a folder name
  {
    char *expanded_name = malloc(sizeof(char) * 12);
    for(i = 0; i < size; i++)
    {
      expanded_name[i] = upperCase( str[i]);
    }
    for(; i < 11;i++)
    {
      expanded_name[i] = ' ';
    }
    expanded_name[i] = 0;
    return expanded_name;
  }
}

void getStat(char* fname,DirectoryEntry *dir)
{
  int i =0;
  for(i =0; i<16;i++)
  {
    if(dir[i].DIR_Attr == 0x01 ||
       dir[i].DIR_Attr == 0x10 ||
       dir[i].DIR_Attr == 0x20)
   {

     char entryName[12];
     int idx =0;
     for (idx = 0; idx < 11; idx++)
     {
       entryName[idx] = dir[i].DIR_Name[idx];
     }
     entryName[idx] = 0;

     if(strcmp(fname,entryName) == 0)
     {
       printf("Attributes              [hex]    : %x\n",dir[i].DIR_Attr);
       printf("Size                    [hex]    : %x\n",dir[i].DIR_FileSize);
       printf("Starting Cluster Number [hex]    : %x\n",dir[i].DIR_FirstCluseterLow);

       return;
     }
   }
  }
  if( i == 16)
  {
    printf("Error: File not found\n" );
  }
}

int getfile(char *fname, DirectoryEntry *dir)
{
  int i =0;
  for(i =0; i<16;i++)
  {
    if(dir[i].DIR_Attr == 0x01 ||
       dir[i].DIR_Attr == 0x10 ||
       dir[i].DIR_Attr == 0x20)
   {

     char entryName[12];
     int idx =0;
     for (idx = 0; idx < 11; idx++)
     {
       entryName[idx] = dir[i].DIR_Name[idx];
     }
     entryName[idx] = 0;

     if(strcmp(fname,entryName) == 0)
     {
       return i;
      }
    }
  }
  if( i == 16)
  {
    return -1;
  }
  return -1;
}
void getInfo()
{
printf("BPB_BytesPerSec is: %d\n"
       "BPB_BytesPerSec is: %x\n\n",
       BPB_BytesPerSec,
       BPB_BytesPerSec);

printf("BPB_SecPerClus is: %d\n"
       "BPB_SecPerClus is: %x\n\n",
       BPB_SecPerClus,
       BPB_SecPerClus);
printf("BPB_RsvdSecCnt is: %d\n"
       "BPB_RsvdSecCnt is: %x\n\n",
        BPB_RsvdSecCnt,
        BPB_RsvdSecCnt);

printf("BPB_NumFATS is: %d\n"
       "BPB_NumFATS is: %x\n\n",
       BPB_NumFATS,
       BPB_NumFATS);
printf("BPB_FATSz32 is: %d\n"
       "BPB_FATSz32 is: %x\n\n",
       BPB_FATSz32,
       BPB_FATSz32);

}
int main()
{
  //****** MY CODE STARTS HERE ********//
  FILE *fp = NULL;
  uint32_t root_dir = 0;
  int inRoot = 0;
  DirectoryEntry dir[16];



  //-----------------------------------------------------------------------------
  //****** DEFAULT CODE HERE  ********//
  //******  Tokenize INPUT  ********//
  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str  = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // \TODO Remove this code and replace with your shell functionality
    /*
    int token_index  = 0;
    for( token_index = 0; token_index < token_count; token_index ++ )
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );
    }
    free( working_root );*/
//-----------------------------------------------------------------------------
//****** MY CODE STARTS HERE ********//

    char* command = token[0];
    // NULL command
    if(command == 0)
    {
    }
    else if((strcmp("open",command)==0) && (token_count == 3))
    {
      // fp = openFAT32File(fp,"fat32.img");
      //NOTE::: tHE BOTTOM LINE IS THE CORRECT ONE
      fp = openFAT32File(fp,token[1]);
      if(fp)
      {
        root_dir = root_cluster();
        inRoot = 1; // states we are in root dir
        getDirectoryEntries(fp,dir,root_dir);
      }
    }
    else if (strcmp("ls",command)==0)
    {
      if(fp)
      {
        printFiles(dir);
      }
      else
      {
        printf("Error: File system not open\n");
      }
    }
    else if (strcmp("close",command) == 0)
    {
      if(fp)
      {
        fclose(fp);
        fp = NULL;
      }
      else
      {
        printf("Error: File system not open\n");
      }
    }
    else if (strcmp("info",command) == 0)
    {
      if(fp)
      {
        getInfo();
      }
      else
      {
        printf("Error: File system not open\n");
      }
    }
    else if ((strcmp("stat",command) == 0) && (token_count == 3))
    {
      if(fp)
      {
        char *convertedToken =expanded_upperCase_name_compare(token[1],strlen(token[1]));
        getStat(convertedToken, dir);
        free(convertedToken);
      }
      else
      {
        printf("Error: File system not open\n");
      }
    }
    else if ((strcmp("get",command) == 0)  && (token_count == 3))
    {
      if(fp)
      {
        char *convertedToken = expanded_upperCase_name_compare(token[1],strlen(token[1]));
        int dirIdx = getfile(convertedToken,dir); // grabs the index for the directory entry
        // checks to see if file name being requested is valid
        if(dirIdx > -1)
        {
          FILE *writepointer;
          writepointer = fopen(token[1],"w");

          // set fp to where file starts and continue for bytes sizeof
          int startingAddr = LBAToOffset(dir[dirIdx].DIR_FirstCluseterLow);
          uint32_t fileSize = dir[dirIdx].DIR_FileSize;
          fseek(fp,startingAddr,SEEK_SET);
          int i =0;
          // grabs bytes and outputs file requested to current directory
          for(i =0; i < fileSize; i++)
          {
            char data;
            fread(&data,1,1,fp);
            fprintf(writepointer, "%c", data );
            fseek(fp,startingAddr+1+i,SEEK_SET);
          }
          fclose(writepointer);
        }
        else
        {
          printf("Error: File not found\n");
        }
        free(convertedToken);
      }
      else
      {
        printf("Error: File system not open\n");
      }

    }
    else if ((strcmp("read",command) == 0) && (token_count == 5))
    {
      if(fp)
      {
        char *convertedToken = expanded_upperCase_name_compare(token[1],strlen(token[1]));
        int dirIdx = getfile(convertedToken,dir); // grabs the index for the directory entry
        // checks to see if file name being requested is valid
        if(dirIdx > -1)
        {
          int offset = atoi(token[2]);
          int numBytes = atoi(token[3]);
          // set fp to where file starts and continue for bytes sizeof
          int startingAddr = LBAToOffset(dir[dirIdx].DIR_FirstCluseterLow)  + offset;
          fseek(fp,startingAddr,SEEK_SET);
          int i =0;
          // grabs bytes and outputs to terminal
          // verifying that it is a file and not a directory
          if(dir[dirIdx].DIR_Attr != 0x10)
          {
            for(i =0; i < numBytes; i++)
            {
              char data;
              fread(&data,1,1,fp);
              printf("%c",data ); // prints out a byte from the file
              fseek(fp,startingAddr+1+i,SEEK_SET);
            }
            printf("\n");
          }
        }
        free(convertedToken);
      }
      else
      {
        printf("Error: File system not open\n");
      }
    }
    else if ((strcmp("cd",command) == 0) && (token_count == 3))
    {
      if(fp)
      {
        // checks if you are parent directory or nawwwwwwwwwww
        if( (!inRoot) && (strcmp("..",token[1]) == 0) )
        {
          int indxing = 0;
          indxing = 0;
          while(indxing<16)
          {
            char byte1 =  dir[indxing].DIR_Name[0];
            char byte2 = dir[indxing].DIR_Name[1];
            if((byte1 == 0x2e) && (byte2 == 0x2e))
            {
              uint32_t startingAddr = (int32_t) LBAToOffset(dir[indxing].DIR_FirstCluseterLow);
              // In the root Dir ?
              if(startingAddr == 0x100000 )
              {
                root_dir = root_cluster();
                inRoot = 1; // states we are in root dir
                getDirectoryEntries(fp,dir,root_dir);
              }
              else
              {
                getDirectoryEntries(fp,dir,startingAddr);
                break;
              }
            }
            indxing++;
          }
        }
        else
        {
          // traversing down the directories [inside] them
          char *convertedToken = expanded_upperCase_name_compare(token[1],strlen(token[1]));
          int dirIdx = getfile(convertedToken,dir); // grabs the index for the directory entry

          // IS THIS A VALID DirectoryEntry
          if((dirIdx > -1) && (dir[dirIdx].DIR_Attr == 0x10))
          {
            // set fp to where file starts and continue for bytes sizeof
            uint32_t startingAddr = (int32_t) LBAToOffset(dir[dirIdx].DIR_FirstCluseterLow);
            inRoot = 0; // WE ARE NOT IN THE ROOT DIR
            getDirectoryEntries(fp,dir,startingAddr);
          }
          else
          {
            printf("Error: File not found\n");
          }
          free(convertedToken);
        }
      }
      else
      {
        printf("Error: File system not open\n");
      }
    }
    else if (strcmp("exit",command) == 0)
    {
      exit(0);
    }
    free( working_root );
  }
  return 0;
}
