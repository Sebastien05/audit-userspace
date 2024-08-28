/* anomyzing_config.c --
 *
 * Authors:
 *   Sebastien LEFEVRE
 *
 */

#include "obfuscator_config.h"
#include <syslog.h>

char sep[] = "-";

void InitConfigLine(ConfigLine *cfg) {
	cfg->depth=0;
	memset(cfg->nodePath, 0, sizeof(cfg->nodePath));
	memset(cfg->pathTag, 0, sizeof(cfg->pathTag));
}

void InitBuildCtx(BuildCtx *ctx) {
	ctx->last_depth=0;
	memset(ctx->fullPath, 0, sizeof(ctx->fullPath));
}

void InitNode(TreePath *node) {
	memset(node, 0, sizeof(TreePath));
}
/*
 * Search by node path in the current children array
 * TODO: Sort each children array at build and use bsearch
 * to find quickly the children node
 */
TreePath *search_node(const char path[MAX_NODE_PATH_SIZE], TreePath *tree)
{
	for (int i=0; i<tree->childrenNumber; i++) {
		TreePath *node = tree->children[i];
		if (strncmp(node->nodePath, path, MAX_NODE_PATH_SIZE)==0)
		       return node;
	}
	return NULL;
}

/*
 * Print recursively the tree path
 */
void print_tree_path(TreePath *tree)
{
	for (int i=0; i<(tree->depth-1)*5; i++)
		printf(" ");
	if (tree->depth != 0)
		printf("\\___ ");
	if (strcmp(tree->nodePath, "/"))
		printf("%s: %s\n", tree->nodePath, tree->pathTag);
	for (int i=0; i<tree->childrenNumber; i++)
		print_tree_path(tree->children[i]);
}

/*
 * Build the res chain char in order to replace the fullpath
 *
*/
int replace_path(char *res, const char fullPath[MAX_DIR_DEPTH][MAX_NODE_PATH_SIZE],  TreePath *tree)
{
	if (tree->depth >= MAX_DIR_DEPTH) {
		return ERROR_PARSING;
	} else if (fullPath[tree->depth+1][0] == '\0' && tree->depth!=0) {
		return SUCCESS;
	} else {
		TreePath *child_node = search_node(fullPath[tree->depth+1], tree);
		if (child_node == NULL) {
			return ERROR_NODE_NOT_FOUND;
		} else {
			// Compute available bytes (avb) in res and truncate if it's necessary
			int s_tag = strlen(child_node->pathTag);
			int s_sep = strlen(sep);
			int s_res = strlen(res);
			int avb = MAX_LINE_SIZE - (s_res + s_sep + s_tag);
			avb = (avb < 0 ? (s_sep + s_tag - avb) : s_sep + s_tag);

			// Concatenate separator not at beginning and if the tag is not empty 
			if (s_res != 0 && s_tag > 0)
				strncat(res, sep, (avb-s_tag));

			// Concatenate the node's pathTag name
			strncat(res, child_node->pathTag, (avb-s_sep));
			return replace_path(res, fullPath, child_node);
		}
	}
}

/*
 * Traverses the tree and inserts a leaf node
 * The condition to insert is to reach the depth-1
 */
int insert_node_path(const ConfigLine *cfg, const char fullPath[MAX_DIR_DEPTH][MAX_NODE_PATH_SIZE], TreePath *tree)
{
	if (cfg->depth>0 && cfg->depth-1 != tree->depth) {
		if (tree->depth>=MAX_DIR_DEPTH) {
			printf("This node exceed tha maximum depth\n");
			return -1;
		}
		// Retrieve the next node
		char node_to_search[MAX_NODE_PATH_SIZE]={0};
		strcpy(node_to_search, fullPath[tree->depth+1]);
		TreePath *next = search_node(node_to_search, tree);
		if (next==NULL) {
			printf("Internal error, intermediate node not found\n");
			return -1;
		}
		// Go recursively until the last part of the path
		return insert_node_path(cfg, fullPath, next);
	} else {
		if (tree->childrenNumber > MAX_CHILDREN) {
			printf("Maximum of children exceed for this node\n");
			return -1;
		}
		// Initialize the new node
		TreePath *new_leaf = malloc(sizeof(TreePath));
		if (new_leaf == NULL) {
			printf("Failed to allocate a new node\n");
			return -1;
		}
		InitNode(new_leaf);
		new_leaf->depth=cfg->depth;
		strcpy(new_leaf->nodePath, cfg->nodePath);
		strcpy(new_leaf->pathTag, cfg->pathTag);

		// Update the parent node
		tree->children[tree->childrenNumber] = new_leaf;
		tree->childrenNumber+=1;
	}
	return 0;
}

int iseol(char c) {
	return (c == '\0' || c == '\n');
}

/*
 * Parse a line to retrieve a normalized data
 *  - depth: number of '>'
 *  - nodePath: sub or complete path to the ressource
 *  - pathTag: the data anonymized
 */
