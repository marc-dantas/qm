#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <dirent.h> 
#include <termios.h>
#include <unistd.h>

// Color helpers
#define RESET  "\x1B[0m"  // Reset code, don't change this
#define RED    "\x1B[31m"
#define PURPLE "\x1B[35m"
#define UNDER  "\x1B[4m"  // Underline
#define CYAN   "\x1B[36m"
#define ITAL   "\x1B[3m"
#define GRAY   "\x1B[0;30m"

// Colors for each thing in qm, change each color according to your preference.
#define C_DIR      "\x1B[0;36m" // Color applied to directory names  
#define C_DIR_SEL  "\x1B[4;36m" // Color applied to selected directory  
#define C_FILE     "\x1B[0;3m"  // Color applied to file names  
#define C_FILE_SEL "\x1B[4;3m"  // Color applied to selected file  

// Maximum capacity of items (folders + files) that this explorer handles inside a folder
#define MAX_ITEMS 1024 // Should be good
#define MAX_FILENAME_LEN 256 // On most file systems, a file name can be up to 255 Unicode characters long

// Amount of items (folders + files) that can be rendered at once (Rolling window size in lines)
// Change this depending on your display size.
#define ROLLING_WINDOW_SIZE 12

typedef enum {
	ITEM_FILE = 0,
	ITEM_DIR
} ItemType;

typedef struct {
	char name[MAX_FILENAME_LEN];
	ItemType type;
	bool hidden;
} Item;

typedef struct {
	char path[260];
	Item items[MAX_ITEMS];
	int len, flen, dlen;
	int selection;
} qm_Instance;

void usage(char* program) {
	fprintf(stderr, "qm: usage: %s [PATH]\n", program);	
}

char readchar();
void reset();

int qm_open(qm_Instance* target, char* path) {
	DIR* d = opendir(path);
	if (d == NULL) {
		reset();
		fprintf(stderr, RED"qm: error: could not open directory \"%s\"\n"RESET, path);
		readchar();
		return 255;
	}

	target->len = 0;
	target->flen = 0;
	target->dlen = 0;

	struct dirent* dir;
	while ((dir = readdir(d)) != NULL && target->len < MAX_ITEMS) {
		if (dir->d_type == DT_DIR) target->dlen++;
		else                       target->flen++;

		Item i = {0};
		strncpy(i.name, dir->d_name, MAX_FILENAME_LEN);
		i.name[MAX_FILENAME_LEN] = '\0';
		i.type = (dir->d_type == DT_DIR) ? ITEM_DIR : ITEM_FILE;
		i.hidden = *dir->d_name == '.';


		target->items[target->len] = i;
		target->len += 1;
	}
	target->selection = 0;
	char* resolved_path = realpath(path, NULL);
	if (!resolved_path) {
	    reset();
	    fprintf(stderr, RED"qm: error: could not resolve realpath for \"%s\"\n"RESET, path);
	    readchar();
	    closedir(d);
	    return 255;
	}
	strncpy(target->path, resolved_path, sizeof(target->path) - 1);
	target->path[sizeof(target->path) - 1] = '\0'; // null-terminate just in case
	free(resolved_path);
	closedir(d);
	return 0;
}

void reset() {
	printf("\033[H\033[J");
    fflush(stdout);
}

char readchar() {
	struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);	
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

