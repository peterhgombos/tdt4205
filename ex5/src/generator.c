#include <tree.h>
#include <generator.h>
#include <stdio.h>

bool peephole = false;


/* Elements of the low-level intermediate representation */

/* Instructions */
typedef enum {
    STRING, LABEL, PUSH, POP, MOVE, CALL, SYSCALL, LEAVE, RET,
    ADD, SUB, MUL, DIV, JUMP, JUMPZERO, JUMPNONZ, DECL, CLTD, NEG, CMPZERO, NIL,
    CMP, SETL, SETG, SETLE, SETGE, SETE, SETNE, CBW, CWDE,JUMPEQ
} opcode_t;

/* Registers */
static char
*eax = "%eax", *ebx = "%ebx", *ecx = "%ecx", *edx = "%edx",
    *ebp = "%ebp", *esp = "%esp", *esi = "%esi", *edi = "%edi",
    *al = "%al", *bl = "%bl";

/* A struct to make linked lists from instructions */
typedef struct instr {
    opcode_t opcode;
    char *operands[2];
    int32_t offsets[2];
    struct instr *next;
} instruction_t;

/* Start and last element for emitting/appending instructions */
static instruction_t *start = NULL, *last = NULL;

/*
 * Track the scope depth when traversing the tree - init. value may depend on
 * how the symtab was built
 */ 
static int32_t depth = 1;

/* Prototypes for auxiliaries (implemented at the end of this file) */
static void instruction_add ( opcode_t op, char *arg1, char *arg2, int32_t off1, int32_t off2 );
static void instructions_print ( FILE *stream );
static void instructions_finalize ( void );


/*
 * Convenience macro to continue the journey through the tree - just to save
 * on duplicate code, not really necessary
 */
#define RECUR() do {\
    for ( int32_t i=0; i<root->n_children; i++ )\
    generate ( stream, root->children[i] );\
} while(false)

/*
 * These macros set implement a function to start/stop the program, with
 * the only purpose of making the call on the first defined function appear
 * exactly as all other function calls.
 */
#define TEXT_HEAD() do {\
    instruction_add ( STRING,       STRDUP("main:"), NULL, 0, 0 );      \
    instruction_add ( PUSH,         ebp, NULL, 0, 0 );                  \
    instruction_add ( MOVE,         esp, ebp, 0, 0 );                   \
    instruction_add ( MOVE,         esp, esi, 8, 0 );                   \
    instruction_add ( DECL,         esi, NULL, 0, 0 );                  \
    instruction_add ( JUMPZERO,     STRDUP("noargs"), NULL, 0, 0 );     \
    instruction_add ( MOVE,         ebp, ebx, 12, 0 );                  \
    instruction_add ( STRING,       STRDUP("pusharg:"), NULL, 0, 0 );   \
    instruction_add ( ADD,          STRDUP("$4"), ebx, 0, 0 );          \
    instruction_add ( PUSH,         STRDUP("$10"), NULL, 0, 0 );        \
    instruction_add ( PUSH,         STRDUP("$0"), NULL, 0, 0 );         \
    instruction_add ( PUSH,         STRDUP("(%ebx)"), NULL, 0, 0 );     \
    instruction_add ( SYSCALL,      STRDUP("strtol"), NULL, 0, 0 );     \
    instruction_add ( ADD,          STRDUP("$12"), esp, 0, 0 );         \
    instruction_add ( PUSH,         eax, NULL, 0, 0 );                  \
    instruction_add ( DECL,         esi, NULL, 0, 0 );                  \
    instruction_add ( JUMPNONZ,     STRDUP("pusharg"), NULL, 0, 0 );    \
    instruction_add ( STRING,       STRDUP("noargs:"), NULL, 0, 0 );    \
} while ( false )

#define TEXT_TAIL() do {\
    instruction_add ( LEAVE, NULL, NULL, 0, 0 );            \
    instruction_add ( PUSH, eax, NULL, 0, 0 );              \
    instruction_add ( SYSCALL, STRDUP("exit"), NULL, 0, 0 );\
} while ( false )




