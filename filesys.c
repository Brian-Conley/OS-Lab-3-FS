// ACADEMIC INTEGRITY PLEDGE
//
// - I have not used source code obtained from another student nor
//   any other unauthorized source, either modified or unmodified.
//
// - All source code and documentation used in my program is either
//   my original work or was derived by me from the source code
//   published in the textbook for this course or presented in
//   class.
//
// - I have not discussed coding details about this project with
//   anyone other than my instructor. I understand that I may discuss
//   the concepts of this program with other students and that another
//   student may help me debug my program so long as neither of us
//   writes anything during the discussion or modifies any computer
//   file during the discussion.
//
// - I have violated neither the spirit nor letter of these restrictions.
//
//
//
// Signed: Brian Conley Date: 12 Dec. 2024

//filesys.c
//Based on a program by Michael Black, 2007
//Revised 11.3.2020 O'Neil

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

const int MAX_FILE_SIZE = 12288;

typedef struct {
    char name[8];
    char type;
    int dirIndex;
    int startSector;
    int numSectors;
} file_metadata_t;

file_metadata_t* findFile(char* dir, char* filename);
int findAvailableDirEntry(char* dir);
int findAvailableSector(char* map);
void printDir(char* dir);
void printDiskMap(char* map);

int main(int argc, char* argv[])
{
	int i, j, size, noSecs, startPos;

	//open the floppy image
	FILE* floppy;
	floppy=fopen("floppya.img","r+");
	if (floppy==0)
	{
		printf("floppya.img not found\n");
		return 0;
	}

	//load the disk map from sector 256
	char map[512];
	fseek(floppy,512*256,SEEK_SET);
	for(i=0; i<512; i++)
		map[i]=fgetc(floppy);

	//load the directory from sector 257
	char dir[512];
	fseek(floppy,512*257,SEEK_SET);
	for (i=0; i<512; i++)
		dir[i]=fgetc(floppy);

    // check if program is called correctly
    if (argc <= 1) {
        printf("Usage: %s <option> <argument>\n", argv[0]);
        fclose(floppy);
        return 0;
    }

    // Option D
    if (strcmp(argv[1], "D") == 0) {
        if (argc != 3) {
            printf("Usage: %s D <filename>\n", argv[0]);
            fclose(floppy);
            return 0;
        }

        // check if file exists
        file_metadata_t* fileData = findFile(dir, argv[2]);
        if (!fileData) {
            printf("Error: File \"%s\" not found\n", argv[2]);
            fclose(floppy);
            return 0;
        }

        // mark space as free
        for (int i = 0; i < 16; ++i) {
            dir[16*fileData->dirIndex + i] = 0;
        }
        for (int i = fileData->startSector; i < fileData->startSector + fileData->numSectors; ++i)
            map[i] = 0;
    }

    // Option L
    if (strcmp(argv[1], "L") == 0) {
        if (argc > 2) {
            printf("Usage: %s L", argv[0]);
            fclose(floppy);
            return 0;
        }
        printDir(dir);
    }

    // Option M
    if (strcmp(argv[1], "M") == 0) {
        if (argc != 3) {
            printf("Usage: %s M <filename>\n", argv[0]);
            fclose(floppy);
            return 0;
        }

        // truncate long file name
        char filename[9];
        strncpy(filename, argv[2], 8);

        // check if file exists
        file_metadata_t* fileData = findFile(dir, filename);
        if (fileData) {
            printf("Error: File \"%s\" already exists\n", filename);
            free(fileData);
            fileData = NULL;
            fclose(floppy);
            return 0;
        }

        // check if directory has space for file
        int dirEntryIndex = findAvailableDirEntry(dir);
        if (dirEntryIndex == -1) {
            printf("Error: Insufficient disk space\n");
            fclose(floppy);
            return 0;
        }

        // check if disk has a free sector
        int firstAvailableSector = findAvailableSector(map);
        if (firstAvailableSector == -1) {
            printf("Error: Insufficient disk space\n");
            fclose(floppy);
            return 0;
        }

        // get text input
        char fileContents[512];
        printf("Enter up to 512 bytes of text\n");
        if (fgets(fileContents, sizeof(fileContents), stdin) == NULL) {
            printf("Error: User input failed\n");
            fclose(floppy);
            return 0;
        }

        // write filename to directory
        for (int i = 0; i < 8; ++i) {
            if (filename[i] == '\0') {
                break;
            }
            dir[16*dirEntryIndex + i] = filename[i];
        }

        // write file type 't' to directory
        dir[16*dirEntryIndex + 8] = 't';

        // write sector information to directory
        dir[16*dirEntryIndex + 9] = firstAvailableSector;
        dir[16*dirEntryIndex + 10] = 1;

        // mark sector used on map
        map[firstAvailableSector] = 0xFF;

        // write file to disk
        fseek(floppy, 512*firstAvailableSector, SEEK_SET);
        for (i = 0; i < 512; ++i) fputc(fileContents[i], floppy);
    }

    // Option P
    if (strcmp(argv[1], "P") == 0) {
        if (argc != 3) {
            printf("Usage: %s P <filename>\n", argv[0]);
            fclose(floppy);
            return 0;
        }

        // get file
        file_metadata_t* fileData = findFile(dir, argv[2]);

        // check if file exists (this also flags for invalid names)
        if (!fileData) {
            printf("Error: File \"%s\" not found\n", argv[2]);
            fclose(floppy);
            return 0;
        }

        // check if file is printable (not an executable)
        if (fileData->type == 'x' || fileData->type == 'X') {
            printf("Error: File \"%s\" is an executable\n", fileData->name);
            fclose(floppy);
            return 0;
        }

        // read file into buffer
        char file[MAX_FILE_SIZE];
        fseek(floppy, 512*fileData->startSector, SEEK_SET);
        for (i = 0; i < 512*fileData->numSectors; ++i)
            file[i]=fgetc(floppy);

        // print file buffer
        for (i = 0; i < MAX_FILE_SIZE; ++i) {
            if (file[i] == 0) break;
            printf("%c", file[i]);
        }

        free(fileData);
        fileData = NULL;
    }

	//write the map and directory back to the floppy image
    fseek(floppy,512*256,SEEK_SET);
    for (i=0; i<512; i++) fputc(map[i],floppy);

    fseek(floppy,512*257,SEEK_SET);
    for (i=0; i<512; i++) fputc(dir[i],floppy);

	fclose(floppy);
}