void qm_draw(qm_Instance* inst) {
	printf(PURPLE"%s"RESET"\n\n", inst->path);
	printf(CYAN"\t%d directories"RESET, inst->dlen);
	if (inst->flen == 0)      printf(CYAN"\tno files"RESET"\n");
	else if (inst->flen == 1) printf(CYAN"\t%d file"RESET"\n", inst->flen);		
	else                      printf(CYAN"\t%d files"RESET"\n", inst->flen);
	printf("\n");

	int window = ROLLING_WINDOW_SIZE;
	int start = inst->selection - (window / 2);
	if (start < 0) start = 0;
	if (start + window > inst->len) start = inst->len - window;
	if (start < 0) start = 0;

	if (start > 0) printf(GRAY" +\t"PURPLE"..."RESET"\n");

	int render_count = 0;

	for (int i = start; i < inst->len && render_count < window; ++i) {
		Item it = inst->items[i];
		printf(" "GRAY"|"RESET"\t");
		switch (it.type) {
		case ITEM_DIR:
			if (inst->selection == i) printf(RED">> "RESET C_DIR_SEL"%s"RESET"/", it.name);
			else                      printf("   "C_DIR"%s"RESET"/",     it.name);
			break;
		case ITEM_FILE:
			if (inst->selection == i) printf(RED">> "RESET C_FILE_SEL"%s"RESET, it.name);
			else                      printf("   "C_FILE"%s"RESET,     it.name);	
			break;
		}
		printf("\n");
		render_count++;
	}
	
	if (start + window < inst->len) printf(GRAY" +\t"PURPLE"..."RESET"\n");
}

void qm_enter(qm_Instance* target, char* path) {
	char new_path[260];
    snprintf(new_path, sizeof(new_path), "%s/%s", target->path, path);
    if (qm_open(target, new_path) == 0) {
        chdir(new_path);
    }
}

int main(int argc, char** argv) {
	char p[1024];
	getcwd(p, sizeof(p));
	char* path = argc >= 2 ? argv[1] : p;

	qm_Instance inst = {0};
	if (qm_open(&inst, path) != 0) return 255;
	chdir(path);
	for (;;) {
		reset();
		qm_draw(&inst);
		switch (readchar()) {
		case 'w':
			if (inst.selection > 0) {
				inst.selection -= 1;
			}
			break;
		case 'd':
		    if (inst.items[inst.selection].type == ITEM_DIR) {
				qm_enter(&inst, inst.items[inst.selection].name);
	        }
	        break;
		case 'h':
			reset();
			printf(PURPLE"HELP"RESET"\n");
			printf("  - "CYAN"Navigation"RESET":\n");
			printf("      ["RED"W"RESET"] Move selection up\n");
			printf("      ["RED"A"RESET"] Go back\n");
			printf("      ["RED"S"RESET"] Move selection down\n");
			printf("      ["RED"D"RESET"] Enter folder under selection\n");
			printf("  - "CYAN"Application"RESET":\n");
			printf("      ["RED"Q"RESET"] Quit application\n");
			printf("      ["RED"H"RESET"] Show this help message\n");
			printf("  - "CYAN"Utility"RESET":\n");
			printf("      ["RED"R"RESET"] Run an arbitrary terminal command\n");
			printf("      ["RED"ENTER"RESET"] Launch desired application with selected file\n");
			printf(UNDER"\n(c) 2025 Marcio Dantas"RESET"\n");
			printf(ITAL"Press any key to continue..."RESET"\n");
			readchar();
			break;
		case 'a':
			qm_enter(&inst, "..");
			break;
		case 's':
			if (inst.selection+1 < inst.len) {
				inst.selection += 1;
			}
			break;
		case '\n':
			if (inst.items[inst.selection].type == ITEM_FILE) {
				char* filename = inst.items[inst.selection].name;
				printf(CYAN"Program to launch with '"RESET C_FILE_SEL"%s"RESET CYAN"'"RESET": ", filename);
				char program[32];
				fgets(program, 32, stdin);
				program[strcspn(program, "\n")] = 0;
				char cmd[1024];
				sprintf(cmd, "%s \"%s\"", program, filename);
				reset();
				system(cmd);
				printf(ITAL"\nPress any key to continue..."RESET"\n");
				readchar();
			}
			break;
		case 'r':
			printf(CYAN"Run"RESET": ");
			char cmd[255];
			fgets(cmd, 255, stdin);
			cmd[strcspn(cmd, "\n")] = 0;
			reset();
			system(cmd);
			printf(ITAL"\nPress any key to continue..."RESET"\n");
			readchar();
			break;
		case 'q':
			reset();
		    return 0;
		}
	}
	return 0;
}