void generate ( FILE *stream, node_t *root )
{
    int elegant_solution;
    if ( root == NULL )
        return;

    switch ( root->type.index )
    {
        case PROGRAM:
            /* Output the data segment */
            strings_output ( stream );
            instruction_add ( STRING, STRDUP( ".text" ), NULL, 0, 0 );

            RECUR();
            TEXT_HEAD();

            instruction_add(CALL, STRDUP( root->children[0]->data ), NULL, 0, 0);
            instruction_add(LEAVE, NULL, NULL, 0, 0);
            instruction_add(PUSH, eax, NULL, 0, 0);
            instruction_add(SYSCALL, STRDUP("exit"), NULL, 0, 0);

            TEXT_TAIL();

            instructions_print ( stream );
            instructions_finalize ();
            break;

        case FUNCTION:
            /*
             * Function definitions:
             * Set up/take down activation record for the function, return value
             */
            instruction_add(LABEL, STRDUP(root->children[0]->data), NULL, 0, 0);
            instruction_add(PUSH, ebp, NULL, 0, 0);
            instruction_add(MOVE, esp, ebp, 0, 0);

            depth += 1;
            generate(stream, root->children[2] ); // why? know this
            instruction_add(LEAVE, NULL, NULL, 0, 0);
            depth -= 1;
            
            instruction_add(RET, NULL, NULL, 0, 0);

            break;

        case BLOCK:
            /*
             * Blocks:
             * Set up/take down activation record, no return value
             */
            instruction_add(PUSH, ebp, NULL, 0, 0);
            instruction_add(MOVE, esp, ebp, 0, 0);

            
            depth += 1;
            RECUR();
            depth -= 1;

            instruction_add(LEAVE, NULL, NULL, 0, 0);

            break;

        case DECLARATION:
            /*
             * Declarations:
             * Add space on local stack
             */
            for ( int i = 0; i < root->children[0]->n_children; i++){
                instruction_add(PUSH, STRDUP("$0"), NULL, 0, 0); 
            }

            break;

        case PRINT_LIST:
           /*
            * Print lists:
            * Emit the list of print items, followed by newline (0x0A)
            */
           /* Print the items in the list */
            RECUR(); 
           /* print a newline */
            instruction_add(PUSH, STRDUP("0x0A\n"), NULL, 0, 0);
            instruction_add(SYSCALL, STRDUP("putchar"), NULL, 0, 0);
            instruction_add(ADD, STRDUP("$4"), esp, 0, 0);
            break;

        case PRINT_ITEM:
           /*
            * Items in print lists:
            * Determine what kind of value (string literal or expression)
            * and set up a suitable call to printf
            */
            if (root->children[0]->type.index == TEXT){
                char string[19];
                sprintf(string, "$.STRING%d", *((int32_t *)root->children[0]->data));
                instruction_add(PUSH, STRDUP(string), NULL, 0, 0);
                instruction_add(SYSCALL, STRDUP("printf"), NULL, 0, 0);
                instruction_add(ADD, STRDUP("$4"), esp, 0, 0);
            }
            else {
                RECUR();
                instruction_add(PUSH, STRDUP("$.INTEGER"), NULL, 0, 0);
                instruction_add(SYSCALL, STRDUP("printf"), NULL, 0, 0);
                instruction_add(ADD, STRDUP("$4"), esp, 0, 0);
            }

            break;

        case EXPRESSION:
           /*
            * Expressions:
            * Handle any nested expressions first, then deal with the
            * top of the stack according to the kind of expression
            * (single variables/integers handled in separate switch/cases)
            */

            /* Nested expression */
            RECUR();
           
            if (root->n_children == 2){
                /* Handle function calls and binary operators */
                if (root->children[0]->type.index == VARIABLE){
                   instruction_add(CALL, STRDUP(root->children[0]->data), NULL, 0, 0);
                   char value[12];
                   sprintf(value, "$%d", 4*root->children[1]->n_children);
                   instruction_add(ADD, STRDUP(value), esp, 0, 0);
                   instruction_add(PUSH, eax, NULL, 0,0);
                }
                else {
                    instruction_add(POP, ebx, NULL, 0, 0);
                    instruction_add(POP, ebx, NULL, 0, 0);
                    switch(*((char *)root->data)){
                        case '+':
                            instruction_add(ADD, ebx, eax, 0, 0);
                            break;
                        case '-':
                            instruction_add(SUB, ebx, eax, 0, 0);
                            break;
                        case '*':
                            instruction_add(CLTD, NULL, NULL, 0, 0);
                            instruction_add(MUL, ebx, NULL, 0, 0);
                            break;
                        case '/':
                            instruction_add(CLTD, NULL, NULL, 0, 0);
                            instruction_add(DIV, ebx, NULL, 0, 0);
                            break;
                    }
                    instruction_add(PUSH, eax, NULL, 0, 0);

                            
                }
            

            }
            else if ( root->n_children == 1 && root->data != NULL){
                /* Handle unary minus */
                instruction_add(POP, eax, NULL, 0, 0);
                instruction_add(NEG, eax, NULL, 0, 0);
                instruction_add(PUSH, eax, NULL, 0, 0);
            }
            

           break;

        case VARIABLE:
           /*
            * Occurrences of variables: (declarations have their own case)
            * - Find the variable's stack offset
            * - If var is not local, unwind the stack to its correct base
            */
            if(root->entry->label == NULL){
                int offset = root->entry->stack_offset;
                instruction_add(MOVE, ebp, ecx, 0, 0);
                for(int i = 0; i < (depth-(root->entry->depth)); i++)
                   instruction_add(MOVE, ecx, ecx, 4, 0);
                instruction_add(PUSH, ecx, NULL, offset, 0);
            }


            break;

        case INTEGER:
           /*
            * Integers: constants which can just be put on stack
            */
            {
                char value[13];
                sprintf(value, "$%d\0", *((int32_t *)root->data));
                instruction_add(PUSH, STRDUP(value), NULL, 0, 0);
            }


            break;

        case ASSIGNMENT_STATEMENT:
           /*
            * Assignments:
            * Right hand side is an expression, find left hand side on stack
            * (unwinding if necessary)
            */
            generate(stream, root->children[1]);
            instruction_add(POP, eax, NULL, 0, 0);

            instruction_add(MOVE, ebp, ecx, 0, 0);
            for (int i = 0; i < (depth - (root->children[0]->entry->depth)); i++)
                instruction_add(MOVE, ecx, ecx, 4, 0);
            instruction_add(MOVE, eax, ecx, 0, root->children[0]->entry->stack_offset);

            break;

        case RETURN_STATEMENT:
           /*
            * Return statements:
            * Evaluate the expression and put it in EAX
            */
            RECUR();
            instruction_add(POP, eax, NULL, 0, 0);

            for (int i = 0; i < depth-1; i++)
                instruction_add(LEAVE, NULL, NULL, 0, 0);
            instruction_add(RET, eax, NULL, 0, 0);
            break;

        default:
            /* Everything else can just continue through the tree */
            RECUR();
            break;
    }
}


