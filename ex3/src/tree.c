#include "tree.h"


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


node_t* simplify_tree ( node_t* node )
{

    if ( node != NULL ){

        // Recursively simplify the children of the current node
        for ( uint32_t i=0; i<node->n_children; i++ ){
            node->children[i] = simplify_tree ( node->children[i] );
        }

        // After the children have been simplified, we look at the current node
        // What we do depend upon the type of node
        switch ( node->type.index )
        {
            // These are lists which needs to be flattened. Their structure
            // is the same, so they can be treated the same way.
            case FUNCTION_LIST: case STATEMENT_LIST: case PRINT_LIST:
            case EXPRESSION_LIST: case VARIABLE_LIST:
                if(node->n_children > 1){
                    node_t *temp, *simplified;
                    temp = node->children[1];
                    simplified = simplify_tree(node->children[0]);
                    node->children = realloc(node->children, (1 + simplified->n_children)*sizeof(node_t));
                    for(int i = 0; i < simplified->n_children; i++){
                        node->children[i] = simplified->children[i];
                    }
                    node->children[simplified->n_children] = temp;

                }
                else{
                    node->type = node->children[0]->type;
                    node->data = node->children[0]->data;
                    node->entry = node->children[0]->entry;
                    node->n_children = 0;
                    //destroy_subtree(node->children[0]);
                }
                break;


            // Declaration lists should also be flattened, but their stucture is sligthly
            // different, so they need their own case
            case DECLARATION_LIST:
                break;

            
            // These have only one child, so they are not needed
            case STATEMENT: case PARAMETER_LIST: case ARGUMENT_LIST:
                node->type = node->children[0]->type;
                node->data = node->children[0]->data;
                node->entry = node->children[0]->entry;
                node->n_children = 0;
                //destroy_subtree(node->children[0]);
                break;


            // Expressions where both children are integers can be evaluated (and replaced with
            // integer nodes). Expressions whith just one child can be removed (like statements etc above)
            case EXPRESSION:
                if(node->n_children = 1){
                    node->type = node->children[0]->type;
                    node->data = node->children[0]->data;
                    node->entry = node->children[0]->entry;
                    node->n_children = 0;
                    //destroy_subtree(node->children[0]);
                }
                else if( node->n_children = 2){
                    if(node->children[0]->type.index == INTEGER && node->children[0]->type.index == INTEGER){
                        node->type = node->children[0]->type;
                        int *a = (node->children[0]->data);
                        int *b = (node->children[0]->data);
                        int *result =  malloc(sizeof(int));
                        {
                        switch(((char *)node->data)[0]){
                            case '+':
                                *result = *a + *b;
                                break;
                            case '-':
                                *result = *a - *b;
                                break;
                            case '*':
                                *result = (*a) * (*b);
                                break;
                            case '/':
                                *result = (*a) / (*b);
                                break;
                        }
                        }
                        node->data = &result;
                        node->entry = node->children[0]->entry;
                        node->n_children = 0;
                        //destroy_subtree(node->children[0]);
                        //destroy_subtree(node->children[1]);


                    }

                }
                break;

        }
    }
    return node;
}
