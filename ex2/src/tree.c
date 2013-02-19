#include "tree.h"
#include <stdarg.h>


#ifdef DUMP_TREES
void
node_print ( FILE *output, node_t *root, uint32_t nesting )
{
    if ( root != NULL )
    {
        fprintf ( output, "%*c%s", nesting, ' ', root->type.text );
        if ( root->type.index == INTEGER )
            fprintf ( output, "(%d)", *((int32_t *)root->data) );
        if ( root->type.index == VARIABLE || root->type.index == EXPRESSION )
        {
            if ( root->data != NULL )
                fprintf ( output, "(\"%s\")", (char *)root->data );
            else
                fprintf ( output, "%p", root->data );
        }
        fputc ( '\n', output );
        for ( int32_t i=0; i<root->n_children; i++ )
            node_print ( output, root->children[i], nesting+1 );
    }
    else
        fprintf ( output, "%*c%p\n", nesting, ' ', root );
}
#endif


node_t * node_init ( node_t *nd, nodetype_t type, void *data, uint32_t n_children, ... ) {
    va_list args;
    va_start( args, n_children);
    struct n **children = (struct n **)malloc(n_children*sizeof(node_t));
    for(int i = 0; i < n_children; i++){
        *children[i] = va_arg(args, node_t);
    }
    va_end(args);
    
    node_t *node;
    node->type = type;
    node->data = data;
    node->n_children = n_children;
    node->children = children;
    return node;
}


void node_finalize ( node_t *discard ) {
   free(discard->data);
   free(discard->entry);
   free(discard->children);
}


void destroy_subtree ( node_t *discard ){
    for(int i = 0; i < discard->n_children; i++){
        node_finalize(discard->children[i]);
    }
    discard->n_children = 0;
}