/*
 * Find a file in the directory
 * @param dir       char array of directory contents
 * @param filename  name of printed file
 * @retval          pointer to struct with file information
 *                  NULL if an error has occurred
 */
file_metadata_t* findFile(char* dir, char* filename) {

    int i, j;

    // check if filename length is valid
    if (strlen(filename) > 8) {
        return NULL;
    }

    // check if filename is not empty
    if (strlen(filename) == 0) {
        return NULL;
    }

    // Search dir for file name
    for (i = 0; i < 512; i+=16) {
        if (dir[i] == 0) break;

        // check if filename length is shorter than filename in directory
        if (strlen(filename) < 8)
            if (dir[i+strlen(filename)] != 0) continue;

        // check if filename length is longer than filename in directory
        if (dir[i+strlen(filename)-1] == 0) continue;

        // check if filename matches filename in directory
        bool filenameMatch = true;
        for (j = 0; j < strlen(filename); ++j) {
            if (filename[j] != dir[i+j]) filenameMatch = false;
        }
        if (!filenameMatch) continue;

        // get file information
        file_metadata_t* fileData = malloc(sizeof(file_metadata_t));
        strcpy(fileData->name, filename);
        fileData->type = dir[i+8];
        fileData->dirIndex = i/16;
        fileData->startSector = dir[i+9];
        fileData->numSectors = dir[i+10];

        return fileData;
    }
    // File not found
    return NULL;
}

/*
 * Find the first slot available in dir
 * @param   dir       char array of directory contents
 *                    dir MUST be 512 bytes
 * @retval  directory index of first available slot
 *          -1 if no slot available
 */
int findAvailableDirEntry(char* dir) {
    
    // Search for empty slot
    for (int i = 0; i < 512; i+=16) {
        if (dir[i] == 0) {
            return i/16;
        }
    }
    // No slot was found
    return -1;
}

/*
 * Find the first available sector using the map
 * This assumes a max of 511 sectors
 * @param   map     char array represeting a map of sector usage
 * @retval  map index of first available sector
 *          -1 if full
 */
int findAvailableSector(char* map) {

    if (strlen(map) >= 511) {
        return -1;
    }
    // First \0 in map
    return strlen(map);
}

// print directory
void printDir(char* dir) {

    int i, j;

    int diskSpaceUsed = 0;
	printf("\nDisk directory:\n");
    for (i=0; i<512; i+=16) {
        if (dir[i]==0) break;
        for (j=0; j<8; ++j) {
            if (dir[i+j] != 0) printf("%c", dir[i+j]);
        }
        printf(".%c %6d bytes\n", dir[i+8], 512*dir[i+10]);
        diskSpaceUsed += 512*dir[i+10];
    }
    printf("%d / %d bytes used\n", diskSpaceUsed, 511*512);
}

//print disk map
void printDiskMap(char* map) {

    int i, j;

	printf("Disk usage map:\n");
	printf("      0 1 2 3 4 5 6 7 8 9 A B C D E F\n");
	printf("     --------------------------------\n");
	for (i=0; i<16; i++) {
		switch(i) {
			case 15: printf("0xF_ "); break;
			case 14: printf("0xE_ "); break;
			case 13: printf("0xD_ "); break;
			case 12: printf("0xC_ "); break;
			case 11: printf("0xB_ "); break;
			case 10: printf("0xA_ "); break;
			default: printf("0x%d_ ", i); break;
		}
		for (j=0; j<16; j++) {
			if (map[16*i+j]==-1) printf(" X"); else printf(" .");
		}
		printf("\n");
	}
}