/* Provided auxiliaries... */


    static void
instruction_append ( instruction_t *next )
{
    if ( start != NULL )
    {
        last->next = next;
        last = next;
    }
    else
        start = last = next;
}


    static void
instruction_add (
        opcode_t op, char *arg1, char *arg2, int32_t off1, int32_t off2 
        )
{
    instruction_t *i = (instruction_t *) malloc ( sizeof(instruction_t) );
    *i = (instruction_t) { op, {arg1, arg2}, {off1, off2}, NULL };
    instruction_append ( i );
}


    static void
instructions_print ( FILE *stream )
{
    instruction_t *this = start;
    while ( this != NULL )
    {
        switch ( this->opcode )
        {
            case PUSH:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tpushl\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tpushl\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case POP:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tpopl\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tpopl\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case MOVE:
                if ( this->offsets[0] == 0 && this->offsets[1] == 0 )
                    fprintf ( stream, "\tmovl\t%s,%s\n",
                            this->operands[0], this->operands[1]
                            );
                else if ( this->offsets[0] != 0 && this->offsets[1] == 0 )
                    fprintf ( stream, "\tmovl\t%d(%s),%s\n",
                            this->offsets[0], this->operands[0], this->operands[1]
                            );
                else if ( this->offsets[0] == 0 && this->offsets[1] != 0 )
                    fprintf ( stream, "\tmovl\t%s,%d(%s)\n",
                            this->operands[0], this->offsets[1], this->operands[1]
                            );
                break;

            case ADD:
                if ( this->offsets[0] == 0 && this->offsets[1] == 0 )
                    fprintf ( stream, "\taddl\t%s,%s\n",
                            this->operands[0], this->operands[1]
                            );
                else if ( this->offsets[0] != 0 && this->offsets[1] == 0 )
                    fprintf ( stream, "\taddl\t%d(%s),%s\n",
                            this->offsets[0], this->operands[0], this->operands[1]
                            );
                else if ( this->offsets[0] == 0 && this->offsets[1] != 0 )
                    fprintf ( stream, "\taddl\t%s,%d(%s)\n",
                            this->operands[0], this->offsets[1], this->operands[1]
                            );
                break;
            case SUB:
                if ( this->offsets[0] == 0 && this->offsets[1] == 0 )
                    fprintf ( stream, "\tsubl\t%s,%s\n",
                            this->operands[0], this->operands[1]
                            );
                else if ( this->offsets[0] != 0 && this->offsets[1] == 0 )
                    fprintf ( stream, "\tsubl\t%d(%s),%s\n",
                            this->offsets[0], this->operands[0], this->operands[1]
                            );
                else if ( this->offsets[0] == 0 && this->offsets[1] != 0 )
                    fprintf ( stream, "\tsubl\t%s,%d(%s)\n",
                            this->operands[0], this->offsets[1], this->operands[1]
                            );
                break;
            case MUL:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\timull\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\timull\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case DIV:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tidivl\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tidivl\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case NEG:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tnegl\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tnegl\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;

            case DECL:
                fprintf ( stream, "\tdecl\t%s\n", this->operands[0] );
                break;
            case CLTD:
                fprintf ( stream, "\tcltd\n" );
                break;
            case CBW:
                fprintf ( stream, "\tcbw\n" );
                break;
            case CWDE:
                fprintf ( stream, "\tcwde\n" );
                break;
            case CMPZERO:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tcmpl\t$0,%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tcmpl\t$0,%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case CMP:
                if ( this->offsets[0] == 0 && this->offsets[1] == 0 )
                    fprintf ( stream, "\tcmpl\t%s,%s\n",
                            this->operands[0], this->operands[1]
                            );
                else if ( this->offsets[0] != 0 && this->offsets[1] == 0 )
                    fprintf ( stream, "\tcmpl\t%d(%s),%s\n",
                            this->offsets[0], this->operands[0], this->operands[1]
                            );
                else if ( this->offsets[0] == 0 && this->offsets[1] != 0 )
                    fprintf ( stream, "\tcmpl\t%s,%d(%s)\n",
                            this->operands[0], this->offsets[1], this->operands[1]
                            );
                break;
            case SETL:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tsetl\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tsetl\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case SETG:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tsetg\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tsetg\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case SETLE:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tsetle\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tsetle\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case SETGE:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tsetge\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tsetge\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case SETE:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tsete\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tsete\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;
            case SETNE:
                if ( this->offsets[0] == 0 )
                    fprintf ( stream, "\tsetne\t%s\n", this->operands[0] );
                else
                    fprintf ( stream, "\tsetne\t%d(%s)\n",
                            this->offsets[0], this->operands[0]
                            );
                break;

            case CALL: case SYSCALL:
                fprintf ( stream, "\tcall\t" );
                if ( this->opcode == CALL )
                    fputc ( '_', stream );
                fprintf ( stream, "%s\n", this->operands[0] );
                break;
            case LABEL: 
                fprintf ( stream, "_%s:\n", this->operands[0] );
                break;

            case JUMP:
                fprintf ( stream, "\tjmp\t%s\n", this->operands[0] );
                break;
            case JUMPZERO:
                fprintf ( stream, "\tjz\t%s\n", this->operands[0] );
                break;
            case JUMPEQ:
                fprintf ( stream, "\tje\t%s\n", this->operands[0] );
                break;
            case JUMPNONZ:
                fprintf ( stream, "\tjnz\t%s\n", this->operands[0] );
                break;

            case LEAVE: fputs ( "\tleave\n", stream ); break;
            case RET:   fputs ( "\tret\n", stream );   break;

            case STRING:
                        fprintf ( stream, "%s\n", this->operands[0] );
                        break;

            case NIL:
                        break;

            default:
                        fprintf ( stderr, "Error in instruction stream\n" );
                        break;
        }
        this = this->next;
    }
}


    static void
instructions_finalize ( void )
{
    instruction_t *this = start, *next;
    while ( this != NULL )
    {
        next = this->next;
        if ( this->operands[0] != eax && this->operands[0] != ebx &&
                this->operands[0] != ecx && this->operands[0] != edx &&
                this->operands[0] != ebp && this->operands[0] != esp &&
                this->operands[0] != esi && this->operands[0] != edi &&
                this->operands[0] != al && this->operands[0] != bl 
           )
            free ( this->operands[0] );
        free ( this );
        this = next;
    }
}
