#include "symtab.h"
#include <string.h>

// static does not mean the same as in Java.
// For global variables, it means they are only visible in this file.

// Pointer to stack of hash tables 
static hash_t **scopes;

// Pointer to array of values, to make it easier to free them
static symbol_t **values;

// Pointer to array of strings, should be able to dynamically expand as new strings
// are added.
static char **strings;

// Helper variables for manageing the stacks/arrays
static int32_t scopes_size = 16, scopes_index = -1;
static int32_t values_size = 16, values_index = -1;
static int32_t strings_size = 16, strings_index = -1;


void
symtab_init ( void )
{
    // doing sanity checks
    if(scopes_index == -1){
        scopes = malloc(scopes_size*sizeof(hash_t));
        scopes_index = 0;
    }
    if(values_index == -1){
        values = malloc(values_size*sizeof(symbol_t));
        values_index = 0;
    }
    if(strings_index == -1){
        strings = malloc(strings_size*sizeof(char*));
        strings_index = 0;
    }
}


void
symtab_finalize ( void )
{
    for(int i = 0; i <= scopes_index; i++){
        free(scopes[i]);
    }
    free(scopes);
    for(int i = 0; i <= values_index; i++){
        free(values[i]);
    }
    free(values);
    for(int i = 0; i <= strings_index; i++){
        free(strings[i]);
    }
    free(strings);
}


int32_t
strings_add ( char *str )
{
    if(strings_size == strings_index+1){
        // realloc, REALLOC!
        strings_size *= 2;
        strings = realloc(strings, strings_size*sizeof(char*));
    }
    strings[++strings_index] = str;
    return strings_index;

}


void
strings_output ( FILE *stream )
{
    fprintf(stream, ".data\n");
    fprintf(stream, ".INTEGER: .string \"%d \"\n");
    for(int i = 0; i <= strings_index; i++){
        fprintf(stream, ".STRING%i: .string \"%s\"\n");
    }
    fprintf(stream, ".global main\n");
}


void
scope_add ( void )
{
    if(scopes_size == scopes_index+1){
        // will overflow now, lets realloc!
        scopes_size *= 2;
        scopes = realloc(scopes, scopes_size*sizeof(hash_t));
    }
    scopes[++scopes_index] = ght_create(8);
}


void
scope_remove ( void )
{
    ght_finalize(scopes[scopes_index--]);
}


void
symbol_insert ( char *key, symbol_t *value )
{

    ght_insert(scopes[scopes_index], value, strlen(key), key);
    // Keep this for debugging/testing
    #ifdef DUMP_SYMTAB
        fprintf ( stderr, "Inserting (%s,%d)\n", key, value->stack_offset );
    #endif

}


symbol_t *
symbol_get ( char *key )
{
    symbol_t* result = ght_get(scopes[scopes_index], strlen(key), key);

// Keep this for debugging/testing
    #ifdef DUMP_SYMTAB
        if ( result != NULL )
            fprintf ( stderr, "Retrieving (%s,%d)\n", key, result->stack_offset );
    #endif
    return result;
}