int parse_line(char chunk[MAX_LINE_SIZE], ConfigLine *cfg) {

	int depth=1; // We start by 1 because root is 0
	char nodePath[MAX_NODE_PATH_SIZE] = {0};
	char pathTag[MAX_NODE_PATH_SIZE] = {0};

	int i=0;
	if (iseol(chunk[0]))
		return SKIP_NEW_LINE;
	for (; i<MAX_LINE_SIZE; i++) {
		if (chunk[i] == '>')
			depth++;
		else if (!isprint(chunk[i]))
			return BAD_CONFIGURATION;
		else
			break;
	}
	// Nothing is following after one or multiple '>'
	if (depth > 0 && iseol(chunk[i]))
		return BAD_CONFIGURATION;

	int j=0, state=PATH_FILLING;
	for (; i<MAX_LINE_SIZE; i++) {

		if (chunk[i] != ':' && !iseol(chunk[i]) && state==PATH_FILLING) {
			// Fill the node path
			nodePath[j]=chunk[i];
		} else {
			if (strlen(nodePath)==0)
				return BAD_CONFIGURATION;
			if (chunk[i] == ':') {
				state=TAG_FILLING;
				j=-1;
			} else {
				if (iseol(chunk[i]))
					break;
				// Fill the renaming part
				pathTag[j]=chunk[i];
			}
		} j++;
	}
	cfg->depth=depth;
	strcpy(cfg->nodePath, nodePath);
	strcpy(cfg->pathTag, pathTag);
	return 0;
}

/*
	Free recursivly a tree structure until the root node (included)
*/
void free_config(TreePath *tree) {
	// Free children node recursivly
	for(int i=0; i<tree->childrenNumber; i++) {
		if (tree->children[i]) {
			free_config(tree->children[i]);
			free(tree->children[i]);
		}
	}
	// When it comes to free root node
	if (!strncmp(tree->nodePath, "/\0",2))
		free(tree);
}

/*
 * Build the tree path which will be use for anonyzing path during event compute
 */
TreePath *load_config(const char *filename) {

	FILE *fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("fopen");
		fclose(fp);
		return NULL;
	}

	// Initialize root node
	TreePath *tree = malloc(sizeof(TreePath));
	if (tree == NULL) {
		printf("Memory allocation failed\n");
		fclose(fp);
		return NULL;
	}
	strcpy(tree->nodePath, "/");

	ConfigLine	*cfg = (ConfigLine *)malloc(sizeof(ConfigLine));
	BuildCtx 	*ctx = (BuildCtx *)malloc(sizeof(BuildCtx));

	if (cfg == NULL || ctx == NULL) {
		printf("Memory allocation failed\n");
		fclose(fp);
		free(tree);
		return NULL;
	}

	InitConfigLine(cfg);
	InitBuildCtx(ctx);

	int	ret=0, ln=1;
	char chunk[MAX_LINE_SIZE] = {0};

	while (fgets(chunk, sizeof(chunk), fp) != NULL) {

		ret = parse_line(chunk, cfg);
		if (ret==BAD_CONFIGURATION) {
			free_config(tree);
			tree=NULL;
			break;
		} else if (ret==SKIP_NEW_LINE) {
			continue;
		} else {

			if (cfg->depth < 0) {
				printf("Error at line %d: internal error, negative depth !\n", ln);
				free_config(tree);
				tree=NULL;
				break;
			}
			if (cfg->depth >= MAX_DIR_DEPTH) {
				printf("Error at line %d: too much depth !\n", ln);
				free_config(tree);
				tree=NULL;
				break;
			}
			if (cfg->depth > ctx->last_depth + 1) {
				printf("Error at line %d: too much depth according to last one !\n", ln);
				free_config(tree);
				tree=NULL;
				break;
			}
			printf("=== Config Line: <depth %d> %s:%s\n", cfg->depth, cfg->nodePath, cfg->pathTag);

			// Insert a new node according to cfg values and his full path
			// Old data fullPath[i] with i>depth are not cleanned because we stop by depth value
			if (insert_node_path(cfg, ctx->fullPath, tree)) {
				free_config(tree);
				tree=NULL;
				break;
			}
			// Copy the new node path name at his proper depth
			strcpy(ctx->fullPath[cfg->depth], cfg->nodePath);
		}
		ctx->last_depth=cfg->depth;
		ln++;
	}
	free(cfg);
	free(ctx);
	fclose(fp);
	return tree;
}

/*
	Serialize the path file to an array from splited directory/file
*/
void serialize_path(char fullPath[MAX_DIR_DEPTH][MAX_NODE_PATH_SIZE], char* path) {

	char delim[] = "/";
	int depth=1; // depth 0 is root, therefore this part of path is empty
	char *raw_path = strdup(path);
	if (raw_path != NULL) {
		char *ptr = strtok(raw_path, "/");
		while (ptr != NULL) {
			strncpy(fullPath[depth++], ptr, MAX_NODE_PATH_SIZE);
			ptr = strtok(NULL, delim);
		}
		free(raw_path);
	}
}

int main_test(int argc, char *argv[]) {
	if (argc !=3) {
		printf("Bad number of argument\n");
		return EINVAL;
	}
	int ret = 0;
	struct stat buffer;
	if (stat(argv[1], &buffer) != 0) {
		printf("%s file doesn't exist\n", argv[1]);
		return ENOENT;
	}
	TreePath *tree = load_config(argv[1]);
	if (!tree)
		return EINVAL;

	char res[MAX_LINE_SIZE] = {0};
	char fullPath[MAX_DIR_DEPTH][MAX_NODE_PATH_SIZE] = {0};
	serialize_path(fullPath, argv[2]);

	replace_path(res, fullPath, tree);
	printf("Old path = %s\nNew path = %s\n", argv[2], res);

	free_config(tree);

	if (ret != 0)
		printf("Internal error: error during loading configuration..\n");
	return 0;
}
