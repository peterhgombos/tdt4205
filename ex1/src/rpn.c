#include "rpn.h"
#include <stdlib.h>

RpnCalc newRpnCalc(){
    RPNCalc calc;
    calc.size = 64;
    calc.top = 0;
    calc.stack = (double)malloc(size*sizeof(double));
    return calc;
}

void push(RpnCalc* rpnCalc, double n){
    // TODO push n on stack, expand stack if neccesary
    if(rpnCalc.top == rpnCalc.size){
        // TODO expand stack to twice the size
        rpnCalc.size = 2*rpnCalc.size;
    }
    rpnCalc[++top] = n;
    return;
}

void performOp(RpnCalc* rpnCalc, char op){
    // TODO perform operation
}

double peek(RpnCalc* rpnCalc){
    // TODO return top element of stack
}
