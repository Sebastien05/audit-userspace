
/* anomyzing_config.c --
 *
 * Authors:
 *   Sebastienb LEFEVRE
 *
 */

#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#define SUCCESS 0
#define MAX_NODE_PATH_SIZE 32
#define MAX_LINE_SIZE 64
#define MAX_CHILDREN 128
#define MAX_DIR_DEPTH 16

#define PATH_FILLING 0
#define TAG_FILLING 1

#define SKIP_NEW_LINE 5
#define BAD_CONFIGURATION 6
#define ERROR_PARSING 7
#define ERROR_CATENATE 8
#define ERROR_CATENATE 8
#define ERROR_NODE_NOT_FOUND 9


typedef struct node {
        char nodePath[MAX_NODE_PATH_SIZE];
        struct node *children[MAX_CHILDREN];
        char pathTag[MAX_NODE_PATH_SIZE];
        int childrenNumber;
        int depth;
} TreePath;
// Add indicator that only this pathTag will be displayed at replacement

typedef struct cfg_t {
        int depth;
        char nodePath [MAX_NODE_PATH_SIZE];
        char pathTag [MAX_NODE_PATH_SIZE];
} ConfigLine;

typedef struct ctx_t {
        int last_depth;
        char fullPath[MAX_DIR_DEPTH][MAX_NODE_PATH_SIZE];
} BuildCtx;

char sep[] = "-";
