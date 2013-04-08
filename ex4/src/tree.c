#include "tree.h"
#include "symtab.h"

static int current_offset;
static int current_depth;

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


node_t *
node_init ( node_t *nd, nodetype_t type, void *data, uint32_t n_children, ... )
{
    va_list child_list;
    *nd = (node_t) { type, data, NULL, n_children,
        (node_t **) malloc ( n_children * sizeof(node_t *) )
    };
    va_start ( child_list, n_children );
    for ( uint32_t i=0; i<n_children; i++ )
        nd->children[i] = va_arg ( child_list, node_t * );
    va_end ( child_list );
    return nd;
}


void
node_finalize ( node_t *discard )
{
    if ( discard != NULL )
    {
        free ( discard->data );
        free ( discard->children );
        free ( discard );
    }
}


void
destroy_subtree ( node_t *discard )
{
    if ( discard != NULL )
    {
        for ( uint32_t i=0; i<discard->n_children; i++ )
            destroy_subtree ( discard->children[i] );
        node_finalize ( discard );
    }
}




void
bind_names ( node_t *root )
{
    symbol_t *symbol = malloc(sizeof(symbol_t));
    switch(root->type.index){
        case FUNCTION_LIST:
            scope_add();
            current_depth = 0;
            for(int i = 0; i < root->n_children; i++){
                    symbol_t *child = malloc(sizeof(symbol_t));
                    child->label = &root->children[i]->data;
                    symbol_insert(child->label, child);
            }
            for(int i = 0; i < root->n_children; i++)
                if(root->children[i] != NULL)
                bind_names(root->children[i]);
            break;
        case FUNCTION:
            symbol->label = &root->data;
            scope_add();
            current_depth++;
            symbol_insert(symbol->label, symbol);
            for(int i = 0; i < root->n_children; i++){
                if(root->children[i] != NULL){
                    if(root->children[i]->type.index = PARAMETER_LIST){
                        int32_t local_offset = root->children[i]->n_children*4 + 8;
                        for(int j = 0; j < root->children[i]->n_children; j++){
                            if(root->children[i]->children[j] != NULL){
                                symbol_t *parameter = malloc(sizeof(symbol_t*));
                                parameter->stack_offset = local_offset;
                                local_offset -= 4;
                                parameter->depth = current_depth;
                                root->children[i]->children[j]->entry = &parameter;
                                symbol_insert(&root->children[i]->children[j]->data, parameter);
                            }
                        }
                    }
                    bind_names(root->children[i]);
                }
            }
            scope_remove();
            current_depth--;
            break;
        case BLOCK:
            scope_add();
            int local_offset = -4;
            current_offset = -4;
            for(int i = 0; i < root->n_children; i++)
                if(root->children[i] != NULL)
                    bind_names(root->children[i]);
            scope_remove();
            break;
    }
    
    
    for(int i = 0; i < root->n_children; i++)
        if(root->children[i] != NULL)
            bind_names(root->children[i]);
    

}
